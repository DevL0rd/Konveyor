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
      "import QtQml.Models\n"
      "import \"file://" KONVEYOR_SOURCE_DIR "/widgets/taskbar/plasmoids/org.devl0rd.taskbar/contents/ui/TaskOrder.js\" as TaskOrder\n"
      "\n"
      "QtObject {\n"
      "    property ListModel model: ListModel {}\n"
      "    function run(name, args) {\n"
      "        return JSON.stringify(TaskOrder[name].apply(null, JSON.parse(args)))\n"
      "    }\n"
      "    function sync(keys) {\n"
      "        TaskOrder.syncKeys(model, keys)\n"
      "        const result = []\n"
      "        for (let i = 0; i < model.count; ++i)\n"
      "            result.push(model.get(i).key)\n"
      "        return result.join(\",\")\n"
      "    }\n"
      "}\n";

QJsonObject row(const char *uuid, const char *app)
{
    return {{QStringLiteral("uuid"), QLatin1String(uuid)}, {QStringLiteral("appKey"), QLatin1String(app)}};
}

QJsonObject workspace(int id, const QJsonArray &columns, bool focused, const QJsonArray &displays = {})
{
    return {{QStringLiteral("id"), id}, {QStringLiteral("columns"), columns}, {QStringLiteral("focused"), focused},
        {QStringLiteral("displays"), displays}};
}

QJsonArray keysOf(const QJsonArray &items)
{
    QJsonArray keys;
    for (const QJsonValue &item : items) {
        keys.append(item[QStringLiteral("key")]);
    }
    return keys;
}

}

class TestPlasmoidTaskbar : public QObject
{
    Q_OBJECT

    ScriptProbe m_probe;

    QJsonValue call(const char *name, const QJsonArray &arguments)
    {
        QVariant result;
        const QString text = QString::fromUtf8(QJsonDocument(arguments).toJson(QJsonDocument::Compact));
        QMetaObject::invokeMethod(
            m_probe.get(), "run", Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, QLatin1String(name)), Q_ARG(QVariant, text));
        return QJsonDocument::fromJson(QByteArray("[") + result.toString().toUtf8() + QByteArray("]")).array().at(0);
    }

    QJsonArray buildOn(
        const QJsonArray &workspaces, const QJsonArray &rows, const QJsonArray &pins, bool merge, const QJsonObject &parked = {})
    {
        QJsonObject windows;
        for (const QJsonValue &workspace : workspaces) {
            for (const QJsonValue &column : workspace[QStringLiteral("columns")].toArray()) {
                for (const QJsonValue &id : column.toArray()) {
                    windows[QString::number(id.toInt())] = QJsonObject {{QStringLiteral("uuid"), QStringLiteral("u%1").arg(id.toInt())}};
                }
            }
        }
        return call("buildItems",
            {QJsonObject {{QStringLiteral("workspaces"), workspaces}, {QStringLiteral("windowsById"), windows},
                {QStringLiteral("rows"), rows}, {QStringLiteral("pins"), pins}, {QStringLiteral("launchers"), QJsonObject()},
                {QStringLiteral("merge"), merge}, {QStringLiteral("parkedAfter"), parked}}})
            .toArray();
    }

    QJsonArray build(const QJsonArray &columns, const QJsonArray &rows, const QJsonArray &pins, bool merge, const QJsonObject &parked = {},
        const QJsonArray &displays = {})
    {
        return buildOn({workspace(1, columns, true, displays)}, rows, pins, merge, parked);
    }

