#include "document/apprules.h"
#include "document/borderdefaults.h"
#include "document/configdocument.h"
#include "document/configmigration.h"
#include "documenttesthelpers.h"

#include <QTest>

#include <functional>

using namespace Konveyor::Settings;
using namespace Konveyor::Settings::Testing;

namespace
{

class FailingEdits
{
public:
    explicit FailingEdits(int successfulEdits) { ConfigDocument::failEditsAfter(successfulEdits); }
    ~FailingEdits() { ConfigDocument::failEditsAfter(std::nullopt); }
    FailingEdits(const FailingEdits &) = delete;
    FailingEdits &operator=(const FailingEdits &) = delete;
};

constexpr int MostEditsAnyStepNeeds = 40;

AppPlace dolphin()
{
    AppPlace place;
    place.appId = QStringLiteral("org.kde.dolphin");
    place.output = QStringLiteral("DP-1");
    place.workspaceIndex = 2;
    place.column = 3;
    place.width = 800;
    place.height = 500;
    return place;
}

QString migrated(const std::expected<QString, QString> &result)
{
    return result ? *result : QStringLiteral("<error: ") + result.error() + QLatin1Char('>');
}

int editsItNeeds(const QString &input, const std::function<QString()> &run)
{
    for (int successful = 0; successful < MostEditsAnyStepNeeds; ++successful) {
        const FailingEdits failing(successful);
        if (run() != input) {
            return successful;
        }
    }
    return -1;
}

const QString shippedRing = QStringLiteral(R"(layout {
    focus-ring {
        width 2
        active-style "Glow"
        inactive-style "Dim"
        urgent-style "Alert"
    }
}
border-style "Glow" {
    layer "gradient"
}
border-style "Dim" {
    layer "gradient"
}
border-style "Alert" {
    layer "gradient"
}
)");

const QString oldRing = QStringLiteral(R"(layout {
    focus-ring {
        width 4
        inactive-color "#505050"
    }
}
)");

const QString hotStyle = QStringLiteral(R"(border-style "Hot" {
    layer "heat" {
        colors "#ff0000" "#0000ff"
        baseline 0.2
        position 0.5
        drive "position" "scroll.distance" amount=1
    }
}
)");

const QString idleRipple = QStringLiteral(R"(layout {
    focus-ring {
        active-style "Pulse"
    }
}
border-style "Pulse" {
    layer "ripple"
}
border-style "Other" {
    layer "ripple"
}
)");

}

class TestSettingsEditFailures : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void aFailingEditLeavesTheDocumentAlone();
    void aFailedChildEditRollsTheWholeUpdateBack();
    void aFailedRuleWriteIsReported();
    void aFailedSizeWriteIsReported();
    void failedMigrationsKeepTheConfig();
    void failedBindEditsAreSkipped();
};

void TestSettingsEditFailures::aFailingEditLeavesTheDocumentAlone()
{
    const QString text = QStringLiteral("layout {\n    gaps 4\n}\n");
    ConfigDocument document(text);
    {
        const FailingEdits failing(0);
        const EditResult result = document.setNode(QStringLiteral("layout/gaps"), leaf(QStringLiteral("gaps"), {8}));
        QVERIFY(!result);
        QCOMPARE(result.error(), QStringLiteral("the edit could not be written"));
    }
    QCOMPARE(document.text(), text);
    QVERIFY(document.setNode(QStringLiteral("layout/gaps"), leaf(QStringLiteral("gaps"), {8})).has_value());
    QCOMPARE(document.text(), QStringLiteral("layout {\n    gaps 8\n}\n"));
}

