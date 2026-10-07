#include "scriptprobe.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <QJsonValue>
#include <QTest>

namespace
{

const QByteArray probe
    = "import QtQml\n"
      "import \"file://" KONVEYOR_SOURCE_DIR "/widgets/taskbar/plasmoids/org.devl0rd.taskbar/contents/ui/MenuModel.js\" as MenuModel\n"
      "\n"
      "QtObject {\n"
      "    function tr(text, ...args) {\n"
      "        return args.reduce((result, value, index) => result.replace(\"%\" + (index + 1), value), text)\n"
      "    }\n"
      "    function entries(context) {\n"
      "        return JSON.stringify(MenuModel.entries(JSON.parse(context), tr))\n"
      "    }\n"
      "}\n";

QJsonObject window(const char *title, bool active = false)
{
    return {{QStringLiteral("title"), QLatin1String(title)}, {QStringLiteral("appName"), QStringLiteral("App")},
        {QStringLiteral("active"), active}, {QStringLiteral("konveyorId"), 7}, {QStringLiteral("onAllDesktops"), false},
        {QStringLiteral("minimized"), false}};
}

QJsonObject rules(const QJsonObject &set = {})
{
    QJsonObject remembered {{QStringLiteral("float"), false}, {QStringLiteral("all-workspaces"), false}, {QStringLiteral("stack"), false},
        {QStringLiteral("row"), false}, {QStringLiteral("column"), QJsonValue()}, {QStringLiteral("workspace"), QJsonValue()},
        {QStringLiteral("monitor"), QJsonValue()}, {QStringLiteral("size"), QJsonValue()}};
    for (auto it = set.begin(); it != set.end(); ++it) {
        remembered.insert(it.key(), it.value());
    }
    return {{QStringLiteral("app_id"), QStringLiteral("app")}, {QStringLiteral("output"), QStringLiteral("DP-1")},
        {QStringLiteral("workspace"), 1}, {QStringLiteral("workspace_name"), QJsonValue()}, {QStringLiteral("column"), 2},
        {QStringLiteral("rules"), remembered}};
}

QJsonObject context(const QJsonObject &changes = {})
{
    QJsonObject result {
        {QStringLiteral("entry"),
            QJsonObject {{QStringLiteral("kind"), QStringLiteral("column")}, {QStringLiteral("pinned"), false},
                {QStringLiteral("windows"), QJsonArray {window("Editor", true)}}}},
        {QStringLiteral("window"), window("Editor", true)},
        {QStringLiteral("konveyor"), true},
        {QStringLiteral("column"), QJsonObject {{QStringLiteral("index"), 2}, {QStringLiteral("count"), 3}, {QStringLiteral("rows"), 1}}},
        {QStringLiteral("tabbed"), false},
        {QStringLiteral("floating"), false},
        {QStringLiteral("workspaces"),
            QJsonArray {QJsonObject {{QStringLiteral("idx"), 1}, {QStringLiteral("name"), QString()}, {QStringLiteral("current"), true}},
                QJsonObject {
                    {QStringLiteral("idx"), 2}, {QStringLiteral("name"), QStringLiteral("web")}, {QStringLiteral("current"), false}}}},
        {QStringLiteral("outputs"), QJsonArray {QStringLiteral("DP-1"), QStringLiteral("DP-2")}},
        {QStringLiteral("output"), QStringLiteral("DP-1")},
        {QStringLiteral("panelOutput"), QStringLiteral("DP-1")},
        {QStringLiteral("rules"), rules()},
        {QStringLiteral("appName"), QStringLiteral("App")},
    };
    for (auto it = changes.begin(); it != changes.end(); ++it) {
        result.insert(it.key(), it.value());
    }
    return result;
}

QJsonObject find(const QJsonArray &entries, const QString &text)
{
    for (const QJsonValue &entry : entries) {
        if (entry[QStringLiteral("text")].toString() == text) {
            return entry.toObject();
        }
        const QJsonObject inner = find(entry[QStringLiteral("children")].toArray(), text);
        if (!inner.isEmpty()) {
            return inner;
        }
    }
    return {};
}

}

class TestPlasmoidTaskbarMenu : public QObject
{
    Q_OBJECT

    ScriptProbe m_probe;

