#include "settingsqmlharness.h"

using namespace Konveyor::Settings::Testing;

namespace
{

QVariant fromJson(const QByteArray &json)
{
    return QJsonDocument::fromJson("[" + json + "]").array().first().toVariant();
}

}

class TestSettingsCatalogQml : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase() { QVERIFY(m_home.setUp()); }

    void functions_data();
    void functions();
    void everyActionLoads();

private:
    SettingsHome m_home;
};

void TestSettingsCatalogQml::functions_data()
{
    QTest::addColumn<QString>("library");
    QTest::addColumn<QString>("function");
    QTest::addColumn<QByteArray>("arguments");
    QTest::addColumn<QString>("config");
    QTest::addColumn<QByteArray>("expected");
    const auto row = [](const char *name, const char *library, const char *function, const char *arguments, const char *expected,
                         const char *config = "") {
        QTest::newRow(name) << QString::fromLatin1(library) << QString::fromLatin1(function) << QByteArray(arguments)
                            << QString::fromUtf8(config) << QByteArray(expected);
    };
    row("proportion rounds", "Kdl", "sizeNode", R"j([{"kind":"proportion","value":0.123456789}])j",
        R"j({"name":"proportion","args":[0.12346],"props":{}})j");
    row("fixed rounds", "Kdl", "sizeNode", R"j([{"kind":"fixed","value":99.5}])j", R"j({"name":"fixed","args":[100],"props":{}})j");
    row("size block empty", "Kdl", "sizeBlock", R"j(["w", null])j", R"j({"name":"w","args":[],"props":{},"children":[]})j");
    row("label none", "Kdl", "sizeLabel", "[null]", R"j("App decides")j");
    row("label px", "Kdl", "sizeLabel", R"j([{"kind":"fixed","value":640.4}])j", R"j("640 px")j");
    row("label whole percent", "Kdl", "sizeLabel", R"j([{"kind":"proportion","value":0.5}])j", R"j("50%")j");
    row("label near percent", "Kdl", "sizeLabel", R"j([{"kind":"proportion","value":0.33333}])j", R"j("33.3%")j");
    row("label fraction percent", "Kdl", "sizeLabel", R"j([{"kind":"proportion","value":0.125}])j", R"j("12.5%")j");
    row("is", "Kdl", "textMatch", R"j(["is","org.kde.dolphin"])j", R"j("^org\\.kde\\.dolphin$")j");
    row("starts", "Kdl", "textMatch", R"j(["starts","a+b"])j", R"j("^a\\+b")j");
    row("ends", "Kdl", "textMatch", R"j(["ends","(x)"])j", R"j("\\(x\\)$")j");
    row("contains", "Kdl", "textMatch", R"j(["contains","a/b"])j", R"j("a\\/b")j");
    row("pattern kept", "Kdl", "textMatch", R"j(["pattern","^a.*$"])j", R"j("^a.*$")j");
    row("parse is", "Kdl", "parseTextMatch", R"j(["^org\\.kde$"])j", R"j({"mode":"is","text":"org.kde"})j");
    row("parse starts", "Kdl", "parseTextMatch", R"j(["^fire"])j", R"j({"mode":"starts","text":"fire"})j");
    row("parse ends", "Kdl", "parseTextMatch", R"j(["fox$"])j", R"j({"mode":"ends","text":"fox"})j");
    row("parse escaped dollar", "Kdl", "parseTextMatch", R"j(["cost\\$"])j", R"j({"mode":"contains","text":"cost$"})j");
    row("parse pattern", "Kdl", "parseTextMatch", R"j(["^a.*b$"])j", R"j({"mode":"pattern","text":"^a.*b$"})j");
    row("parse missing", "Kdl", "parseTextMatch", "[null]", R"j({"mode":"is","text":""})j");
    row("unescape plain", "Kdl", "unescapeRegex", R"j(["a\\.b"])j", R"j("a.b")j");
    row("unescape rejects pattern", "Kdl", "unescapeRegex", R"j(["a|b"])j", "null");
    row("paint none", "Kdl", "paintMode", "[null]", R"j("none")j");
    row("paint theme", "Kdl", "paintMode", R"j([{"source":"accent"}])j", R"j("theme")j");
    row("paint color", "Kdl", "paintMode", R"j([{"source":"color","color":"#ff000000"}])j", R"j("color")j");
    row("paint gradient", "Kdl", "paintMode", R"j([{"source":"color","gradient":{}}])j", R"j("gradient")j");
    row("gradient defaults dropped", "Kdl", "gradientProps",
        R"j([{"from":"#ff000000","to":"#ffffffff","angle":179.6,"relative-to":"window","in":"srgb"}])j",
        R"j({"from":"#000000","to":"#ffffff","angle":180})j");
    row("humanize", "Kdl", "humanize", R"j(["focus-column-left"])j", R"j("Focus column left")j");
    row("title case empty", "Kdl", "titleCase", R"j([""])j", R"j("")j");
    row("corner rule none", "CornerRule", "find", R"j(["@store"])j", R"j({"path":"","radii":[0,0,0,0],"clip":false})j",
        "window-rule { match app-id=\"a\"; geometry-corner-radius 4; }\n");
    row("corner rule one", "CornerRule", "find", R"j(["@store"])j", R"j({"path":"window-rule#1","radii":[6,6,6,6],"clip":true})j",
        "window-rule { match app-id=\"a\"; }\nwindow-rule { geometry-corner-radius 6; clip-to-geometry true; }\n");
    row("corner rule four", "CornerRule", "find", R"j(["@store"])j", R"j({"path":"window-rule","radii":[1,2,3,4],"clip":false})j",
        "window-rule { geometry-corner-radius 1 2 3 4; clip-to-geometry false; }\n");
    row("corner rule excluded", "CornerRule", "find", R"j(["@store"])j", R"j({"path":"","radii":[0,0,0,0],"clip":false})j",
        "window-rule { exclude app-id=\"a\"; }\n");
    row("argb short", "RulePaint", "argbFromCss", R"j(["#f00"])j", R"j("#ff0000")j");
    row("argb short alpha", "RulePaint", "argbFromCss", R"j(["#f008"])j", R"j("#88ff0000")j");
    row("argb long alpha", "RulePaint", "argbFromCss", R"j(["#11223344"])j", R"j("#44112233")j");
    row("argb keyword", "RulePaint", "argbFromCss", R"j(["accent"])j", R"j("accent")j");
    row("paint missing", "RulePaint", "paintOf", R"j([null,"active"])j", "null");
    row("paint theme upper", "RulePaint", "paintOf", R"j([{"children":[{"name":"active-color","args":["Accent"]}]},"active"])j",
        R"j({"source":"accent","color":""})j");
    row("rule paint color", "RulePaint", "paintOf", R"j([{"children":[{"name":"urgent-color","args":["#ff000080"]}]},"urgent"])j",
        R"j({"source":"color","color":"#80ff0000"})j");
    row("rule paint gradient", "RulePaint", "paintOf",
        R"j([{"children":[{"name":"active-gradient","props":{"from":"#000","to":"#fff"}}]},"active"])j",
        R"j({"source":"color","color":"","gradient":{"from":"#000000","to":"#ffffff","angle":180,"relative-to":"window","in":"srgb"}})j");
    row("ratio known", "MonitorSummary", "ratioLabel", "[1.78]", R"j("16:9")j");
    row("ratio other", "MonitorSummary", "ratioLabel", "[1.5]", R"j("1.5:1")j");
    row("match portrait", "MonitorSummary", "describeMatch", R"j([{"props":{"aspect-ratio-below":1}}])j",
        R"j("portrait (taller than wide)")j");
    row("match all", "MonitorSummary", "describeMatch",
        R"j([{"props":{"name":"^DP-1$","aspect-ratio-above":2,"aspect-ratio-below":3,"width-above":1000,"width-below":4000,"height-above":500,"height-below":3000}}])j",
        R"j("named DP-1, narrower than 3:1, wider than 2:1, more than 1000 px wide, less than 4000 px wide, more than 500 px tall, less than 3000 px tall")j");
    row("match pattern name", "MonitorSummary", "describeMatch", R"j([{"props":{"name":"DP-(1|2)"}}])j",
        R"j("with a name matching a pattern")j");
    row("match any", "MonitorSummary", "describeMatch", R"j([{"props":{}}])j", R"j("any monitor")j");
    row("profile fallback", "MonitorSummary", "describeProfile", R"j([{"children":[]}])j",
        R"j("Every monitor that no earlier profile claims")j");
    row("layout summary", "MonitorSummary", "describeLayout",
        R"j([{"children":[{"name":"layout","children":[{"name":"default-column-width","children":[{"name":"proportion","args":[1]}]},{"name":"new-window-placement","args":["stack"]},{"name":"max-rows-per-column","args":[2]}]}]}])j",
        R"j("full-width columns, stacks 2 per column")j");
    row("override count", "MonitorSummary", "overrideCount",
        R"j([{"children":[{"name":"layout","children":[{"name":"gaps"},{"name":"struts"}]}]}])j", "2");
    row("same output", "MonitorSummary", "sameOutput", R"j(["DP-1","dp-1"])j", "true");
    row("different output", "MonitorSummary", "sameOutput", R"j(["DP-1","DP-10"])j", "false");
    row("includes output", "MonitorSummary", "includesOutput", R"j([["HDMI-A-1","Dp-2"],"DP-2"])j", "true");
    row("rule target", "RuleSummary", "target",
        R"j([{"children":[{"name":"match","props":{"app-id":"^fire","is-floating":true}},{"name":"exclude","props":{"title":"^Library"}}]},null])j",
        R"j("Apps starting with “fire” floating, except windows titled starting with “Library”")j");
    row("rule effects", "RuleSummary", "summary",
        R"j([{"children":[{"name":"open-floating","args":[true]},{"name":"opacity","args":[0.9]},{"name":"geometry-corner-radius","args":[4,4,4,4]},{"name":"default-column-width","children":[]}]}])j",
        R"j("Opens floating, app picks its width, 90% opaque, 4 px rounded corners")j");
    row("rule empty", "RuleSummary", "summary", R"j([{"children":[]}])j", R"j("Doesn't change anything yet")j");
    row("rule app ids", "RuleSummary", "appIds", R"j([{"children":[{"name":"match","props":{"app-id":"^(a|b\\.c)$"}}]}])j",
        R"j(["a","b.c"])j");
    row("linear", "MotionMath", "easingAt", R"j(["linear",[],0.25])j", "0.25");
    row("quad", "MotionMath", "easingAt", R"j(["ease-out-quad",[],0.5])j", "0.75");
    row("clamped", "MotionMath", "easingAt", R"j(["ease-out-cubic",[],2])j", "1");
    row("critical spring", "MotionMath", "springDurationMs", "[1,100,0.001]", "690.7755278982137");
    row("no damping", "MotionMath", "springDurationMs", "[0,100,0.001]", "10000");
}

void TestSettingsCatalogQml::functions()
{
    QFETCH(QString, library);
    QFETCH(QString, function);
    QFETCH(QByteArray, arguments);
    QFETCH(QString, config);
    QFETCH(QByteArray, expected);
    m_home.resetConfig(config);
    QQmlEngine engine;
    QString error;
    const std::unique_ptr<QObject> host = m_home.create(engine, ScriptHost.toByteArray(), &error);
    QVERIFY2(host, qPrintable(error));
    const QVariant result = script(host.get(), library, function, fromJson(arguments).toList());
    QCOMPARE(QJsonDocument(QJsonArray {QJsonValue::fromVariant(result)}).toJson(QJsonDocument::Compact),
        QJsonDocument(QJsonArray {QJsonValue::fromVariant(fromJson(expected))}).toJson(QJsonDocument::Compact));
}

void TestSettingsCatalogQml::everyActionLoads()
{
    m_home.resetConfig(QStringLiteral("binds {\n}\n"));
    QQmlEngine engine;
    const std::unique_ptr<QObject> host = m_home.create(engine,
        "import \"catalog/Actions.js\" as Actions\n"
        "QtObject {\n"
        "    readonly property var samples: ({ none: [], index: [2], workspace: [\"mail\"], size: [\"+10%\"], monitor: [\"DP-1\"],\n"
        "        display: [\"tabbed\"], command: [\"kitty\", \"--hold\"], shell: [\"echo hi\"], text: [\"notes\"] })\n"
        "    function check() {\n"
        "        const failed = [];\n"
        "        for (const action of Actions.actions) {\n"
        "            const node = { name: action.id, args: samples[action.argument], props: action.followProperty ? { focus: false } : {} "
        "};\n"
        "            SettingsStore.setNode(\"binds/Mod+F9\", { name: \"Mod+F9\", args: [], props: {}, children: [node] });\n"
        "            if (SettingsStore.configError.length || !samples[action.argument] || Actions.describe(node) === action.id) {\n"
        "                failed.push(action.id + \": \" + SettingsStore.configError);\n"
        "            }\n"
        "        }\n"
        "        return [Actions.actions.length].concat(failed);\n"
        "    }\n"
        "}\n");
    QVERIFY(host);
    QVariant result;
    QVERIFY(QMetaObject::invokeMethod(host.get(), "check", Q_RETURN_ARG(QVariant, result)));
    const QVariantList list = result.toList();
    QVERIFY(list.first().toInt() > 100);
    QCOMPARE(list.mid(1), QVariantList {});
}

QTEST_MAIN(TestSettingsCatalogQml)
#include "test_settings_catalog_qml.moc"