private Q_SLOTS:
    void initTestCase()
    {
        const QString error = m_probe.load(probe);
        QVERIFY2(error.isEmpty(), qPrintable(error));
    }

    void itemsFollowTheColumnOrder()
    {
        const QJsonArray items = build({QJsonArray {4}, QJsonArray {2, 3}, QJsonArray {1}},
            {row("u1", "a"), row("u2", "b"), row("u3", "b"), row("u4", "c"), row("u9", "d")}, {}, false);
        QCOMPARE(keysOf(items), (QJsonArray {QStringLiteral("wu4"), QStringLiteral("wu2"), QStringLiteral("wu1"), QStringLiteral("wu9")}));
        QCOMPARE(items[1][QStringLiteral("windows")].toArray().size(), 2);
        QCOMPARE(items[1][QStringLiteral("windows")][1][QStringLiteral("konveyorId")].toInt(), 3);
        QCOMPARE(items[2][QStringLiteral("columns")],
            (QJsonArray {QJsonObject {{QStringLiteral("index"), 3}, {QStringLiteral("id"), 1}, {QStringLiteral("focused"), true}}}));
        QCOMPARE(items[1][QStringLiteral("kind")].toString(), QStringLiteral("column"));
        QCOMPARE(items[3][QStringLiteral("kind")].toString(), QStringLiteral("window"));
        QCOMPARE(items[3][QStringLiteral("columns")], QJsonArray());
    }

    void mergingJoinsNeighbouringColumnsOfOneApp()
    {
        const QJsonArray rows {row("u1", "a"), row("u2", "a"), row("u3", "b"), row("u4", "a")};
        const QJsonArray columns {QJsonArray {1}, QJsonArray {2}, QJsonArray {3}, QJsonArray {4}};
        QCOMPARE(keysOf(build(columns, rows, {}, true)), (QJsonArray {QStringLiteral("wu1"), QStringLiteral("wu3"), QStringLiteral("wu4")}));
        QCOMPARE(build(columns, rows, {}, true)[0][QStringLiteral("columns")].toArray().size(), 2);
        QCOMPARE(keysOf(build(columns, rows, {}, false)).size(), 4);
    }

    void sharedColumnsKeepEveryWindow()
    {
        const QJsonArray rows {row("u1", "a"), row("u2", "a"), row("u3", "a")};
        const QJsonArray items = build({QJsonArray {1, 2}, QJsonArray {3}}, rows, {}, true);
        QCOMPARE(keysOf(items), (QJsonArray {QStringLiteral("wu1"), QStringLiteral("wu3")}));
        QCOMPARE(items[0][QStringLiteral("kind")].toString(), QStringLiteral("column"));
        QCOMPARE(items[0][QStringLiteral("windows")].toArray().size(), 2);
        QCOMPARE(items[0][QStringLiteral("windows")][1][QStringLiteral("uuid")].toString(), QStringLiteral("u2"));
        QCOMPARE(items[1][QStringLiteral("windows")].toArray().size(), 1);
    }

    void tabbedColumnsAreMarked()
    {
        const QJsonArray items = build({QJsonArray {1, 2}, QJsonArray {3}}, {row("u1", "a"), row("u2", "b"), row("u3", "c")}, {}, false, {},
            {QStringLiteral("tabbed"), QStringLiteral("normal")});
        QCOMPARE(items[0][QStringLiteral("tabbed")].toBool(), true);
        QCOMPARE(items[1][QStringLiteral("tabbed")].toBool(), false);
    }

    void appGroupsOnlyJoinSingleWindowColumns()
    {
        const QJsonArray rows {row("u1", "a"), row("u2", "a"), row("u3", "a"), row("u4", "a"), row("u5", "a")};
        const QJsonArray items = build({QJsonArray {1}, QJsonArray {2}, QJsonArray {3, 4}, QJsonArray {5}}, rows, {}, true);
        QCOMPARE(keysOf(items), (QJsonArray {QStringLiteral("wu1"), QStringLiteral("wu3"), QStringLiteral("wu5")}));
        QCOMPARE(items[0][QStringLiteral("kind")].toString(), QStringLiteral("group"));
        QCOMPARE(items[0][QStringLiteral("columns")].toArray().size(), 2);
        QCOMPARE(items[1][QStringLiteral("kind")].toString(), QStringLiteral("column"));
        QCOMPARE(items[2][QStringLiteral("kind")].toString(), QStringLiteral("column"));
        QCOMPARE(call("columnIds", {QJsonValue(items)}), (QJsonArray {1, 2, 3, 5}));
    }

    void badgesOnlyNameColumnsOfTheFocusedWorkspace()
    {
        const QJsonArray items
            = buildOn({workspace(1, {QJsonArray {1}, QJsonArray {2}}, false), workspace(2, {QJsonValue(QJsonArray {3})}, true)},
                {row("u1", "a"), row("u2", "b"), row("u3", "c")}, {}, false);
        QCOMPARE(keysOf(items), (QJsonArray {QStringLiteral("wu1"), QStringLiteral("wu2"), QStringLiteral("wu3")}));
        const QJsonObject labels {{QStringLiteral("1"), QStringLiteral("1")}, {QStringLiteral("2"), QStringLiteral("2")}};
        QCOMPARE(call("columnBadge", {items[0], labels}).toString(), QString());
        QCOMPARE(call("columnBadge", {items[2], labels}).toString(), QStringLiteral("1"));
    }

    void clicksPickTheRightWindow()
    {
        const auto window = [](const char *uuid, bool active, int last) {
            return QJsonObject {{QStringLiteral("uuid"), QLatin1String(uuid)}, {QStringLiteral("active"), active},
                {QStringLiteral("lastActivated"), last}, {QStringLiteral("appKey"), QStringLiteral("a")}};
        };
        const QJsonObject idle = window("1", false, 5);
        const QJsonObject recent = window("2", false, 9);
        const QJsonObject focused = window("3", true, 1);
        const auto result = [this](const QJsonArray &windows, const QJsonValue &clicked, int mode) {
            const QJsonValue value = call("clickResult", {windows, clicked, mode});
            return value[QStringLiteral("action")].toString() + QLatin1Char(':')
                + value[QStringLiteral("window")][QStringLiteral("uuid")].toString();
        };
        QCOMPARE(result({}, QJsonValue(), 0), QStringLiteral("launch:"));
        QCOMPARE(result({idle, focused}, idle, 0), QStringLiteral("activate:1"));
        QCOMPARE(result({focused}, focused, 0), QStringLiteral("minimize:3"));
        QCOMPARE(result({focused}, focused, 1), QStringLiteral("none:"));
        QCOMPARE(result({focused}, focused, 2), QStringLiteral("cycle:3"));
        QCOMPARE(result({idle, recent}, QJsonValue(), 0), QStringLiteral("activate:2"));
        QCOMPARE(result({idle, focused, recent}, QJsonValue(), 0), QStringLiteral("activate:2"));
        QCOMPARE(result({idle, focused}, QJsonValue(), 1), QStringLiteral("none:"));
        const QJsonObject other
            = QJsonObject {{QStringLiteral("uuid"), QStringLiteral("4")}, {QStringLiteral("appKey"), QStringLiteral("b")}};
        QCOMPARE(call("nextOfApp", {QJsonArray {idle, other, focused}, focused})[QStringLiteral("uuid")].toString(), QStringLiteral("1"));
        QVERIFY(call("nextOfApp", {QJsonArray {other, focused}, focused}).isNull());
        QCOMPARE(call("steppedWindow", {QJsonArray {idle, focused, other}, 1})[QStringLiteral("uuid")].toString(), QStringLiteral("4"));
        QCOMPARE(call("steppedWindow", {QJsonArray {idle, focused, other}, -1})[QStringLiteral("uuid")].toString(), QStringLiteral("1"));
        QCOMPARE(call("steppedWindow", {QJsonArray {focused, other}, 1})[QStringLiteral("uuid")].toString(), QStringLiteral("4"));
        QCOMPARE(call("steppedWindow", {QJsonArray {other, focused}, 1})[QStringLiteral("uuid")].toString(), QStringLiteral("4"));
        QVERIFY(call("steppedWindow", {QJsonArray(), 1}).isNull());
    }

    void emptyWorkspacesCanBeHidden()
    {
        const QJsonArray workspaces {QJsonObject {{QStringLiteral("idx"), 1}, {QStringLiteral("columns"), QJsonArray {QJsonArray {1}}}},
            QJsonObject {{QStringLiteral("idx"), 2}, {QStringLiteral("columns"), QJsonArray()}, {QStringLiteral("is_active"), true}},
            QJsonObject {{QStringLiteral("idx"), 3}, {QStringLiteral("columns"), QJsonArray()}}};
        QCOMPARE(call("shownWorkspaces", {workspaces, true}).toArray().size(), 3);
        const QJsonArray shown = call("shownWorkspaces", {workspaces, false}).toArray();
        QCOMPARE(shown.size(), 2);
        QCOMPARE(shown[1][QStringLiteral("idx")].toInt(), 2);
    }

    void pinsAreNormalised()
    {
        QCOMPARE(call("pinUrl", {QStringLiteral(" org.kde.dolphin ")}).toString(), QStringLiteral("applications:org.kde.dolphin.desktop"));
        QCOMPARE(call("pinUrl", {QStringLiteral("firefox.desktop")}).toString(), QStringLiteral("applications:firefox.desktop"));
        QCOMPARE(call("pinUrl", {QStringLiteral("file:///opt/x.desktop")}).toString(), QStringLiteral("file:///opt/x.desktop"));
        QCOMPARE(call("pinUrl", {QStringLiteral("  ")}).toString(), QString());
        QCOMPARE(call("pinLabel", {QStringLiteral("applications:org.kde.dolphin.desktop")}).toString(), QStringLiteral("org.kde.dolphin"));
        QCOMPARE(call("pinLabel", {QStringLiteral("file:///opt/x.desktop")}).toString(), QStringLiteral("x"));
        const QJsonArray pins {QStringLiteral("a"), QStringLiteral("b"), QStringLiteral("c")};
        QCOMPARE(call("movedPin", {pins, 0, 1}), (QJsonArray {QStringLiteral("b"), QStringLiteral("a"), QStringLiteral("c")}));
        QCOMPARE(call("movedPin", {pins, 2, -1}), (QJsonArray {QStringLiteral("a"), QStringLiteral("c"), QStringLiteral("b")}));
        QCOMPARE(call("movedPin", {pins, 0, -1}), pins);
    }

    void minimizedWindowsKeepTheirSlot()
    {
        const QJsonArray rows {row("u1", "a"), row("u2", "b"), row("m1", "c"), row("m2", "d"), row("m3", "e")};
        const QJsonObject parked {{QStringLiteral("m1"), QStringLiteral("u1")}, {QStringLiteral("m2"), QJsonValue()}, {QStringLiteral("m3"), QStringLiteral("gone")}};
        QCOMPARE(keysOf(build({QJsonArray {1}, QJsonArray {2}}, rows, {}, false, parked)),
            (QJsonArray {QStringLiteral("wm2"), QStringLiteral("wu1"), QStringLiteral("wm1"), QStringLiteral("wu2"), QStringLiteral("wm3")}));
    }

    void minimizingKeepsEveryItemInPlace()
    {
        const QJsonArray rows {row("u1", "a"), row("m1", "b"), row("u2", "d")};
        const QJsonArray pins {QStringLiteral("a"), QStringLiteral("b"), QStringLiteral("c")};
        const QJsonArray before = build({QJsonArray {1}, QJsonArray {2}}, {row("u1", "a"), row("u2", "d"), row("m1", "b")}, pins, false,
            {{QStringLiteral("m1"), QStringLiteral("u1")}});
        QCOMPARE(keysOf(before), (QJsonArray {QStringLiteral("wu1"), QStringLiteral("wm1"), QStringLiteral("pc"), QStringLiteral("wu2")}));
        const QJsonArray minimized = build(QJsonArray {QJsonArray {1}}, rows, pins, false,
            {{QStringLiteral("m1"), QStringLiteral("u1")}, {QStringLiteral("u2"), QStringLiteral("u1")}});
        QCOMPARE(keysOf(minimized), keysOf(before));
    }

    void parkedWindowsFollowAParkedAnchor()
    {
        const QJsonArray rows {row("u1", "a"), row("m1", "b"), row("m2", "c"), row("u3", "d")};
        const QJsonObject parked {{QStringLiteral("m2"), QStringLiteral("m1")}, {QStringLiteral("m1"), QStringLiteral("u1")}};
        QCOMPARE(keysOf(build({QJsonArray {1}, QJsonArray {3}}, rows, {}, false, parked)),
            (QJsonArray {QStringLiteral("wu1"), QStringLiteral("wm1"), QStringLiteral("wm2"), QStringLiteral("wu3")}));
    }

    void idlePinsSitAfterTheirPinnedNeighbour()
    {
        const QJsonArray items = build({QJsonArray {1}, QJsonArray {2}}, {row("u1", "x"), row("u2", "b")},
            {QStringLiteral("p1"), QStringLiteral("b"), QStringLiteral("p2"), QStringLiteral("p3")}, false);
        QCOMPARE(keysOf(items),
            (QJsonArray {QStringLiteral("pp1"), QStringLiteral("wu1"), QStringLiteral("wu2"), QStringLiteral("pp2"), QStringLiteral("pp3")}));
        QCOMPARE(items[2][QStringLiteral("pinned")].toBool(), true);
        QCOMPARE(items[1][QStringLiteral("pinned")].toBool(), false);
        QCOMPARE(items[3][QStringLiteral("launcher")][QStringLiteral("url")].toString(), QStringLiteral("p2"));
    }

    void planMovesReachesTheWantedOrder()
    {
        QCOMPARE(call("planMoves", {QJsonArray {1, 2, 3}, QJsonArray {3, 1, 2}}),
            (QJsonArray {QJsonObject {{QStringLiteral("id"), 3}, {QStringLiteral("index"), 1}}}));
        QCOMPARE(call("planMoves", {QJsonArray {1, 2, 3}, QJsonArray {1, 2, 3}}), QJsonArray());
        QCOMPARE(call("planMoves", {QJsonArray {1, 2, 3, 4}, QJsonArray {2, 1, 4, 3}}),
            (QJsonArray {QJsonObject {{QStringLiteral("id"), 2}, {QStringLiteral("index"), 1}},
                QJsonObject {{QStringLiteral("id"), 4}, {QStringLiteral("index"), 3}}}));
    }

    void reorderingKeepsColumnsAndPinsInStep()
    {
        const QJsonArray items
            = build({QJsonArray {1}, QJsonArray {2}}, {row("u1", "a"), row("u2", "b")}, {QStringLiteral("b"), QStringLiteral("c")}, false);
        const QJsonArray moved = call("reorder", {items, 1, 0}).toArray();
        QCOMPARE(call("columnIds", {QJsonValue(moved)}), (QJsonArray {2, 1}));
        QCOMPARE(call("pinOrder", {call("reorder", {items, 2, 0}), QJsonArray {QStringLiteral("b"), QStringLiteral("c")}}),
            (QJsonArray {QStringLiteral("c"), QStringLiteral("b")}));
    }

    void pinnedLaunchesFollowThePinOrder()
    {
        const QJsonArray pins {QStringLiteral("A"), QStringLiteral("B"), QStringLiteral("C")};
        QCOMPARE(call("pinPlacement", {QJsonArray {QStringLiteral("C")}, 0, pins}).toInt(), 0);
        QCOMPARE(call("pinPlacement", {QJsonArray {QStringLiteral("C"), QStringLiteral("A")}, 1, pins}).toInt(), 1);
        QCOMPARE(call("pinPlacement", {QJsonArray {QStringLiteral("A"), QStringLiteral("C"), QStringLiteral("B")}, 2, pins}).toInt(), 2);
        QCOMPARE(call("pinPlacement", {QJsonArray {QStringLiteral("A"), QStringLiteral("x"), QStringLiteral("C")}, 2, pins}).toInt(), 2);
        QCOMPARE(call("pinPlacement", {QJsonArray {QStringLiteral("A"), QStringLiteral("A")}, 1, pins}).toInt(), 0);
        QCOMPARE(call("pinPlacement", {QJsonArray {QStringLiteral("A"), QStringLiteral("x")}, 1, pins}).toInt(), 0);
    }

    void shortcutLabelsComeFromTheBinds()
    {
        const auto bind = [](const char *key, const char *action, const QJsonArray &arguments) {
            return QJsonObject {{QStringLiteral("key"), QLatin1String(key)},
                {QStringLiteral("action"),
                    QJsonObject {{QStringLiteral("name"), QLatin1String(action)}, {QStringLiteral("arguments"), arguments}}}};
        };
        const QJsonValue labels = call("shortcutLabels",
            {QJsonValue(QJsonArray {bind("Super+1", "focus-workspace", {QStringLiteral("1")}),
                bind("Super+Ctrl+2", "focus-column", {QStringLiteral("2")}), bind("Super+Alt+3", "focus-column", {QStringLiteral("3")}),
                bind("Alt+Super+4", "focus-column", {QStringLiteral("4")}), bind("Super+W", "focus-workspace", {QStringLiteral("web")}),
                bind("Super+F1", "focus-workspace", {QStringLiteral("1")}), bind("Super+K", "show-hotkey-overlay", {})})});
        QCOMPARE(labels[QStringLiteral("workspaces")], (QJsonObject {{QStringLiteral("1"), QStringLiteral("1")}}));
        QCOMPARE(labels[QStringLiteral("columns")],
            (QJsonObject {{QStringLiteral("2"), QStringLiteral("Ctrl+2")}, {QStringLiteral("3"), QStringLiteral("3")},
                {QStringLiteral("4"), QStringLiteral("4")}}));
    }

    void thePanelScreenPicksItsOutput()
    {
        const auto output = [](const char *name, int x) {
            return QJsonObject {{QStringLiteral("name"), QLatin1String(name)},
                {QStringLiteral("logical"),
                    QJsonObject {{QStringLiteral("x"), x}, {QStringLiteral("y"), 0}, {QStringLiteral("width"), 1920},
                        {QStringLiteral("height"), 1080}}}};
        };
        const QJsonArray outputs {output("DP-1", 0), output("DP-2", 1920)};
        const auto screen = [](int x) {
            return QJsonObject {
                {QStringLiteral("x"), x}, {QStringLiteral("y"), 0}, {QStringLiteral("width"), 1920}, {QStringLiteral("height"), 1080}};
        };
        QCOMPARE(call("outputAt", {outputs, screen(1920)}).toString(), QStringLiteral("DP-2"));
        QCOMPARE(call("outputAt", {outputs, screen(9000)}).toString(), QStringLiteral("DP-1"));
        QCOMPARE(call("outputAt", {QJsonArray(), screen(0)}).toString(), QString());
    }

    void scrollingStepsThroughTheWorkspaces()
    {
        const QJsonArray workspaces {QJsonObject {{QStringLiteral("idx"), 1}, {QStringLiteral("is_active"), true}},
            QJsonObject {{QStringLiteral("idx"), 2}, {QStringLiteral("is_active"), false}}};
        QCOMPARE(call("steppedWorkspace", {workspaces, 1})[QStringLiteral("idx")].toInt(), 2);
        QVERIFY(call("steppedWorkspace", {workspaces, -1}).isNull());
    }

    void keyedModelsFollowTheKeys()
    {
        QVariant result;
        QMetaObject::invokeMethod(m_probe.get(), "sync", Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, QStringList({"a", "b", "c"})));
        QCOMPARE(result.toString(), QStringLiteral("a,b,c"));
        QMetaObject::invokeMethod(m_probe.get(), "sync", Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, QStringList({"c", "a", "d"})));
        QCOMPARE(result.toString(), QStringLiteral("c,a,d"));
        QMetaObject::invokeMethod(m_probe.get(), "sync", Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, QStringList()));
        QCOMPARE(result.toString(), QString());
    }
};

QTEST_GUILESS_MAIN(TestPlasmoidTaskbar)

#include "test_plasmoid_taskbar.moc"