    QJsonArray entries(const QJsonObject &context)
    {
        QVariant result;
        QMetaObject::invokeMethod(m_probe.get(), "entries", Q_RETURN_ARG(QVariant, result),
            Q_ARG(QVariant, QString::fromUtf8(QJsonDocument(context).toJson(QJsonDocument::Compact))));
        return QJsonDocument::fromJson(result.toString().toUtf8()).array();
    }

private Q_SLOTS:
    void initTestCase()
    {
        const QString error = m_probe.load(probe);
        QVERIFY2(error.isEmpty(), qPrintable(error));
    }

    void idlePinsOnlyOpenAndUnpin()
    {
        const QJsonObject pin {
            {QStringLiteral("kind"), QStringLiteral("pin")}, {QStringLiteral("pinned"), true}, {QStringLiteral("windows"), QJsonArray()}};
        const QJsonArray menu = entries(context({{QStringLiteral("entry"), pin}, {QStringLiteral("window"), QJsonValue()}}));
        QCOMPARE(menu.size(), 2);
        QCOMPARE(menu[0][QStringLiteral("text")].toString(), QStringLiteral("Unpin from Taskbar"));
        QCOMPARE(menu[1][QStringLiteral("action")][QStringLiteral("task")].toString(), QStringLiteral("newInstance"));
    }

    void layoutActionsTargetTheWindow()
    {
        const QJsonArray menu = entries(context());
        QCOMPARE(find(menu, QStringLiteral("Konveyor"))[QStringLiteral("section")].toBool(), true);
        QCOMPARE(find(menu, QStringLiteral("Wider"))[QStringLiteral("action")],
            (QJsonObject {{QStringLiteral("konveyor"), QStringLiteral("set-column-width")},
                {QStringLiteral("args"), QJsonArray {QStringLiteral("+10%")}}, {QStringLiteral("focus"), false}}));
        QCOMPARE(find(menu, QStringLiteral("Join the Column on the Left"))[QStringLiteral("action")][QStringLiteral("konveyor")].toString(),
            QStringLiteral("consume-or-expel-window-left"));
        QVERIFY(find(menu, QStringLiteral("Move Out to Its Own Column")).isEmpty());
        QCOMPARE(find(menu, QStringLiteral("Move Left"))[QStringLiteral("enabled")].toBool(), true);
        QCOMPARE(find(menu, QStringLiteral("web")) [QStringLiteral("action")][QStringLiteral("args")], QJsonArray { 2 });
        QCOMPARE(find(menu, QStringLiteral("Workspace 1"))[QStringLiteral("enabled")].toBool(), false);
        QCOMPARE(find(menu, QStringLiteral("DP-2"))[QStringLiteral("action")][QStringLiteral("konveyor")].toString(),
            QStringLiteral("move-window-to-monitor"));
        QVERIFY(find(menu, QStringLiteral("Bring to This Screen")).isEmpty());
        QCOMPARE(menu.last()[QStringLiteral("action")][QStringLiteral("task")].toString(), QStringLiteral("close"));
    }

    void sharedColumnsOfferToMoveOut()
    {
        const QJsonArray menu = entries(context({{QStringLiteral("tabbed"), true},
            {QStringLiteral("column"),
                QJsonObject {{QStringLiteral("index"), 1}, {QStringLiteral("count"), 1}, {QStringLiteral("rows"), 2}}}}));
        QCOMPARE(find(menu, QStringLiteral("Move Out to Its Own Column"))[QStringLiteral("action")][QStringLiteral("konveyor")].toString(),
            QStringLiteral("consume-or-expel-window-right"));
        QCOMPARE(find(menu, QStringLiteral("Show Column as Tabs"))[QStringLiteral("checked")].toBool(), true);
        QCOMPARE(find(menu, QStringLiteral("Move Left"))[QStringLiteral("enabled")].toBool(), false);
        QCOMPARE(find(menu, QStringLiteral("Move to the End"))[QStringLiteral("enabled")].toBool(), false);
    }

    void floatingWindowsHaveNoColumnMoves()
    {
        const QJsonArray menu = entries(context({{QStringLiteral("column"), QJsonValue()}, {QStringLiteral("floating"), true},
            {QStringLiteral("panelOutput"), QStringLiteral("DP-2")}}));
        QVERIFY(find(menu, QStringLiteral("Move Left")).isEmpty());
        QVERIFY(find(menu, QStringLiteral("Show Column as Tabs")).isEmpty());
        QCOMPARE(find(menu, QStringLiteral("Float"))[QStringLiteral("checked")].toBool(), true);
        QCOMPARE(
            find(menu, QStringLiteral("Bring to This Screen")) [QStringLiteral("action")][QStringLiteral("args")],
            QJsonArray { QStringLiteral("DP-2") });
    }