void TestSettingsEditFailures::aFailedChildEditRollsTheWholeUpdateBack()
{
    const QString text = QStringLiteral("output \"DP-1\" {\n    scale 1\n    off\n    transform \"normal\"\n}\n");
    const QVariantMap rewritten = block(QStringLiteral("output"),
        {leaf(QStringLiteral("transform"), {QStringLiteral("90")}), leaf(QStringLiteral("scale"), {2}), leaf(QStringLiteral("mode"), {1})},
        {QStringLiteral("HDMI-A-1")});
    int rolledBack = 0;
    for (int successful = 0; successful < MostEditsAnyStepNeeds; ++successful) {
        ConfigDocument document(text);
        EditResult result;
        {
            const FailingEdits failing(successful);
            result = document.setNode(QStringLiteral("output"), rewritten);
        }
        if (result) {
            QVERIFY(document.text().startsWith(QStringLiteral("output \"HDMI-A-1\"")));
            QVERIFY(!document.text().contains(QStringLiteral("off")));
            break;
        }
        ++rolledBack;
        QCOMPARE(document.text(), text);
        QCOMPARE(document.setNode(QStringLiteral("output/scale"), leaf(QStringLiteral("scale"), {3})).value_or(QString()),
            QStringLiteral("output/scale"));
    }
    QVERIFY(rolledBack >= 5);
}

void TestSettingsEditFailures::aFailedRuleWriteIsReported()
{
    for (const QString &option : {QStringLiteral("float"), QStringLiteral("column")}) {
        ConfigDocument document;
        const FailingEdits failing(0);
        const EditResult result = setAppRule(document, dolphin(), option, true);
        QVERIFY(!result);
        QCOMPARE(document.text(), QString());
    }
    const QString both = QStringLiteral("window-rule {\n    match app-id=r#\"^org\\.kde\\.dolphin$\"#\n    open-floating true\n}\n");
    ConfigDocument document(both);
    const FailingEdits failing(0);
    QVERIFY(!setAppRule(document, dolphin(), QStringLiteral("float"), false));
    QCOMPARE(document.text(), both);
}

void TestSettingsEditFailures::aFailedSizeWriteIsReported()
{
    const QString rule = QStringLiteral("window-rule {\n    match app-id=r#\"^org\\.kde\\.dolphin$\"# output=\"^DP-1$\"\n}\n");
    for (int successful = 0; successful < 2; ++successful) {
        ConfigDocument document(rule);
        const FailingEdits failing(successful);
        const EditResult result = setAppRule(document, dolphin(), QStringLiteral("size"), true);
        QVERIFY(!result);
        QCOMPARE(result.error(), QStringLiteral("the edit could not be written"));
    }
    ConfigDocument empty;
    const FailingEdits failing(0);
    QVERIFY(!setAppRule(empty, dolphin(), QStringLiteral("size"), true));
}

void TestSettingsEditFailures::failedMigrationsKeepTheConfig()
{
    const QString file = QStringLiteral("/tmp/konveyor-edit-failures/config.kdl");
    const auto needed = [&](const QString &input, auto migrate, const QString &defaults) {
        return editsItNeeds(input, [&] { return migrated(migrate(input, file, defaults)); });
    };
    QVERIFY(needed(hotStyle, migrateHeatLayers, QString()) > 3);
    QVERIFY(needed(idleRipple, migrateFocusEffects, QString()) >= 1);
    QVERIFY(needed(oldRing, migrateBorderDefaults, shippedRing) > 3);
}

void TestSettingsEditFailures::failedBindEditsAreSkipped()
{
    const QString file = QStringLiteral("/tmp/konveyor-edit-failures/config.kdl");
    const QString text = QStringLiteral("binds {\n    Mod+F { fullscreen-window; }\n}\n");
    const QString defaults = QStringLiteral("binds {\n    Mod+F { cycle-window-expansion; }\n    Super+Alt+1 { focus-column 1; }\n}\n");
    {
        const FailingEdits failing(0);
        QCOMPARE(migrated(replaceBindAction(text, file, defaults, {QStringLiteral("Mod+F")}, QStringLiteral("fullscreen-window"))), text);
        QCOMPARE(migrated(addDefaultBinds(text, file, defaults, {QStringLiteral("Super+Alt+1")})), text);
    }
    QVERIFY(migrated(addDefaultBinds(text, file, defaults, {QStringLiteral("Super+Alt+1")})).contains(QStringLiteral("Super+Alt+1")));
}

QTEST_GUILESS_MAIN(TestSettingsEditFailures)

#include "test_settings_edit_failures.moc"
