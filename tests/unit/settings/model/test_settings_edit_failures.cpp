#include "document/apprules.h"
#include "document/borderdefaults.h"
#include "document/configdocument.h"
#include "document/configmigration.h"
#include "documenttesthelpers.h"

#include <QTest>

#include <limits>

using namespace Konveyor::Settings;
using namespace Konveyor::Settings::Testing;

namespace
{

constexpr double unwritable = std::numeric_limits<double>::infinity();

const QString configFile = QStringLiteral("config.kdl");

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

const QString shippedRing = QStringLiteral(R"(layout {
    focus-ring {
        width 1e999
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

}

class TestSettingsEditFailures : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void aFailingEditLeavesTheDocumentAlone();
    void aFailedChildEditRollsTheWholeUpdateBack();
    void aFailedRuleWriteIsReported();
    void failedBorderMigrationKeepsTheConfig();
    void failedBindEditsAreSkipped();
    void widthsThatAreNotTheOldDefaultStay();
};

void TestSettingsEditFailures::aFailingEditLeavesTheDocumentAlone()
{
    const QString text = QStringLiteral("layout {\n    gaps 4\n}\n");
    ConfigDocument document(text);
    const EditResult result = document.setNode(QStringLiteral("layout/gaps"), leaf(QStringLiteral("gaps"), {unwritable}));
    QVERIFY(!result);
    QVERIFY(result.error().startsWith(QStringLiteral("edit produced invalid KDL: ")));
    QCOMPARE(document.text(), text);
    QVERIFY(document.setNode(QStringLiteral("layout/gaps"), leaf(QStringLiteral("gaps"), {8})).has_value());
    QCOMPARE(document.text(), QStringLiteral("layout {\n    gaps 8\n}\n"));
}

void TestSettingsEditFailures::aFailedChildEditRollsTheWholeUpdateBack()
{
    const QString text = QStringLiteral("output \"DP-1\" {\n    scale 1\n    off\n    transform \"normal\"\n}\n");
    const QVariantMap good[] = {
        leaf(QStringLiteral("transform"), {QStringLiteral("90")}), leaf(QStringLiteral("scale"), {2}), leaf(QStringLiteral("mode"), {1})};
    for (int broken = 0; broken < 3; ++broken) {
        QVariantList children;
        for (int position = 0; position < 3; ++position) {
            QVariantMap child = good[position];
            if (position == broken) {
                child.insert(QStringLiteral("args"), QVariantList {unwritable});
            }
            children.append(child);
        }
        const QVariantMap rewritten = block(QStringLiteral("output"), children, {QStringLiteral("HDMI-A-1")});
        ConfigDocument document(text);
        QVERIFY(!document.setNode(QStringLiteral("output"), rewritten));
        QCOMPARE(document.text(), text);
        QCOMPARE(document.setNode(QStringLiteral("output/scale"), leaf(QStringLiteral("scale"), {3})).value_or(QString()),
            QStringLiteral("output/scale"));
    }
}

void TestSettingsEditFailures::aFailedRuleWriteIsReported()
{
    const QString broken = QStringLiteral("layout {\n    gaps 4\n");
    for (const QString &option : {QStringLiteral("float"), QStringLiteral("column"), QStringLiteral("size")}) {
        ConfigDocument document(broken);
        const EditResult result = setAppRule(document, dolphin(), option, true);
        QVERIFY(!result);
        QCOMPARE(document.text(), broken);
    }
}

void TestSettingsEditFailures::failedBorderMigrationKeepsTheConfig()
{
    QCOMPARE(migrated(migrateBorderDefaults(oldRing, configFile, shippedRing)), oldRing);
    QString writable = shippedRing;
    writable.replace(QStringLiteral("1e999"), QStringLiteral("2"));
    QVERIFY(migrated(migrateBorderDefaults(oldRing, configFile, writable)) != oldRing);
    QString unwritableStyle = writable;
    unwritableStyle.replace(QStringLiteral("border-style \"Glow\" {\n    layer \"gradient\""),
        QStringLiteral("border-style \"Glow\" {\n    layer \"gradient\" 1e999"));
    QVERIFY(unwritableStyle != writable);
    QCOMPARE(migrated(migrateBorderDefaults(oldRing, configFile, unwritableStyle)), oldRing);
}

void TestSettingsEditFailures::widthsThatAreNotTheOldDefaultStay()
{
    QString writable = shippedRing;
    writable.replace(QStringLiteral("1e999"), QStringLiteral("2"));
    for (const QString &width : {QStringLiteral("width \"4\""), QStringLiteral("width 4 4"), QStringLiteral("width 3")}) {
        const QString ring
            = QStringLiteral("layout {\n    focus-ring {\n        %1\n        inactive-color \"#505050\"\n    }\n}\n").arg(width);
        const QString result = migrated(migrateBorderDefaults(ring, configFile, writable));
        QVERIFY2(result.contains(width), qPrintable(result));
        QVERIFY2(!result.contains(QStringLiteral("width 2")), qPrintable(result));
    }
}

void TestSettingsEditFailures::failedBindEditsAreSkipped()
{
    const QString text = QStringLiteral("binds {\n    Mod+F { cycle-window-expansion; }\n}\n");
    const QString action = QStringLiteral("cycle-window-expansion");
    const QString unwritableDefaults = QStringLiteral("binds {\n    Mod+F { fullscreen-window size=1e999; }\n}\n");
    const QString writableDefaults = QStringLiteral("binds {\n    Mod+F { fullscreen-window; }\n}\n");
    QCOMPARE(migrated(replaceBindAction(text, configFile, unwritableDefaults, {QStringLiteral("Mod+F")}, action)), text);
    QVERIFY(migrated(replaceBindAction(text, configFile, writableDefaults, {QStringLiteral("Mod+F")}, action)) != text);

    const QString binds = QStringLiteral("binds {\n    Mod+G { fullscreen-window; }\n}\n");
    const QString defaults = QStringLiteral("binds {\n    Super+Alt+1 { focus-column 1e999; }\n    Super+Alt+2 { focus-column 2; }\n}\n");
    const QString added
        = migrated(addDefaultBinds(binds, configFile, defaults, {QStringLiteral("Super+Alt+1"), QStringLiteral("Super+Alt+2")}));
    QVERIFY(!added.contains(QStringLiteral("Super+Alt+1")));
    QVERIFY(added.contains(QStringLiteral("Super+Alt+2")));
}

QTEST_GUILESS_MAIN(TestSettingsEditFailures)

#include "test_settings_edit_failures.moc"