    void rememberedRulesShowTheirState()
    {
        const QJsonArray fresh = entries(context());
        QCOMPARE(find(fresh, QStringLiteral("Always Float"))[QStringLiteral("action")],
            (QJsonObject {{QStringLiteral("rule"), QStringLiteral("float")}, {QStringLiteral("enabled"), true}}));
        QCOMPARE(find(fresh, QStringLiteral("Always Open as Column 2"))[QStringLiteral("checked")].toBool(), false);
        QCOMPARE(find(fresh, QStringLiteral("Edit Rules for App…"))[QStringLiteral("action")][QStringLiteral("editRules")].toBool(), true);
        const QJsonArray set = entries(context({{QStringLiteral("rules"),
            rules({{QStringLiteral("float"), true}, {QStringLiteral("column"), 4}, {QStringLiteral("workspace"), QStringLiteral("web")},
                {QStringLiteral("size"), QJsonObject {{QStringLiteral("width"), 800}}}})}}));
        const QJsonObject floating = find(set, QStringLiteral("Always Float"));
        QCOMPARE(floating[QStringLiteral("checked")].toBool(), true);
        QCOMPARE(floating[QStringLiteral("action")][QStringLiteral("enabled")].toBool(), false);
        QCOMPARE(find(set, QStringLiteral("Always Open as Column 4"))[QStringLiteral("checked")].toBool(), true);
        QCOMPARE(find(set, QStringLiteral("Always Open on Workspace web"))[QStringLiteral("checked")].toBool(), true);
        QCOMPARE(find(set, QStringLiteral("Open at This Size on DP-1"))[QStringLiteral("checked")].toBool(), true);
        QVERIFY(find(entries(context({{QStringLiteral("rules"), QJsonValue()}})), QStringLiteral("Remember for App")).isEmpty());
    }

    void groupsListTheirWindowsFirst()
    {
        const QJsonObject group {{QStringLiteral("kind"), QStringLiteral("group")}, {QStringLiteral("pinned"), true},
            {QStringLiteral("windows"), QJsonArray {window("One"), window("Two", true)}}};
        const QJsonArray menu = entries(context({{QStringLiteral("entry"), group}}));
        QCOMPARE(menu[0][QStringLiteral("text")].toString(), QStringLiteral("One"));
        QCOMPARE(menu[0][QStringLiteral("action")][QStringLiteral("task")].toString(), QStringLiteral("activate"));
        QCOMPARE(menu[1][QStringLiteral("checked")].toBool(), true);
        QVERIFY(!find(menu, QStringLiteral("Close All")).isEmpty());
        QCOMPARE(find(menu, QStringLiteral("To the Right of This One"))[QStringLiteral("action")][QStringLiteral("task")].toString(),
            QStringLiteral("openRight"));
    }

    void layoutActionsShowTheirBinds()
    {
        const auto bind = [](const char *key, const char *name, const QJsonArray &arguments = {}) {
            return QJsonObject {{QStringLiteral("key"), QLatin1String(key)},
                {QStringLiteral("action"),
                    QJsonObject {{QStringLiteral("name"), QLatin1String(name)}, {QStringLiteral("arguments"), arguments}}}};
        };
        const QJsonArray binds {bind("Super+F", "maximize-column"), bind("Super+Ctrl+Left", "move-column-left"),
            bind("Super+Equal", "set-column-width", {QStringLiteral("+10%")}),
            bind("Super+Minus", "set-column-width", {QStringLiteral("-10%")}), bind("Super+Q", "close-window"),
            bind("Super+BracketLeft", "consume-or-expel-window-left")};
        const QJsonArray menu = entries(context({{QStringLiteral("binds"), binds}}));
        QCOMPARE(find(menu, QStringLiteral("Full Width"))[QStringLiteral("hint")].toString(), QStringLiteral("Meta+F"));
        QCOMPARE(find(menu, QStringLiteral("Move Left"))[QStringLiteral("hint")].toString(), QStringLiteral("Meta+Ctrl+Left"));
        QCOMPARE(find(menu, QStringLiteral("Wider"))[QStringLiteral("hint")].toString(), QStringLiteral("Meta+="));
        QCOMPARE(find(menu, QStringLiteral("Narrower"))[QStringLiteral("hint")].toString(), QStringLiteral("Meta+-"));
        QCOMPARE(find(menu, QStringLiteral("Join the Column on the Left"))[QStringLiteral("hint")].toString(), QStringLiteral("Meta+["));
        QCOMPARE(find(menu, QStringLiteral("Close"))[QStringLiteral("hint")].toString(), QStringLiteral("Meta+Q"));
        QCOMPARE(find(menu, QStringLiteral("Maximize"))[QStringLiteral("hint")].toString(), QString());
        const QJsonArray rebound = entries(context({{QStringLiteral("binds"), QJsonArray {bind("Alt+Ctrl+M", "maximize-column")}}}));
        QCOMPARE(find(rebound, QStringLiteral("Full Width"))[QStringLiteral("hint")].toString(), QStringLiteral("Alt+Ctrl+M"));
        QCOMPARE(find(rebound, QStringLiteral("Move Left"))[QStringLiteral("hint")].toString(), QString());
    }

