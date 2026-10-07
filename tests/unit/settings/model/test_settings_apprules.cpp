#include "config/loader.h"
#include "document/apprules.h"

#include <QJsonDocument>
#include <QTest>

using namespace Konveyor::Settings;

namespace
{

AppPlace dolphin()
{
    AppPlace place;
    place.appId = QStringLiteral("org.kde.dolphin");
    place.output = QStringLiteral("DP-1");
    place.workspaceIndex = 2;
    place.column = 3;
    place.width = 800;
    return place;
}

QString applied(const QString &text, const AppPlace &place, const QList<std::pair<QString, bool>> &changes)
{
    ConfigDocument document(text);
    for (const auto &[option, enabled] : changes) {
        if (const EditResult result = setAppRule(document, place, option, enabled); !result) {
            return QStringLiteral("<error: ") + result.error() + QLatin1Char('>');
        }
    }
    const auto loaded = Konveyor::Config::loadString(document.text(), QStringLiteral("config.kdl"));
    return loaded ? document.text() : QStringLiteral("<invalid: ") + loaded.error().toString() + QLatin1Char('>');
}

QJsonObject rulesOf(const QString &text, const QString &output = QStringLiteral("DP-1"))
{
    return appRules(ConfigDocument(text), QStringLiteral("org.kde.dolphin"), output);
}

}