    void appActionsComeFirst()
    {
        const auto item = [](const char *text, const char *id, const char *argument) {
            return QJsonObject {{QStringLiteral("text"), QLatin1String(text)}, {QStringLiteral("icon"), QStringLiteral("firefox")},
                {QStringLiteral("actionId"), QLatin1String(id)}, {QStringLiteral("actionArgument"), QLatin1String(argument)}};
        };
        const QJsonArray actions {item("New Private Window", "_kicker_jumpListAction", "private"),
            item("notes.txt", "_kicker_recentDocument", "file:///notes.txt"),
            item("Forget Recent Files", "_kicker_forgetRecentDocuments", ""), item("Edit Application…", "editApplication", "")};
        const QJsonObject player {{QStringLiteral("playing"), true}, {QStringLiteral("canControl"), true},
            {QStringLiteral("canGoNext"), true}, {QStringLiteral("canGoPrevious"), false},
            {QStringLiteral("track"), QStringLiteral("Song")}};
        const QJsonArray menu = entries(context({{QStringLiteral("appActions"), actions}, {QStringLiteral("player"), player}}));
        QCOMPARE(menu[0][QStringLiteral("text")].toString(), QStringLiteral("New Private Window"));
        QCOMPARE(menu[0][QStringLiteral("action")][QStringLiteral("appAction")][QStringLiteral("actionArgument")].toString(),
            QStringLiteral("private"));
        QCOMPARE(menu[1][QStringLiteral("text")].toString(), QStringLiteral("Recent Files"));
        QCOMPARE(menu[1][QStringLiteral("children")].toArray().size(), 3);
        QVERIFY(find(menu, QStringLiteral("Edit Application…")).isEmpty());
        QCOMPARE(menu[2][QStringLiteral("text")].toString(), QStringLiteral("Song"));
        QCOMPARE(
            find(menu, QStringLiteral("Pause"))[QStringLiteral("action")][QStringLiteral("media")].toString(), QStringLiteral("PlayPause"));
        QCOMPARE(find(menu, QStringLiteral("Previous Track"))[QStringLiteral("enabled")].toBool(), false);
        QCOMPARE(menu[6][QStringLiteral("separator")].toBool(), true);
        QCOMPARE(find(menu, QStringLiteral("Keep Above Others"))[QStringLiteral("action")][QStringLiteral("task")].toString(),
            QStringLiteral("toggleKeepAbove"));
        QCOMPARE(menu.last()[QStringLiteral("text")].toString(), QStringLiteral("Close"));
    }

    void withoutKonveyorTheMenuStaysPlain()
    {
        const QJsonArray menu = entries(context({{QStringLiteral("konveyor"), false}}));
        QVERIFY(find(menu, QStringLiteral("Konveyor")).isEmpty());
        QCOMPARE(find(menu, QStringLiteral("Open New Window"))[QStringLiteral("action")][QStringLiteral("task")].toString(),
            QStringLiteral("newInstance"));
        QCOMPARE(find(menu, QStringLiteral("Minimize"))[QStringLiteral("action")][QStringLiteral("task")].toString(),
            QStringLiteral("toggleMinimized"));
    }
};

QTEST_GUILESS_MAIN(TestPlasmoidTaskbarMenu)

#include "test_plasmoid_taskbar_menu.moc"