class TestSettingsAppRules : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void patternsMatchTheAppIdExactly()
    {
        QCOMPARE(appIdPattern(QStringLiteral("org.kde.dolphin")), QStringLiteral(R"(^org\.kde\.dolphin$)"));
        QCOMPARE(appIdPattern(QStringLiteral("a+b(c)")), QStringLiteral(R"(^a\+b\(c\)$)"));
        QCOMPARE(appIdPattern(QStringLiteral("steam_app-1")), QStringLiteral("^steam_app-1$"));
    }

    void enablingWritesOneRuleForTheApp()
    {
        const QString text = applied(QStringLiteral("// mine\nlayout {\n    gaps 8\n}\n"), dolphin(),
            {{QStringLiteral("float"), true}, {QStringLiteral("column"), true}, {QStringLiteral("workspace"), true},
                {QStringLiteral("monitor"), true}, {QStringLiteral("all-workspaces"), true}});
        QCOMPARE(text,
            QStringLiteral("// mine\nlayout {\n    gaps 8\n}\n\nwindow-rule {\n    match app-id=r#\"^org\\.kde\\.dolphin$\"#\n"
                           "    open-floating true\n    open-at-column 3\n    open-on-workspace 2\n    open-on-output \"DP-1\"\n"
                           "    open-on-all-workspaces true\n}\n"));
    }

    void stackAndRowUseTheirKeywords()
    {
        const QString text = applied(QString(), dolphin(), {{QStringLiteral("stack"), true}, {QStringLiteral("row"), true}});
        QVERIFY2(text.contains(QStringLiteral("    group-app-windows \"stack\"\n    new-window-placement \"stack\"\n")), qPrintable(text));
        QCOMPARE(rulesOf(text).value(QStringLiteral("stack")).toBool(), true);
        QCOMPARE(rulesOf(text).value(QStringLiteral("row")).toBool(), true);
    }

    void namedWorkspacesAreRememberedByName()
    {
        AppPlace place = dolphin();
        place.workspaceName = QStringLiteral("files");
        const QString text = applied(QStringLiteral("workspace \"files\"\n"), place, {{QStringLiteral("workspace"), true}});
        QVERIFY2(text.contains(QStringLiteral("    open-on-workspace \"files\"\n")), qPrintable(text));
        QCOMPARE(rulesOf(text).value(QStringLiteral("workspace")).toString(), QStringLiteral("files"));
    }

    void disablingRemovesTheNodeAndThenTheRule()
    {
        const QString both = applied(QString(), dolphin(), {{QStringLiteral("float"), true}, {QStringLiteral("column"), true}});
        const QString one = applied(both, dolphin(), {{QStringLiteral("float"), false}});
        QVERIFY(!one.contains(QStringLiteral("open-floating")));
        QVERIFY(one.contains(QStringLiteral("open-at-column 3")));
        QCOMPARE(applied(one, dolphin(), {{QStringLiteral("column"), false}}), QString());
        QCOMPARE(applied(QStringLiteral("layout {\n}\n"), dolphin(), {{QStringLiteral("float"), false}}), QStringLiteral("layout {\n}\n"));
    }

    void userRulesForTheAppAreLeftAlone()
    {
        const QString mine
            = QStringLiteral("window-rule {\n    match app-id=\"^org\\\\.kde\\\\.dolphin$\" title=\"Copy\"\n    open-floating true\n}\n");
        QCOMPARE(rulesOf(mine).value(QStringLiteral("float")).toBool(), false);
        const QString text = applied(mine, dolphin(), {{QStringLiteral("column"), true}, {QStringLiteral("float"), false}});
        QVERIFY(text.startsWith(mine));
        QCOMPARE(text.count(QStringLiteral("window-rule")), 2);
        const QString excluded = QStringLiteral(
            "window-rule {\n    match app-id=\"^org\\\\.kde\\\\.dolphin$\"\n    exclude title=\"x\"\n    open-floating true\n}\n");
        QCOMPARE(rulesOf(excluded).value(QStringLiteral("float")).toBool(), false);
    }

    void theLastMatchingRuleWins()
    {
        const QString rule = QStringLiteral("window-rule {\n    match app-id=\"^org\\\\.kde\\\\.dolphin$\"\n    open-at-column %1\n}\n");
        const QString text = rule.arg(1) + rule.arg(4);
        QCOMPARE(rulesOf(text).value(QStringLiteral("column")).toInt(), 4);
        QCOMPARE(applied(text, dolphin(), {{QStringLiteral("column"), true}}), rule.arg(1) + rule.arg(3));
    }

    void sizeIsRememberedPerMonitor()
    {
        AppPlace place = dolphin();
        place.height = 500;
        const QString text = applied(QString(), place, {{QStringLiteral("size"), true}});
        QCOMPARE(text,
            QStringLiteral("window-rule {\n    match app-id=r#\"^org\\.kde\\.dolphin$\"# output=\"^DP-1$\"\n"
                           "    default-column-width { fixed 800; }\n    default-window-height { fixed 500; }\n}\n"));
        QCOMPARE(rulesOf(text).value(QStringLiteral("size")).toObject(),
            (QJsonObject {{QStringLiteral("width"), 800}, {QStringLiteral("height"), 500}}));
        QVERIFY(rulesOf(text, QStringLiteral("HDMI-A-1")).value(QStringLiteral("size")).isNull());
        QVERIFY(rulesOf(text).value(QStringLiteral("column")).isNull());
        place.height.reset();
        place.width = 640;
        const QString resized = applied(text, place, {{QStringLiteral("size"), true}});
        QCOMPARE(rulesOf(resized).value(QStringLiteral("size")).toObject(),
            (QJsonObject {{QStringLiteral("width"), 640}, {QStringLiteral("height"), QJsonValue()}}));
        QCOMPARE(applied(resized, place, {{QStringLiteral("size"), false}}), QString());
    }

    void refusesWhatCannotBeRemembered()
    {
        AppPlace floating = dolphin();
        floating.column.reset();
        QVERIFY(applied(QString(), floating, {{QStringLiteral("column"), true}}).startsWith(QStringLiteral("<error: ")));
        QVERIFY(applied(QString(), dolphin(), {{QStringLiteral("sideways"), true}}).startsWith(QStringLiteral("<error: ")));
        AppPlace nameless = dolphin();
        nameless.appId.clear();
        QVERIFY(applied(QString(), nameless, {{QStringLiteral("float"), true}}).startsWith(QStringLiteral("<error: ")));
    }
};

QTEST_GUILESS_MAIN(TestSettingsAppRules)

#include "test_settings_apprules.moc"
