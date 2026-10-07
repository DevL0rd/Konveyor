#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QTest>

#include <memory>

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

    std::unique_ptr<QQmlEngine> m_engine;
    std::unique_ptr<QObject> m_probe;

    QJsonValue call(const char *name, const QJsonArray &arguments)
    {
        QVariant result;
        const QString text = QString::fromUtf8(QJsonDocument(arguments).toJson(QJsonDocument::Compact));
        QMetaObject::invokeMethod(
            m_probe.get(), "run", Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, QLatin1String(name)), Q_ARG(QVariant, text));
        return QJsonDocument::fromJson(QByteArray("[") + result.toString().toUtf8() + QByteArray("]")).array().at(0);
    }

    QJsonArray build(const QJsonArray &columns, const QJsonArray &rows, const QJsonArray &pins, bool merge, const QJsonObject &parked = {})
    {
        QJsonObject windows;
        for (const QJsonValue &column : columns) {
            for (const QJsonValue &id : column.toArray()) {
                windows[QString::number(id.toInt())] = QJsonObject {{QStringLiteral("uuid"), QStringLiteral("u%1").arg(id.toInt())}};
            }
        }
        return call("buildItems",
            {QJsonObject {{QStringLiteral("columns"), columns}, {QStringLiteral("windowsById"), windows}, {QStringLiteral("rows"), rows},
                {QStringLiteral("pins"), pins}, {QStringLiteral("launchers"), QJsonObject()}, {QStringLiteral("merge"), merge},
                {QStringLiteral("parkedAfter"), parked}}})
            .toArray();
    }

private Q_SLOTS:
    void initTestCase()
    {
        m_engine = std::make_unique<QQmlEngine>();
        QQmlComponent component(m_engine.get());
        component.setData(probe, QUrl(QStringLiteral("file:///probe.qml")));
        m_probe.reset(component.create());
        QVERIFY2(m_probe, qPrintable(component.errorString()));
    }

    void itemsFollowTheColumnOrder()
    {
        const QJsonArray items = build({QJsonArray {4}, QJsonArray {2, 3}, QJsonArray {1}},
            {row("u1", "a"), row("u2", "b"), row("u3", "b"), row("u4", "c"), row("u9", "d")}, {}, false);
        QCOMPARE(keysOf(items), (QJsonArray {QStringLiteral("c4"), QStringLiteral("c2"), QStringLiteral("c1"), QStringLiteral("wu9")}));
        QCOMPARE(items[1][QStringLiteral("windows")].toArray().size(), 2);
        QCOMPARE(items[1][QStringLiteral("windows")][1][QStringLiteral("konveyorId")].toInt(), 3);
        QCOMPARE(items[2][QStringLiteral("columns")], (QJsonArray {QJsonObject {{QStringLiteral("index"), 3}, {QStringLiteral("id"), 1}}}));
        QCOMPARE(items[3][QStringLiteral("columns")], QJsonArray());
    }

    void mergingJoinsNeighbouringColumnsOfOneApp()
    {
        const QJsonArray rows {row("u1", "a"), row("u2", "a"), row("u3", "b"), row("u4", "a")};
        const QJsonArray columns {QJsonArray {1}, QJsonArray {2}, QJsonArray {3}, QJsonArray {4}};
        QCOMPARE(keysOf(build(columns, rows, {}, true)), (QJsonArray {QStringLiteral("c1"), QStringLiteral("c3"), QStringLiteral("c4")}));
        QCOMPARE(build(columns, rows, {}, true)[0][QStringLiteral("columns")].toArray().size(), 2);
        QCOMPARE(keysOf(build(columns, rows, {}, false)).size(), 4);
    }

    void minimizedWindowsKeepTheirSlot()
    {
        const QJsonArray rows {row("u1", "a"), row("u2", "b"), row("m1", "c"), row("m2", "d"), row("m3", "e")};
        const QJsonObject parked {{QStringLiteral("m1"), 1}, {QStringLiteral("m2"), QJsonValue()}, {QStringLiteral("m3"), 77}};
        QCOMPARE(keysOf(build({QJsonArray {1}, QJsonArray {2}}, rows, {}, false, parked)),
            (QJsonArray {QStringLiteral("wm2"), QStringLiteral("c1"), QStringLiteral("wm1"), QStringLiteral("c2"), QStringLiteral("wm3")}));
    }

    void idlePinsSitAfterTheirPinnedNeighbour()
    {
        const QJsonArray items = build({QJsonArray {1}, QJsonArray {2}}, {row("u1", "x"), row("u2", "b")},
            {QStringLiteral("p1"), QStringLiteral("b"), QStringLiteral("p2"), QStringLiteral("p3")}, false);
        QCOMPARE(keysOf(items),
            (QJsonArray {QStringLiteral("pp1"), QStringLiteral("c1"), QStringLiteral("c2"), QStringLiteral("pp2"), QStringLiteral("pp3")}));
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
                bind("Super+Ctrl+2", "focus-column", {QStringLiteral("2")}), bind("Super+W", "focus-workspace", {QStringLiteral("web")}),
                bind("Super+F1", "focus-workspace", {QStringLiteral("1")}), bind("Super+K", "show-hotkey-overlay", {})})});
        QCOMPARE(labels[QStringLiteral("workspaces")], (QJsonObject {{QStringLiteral("1"), QStringLiteral("1")}}));
        QCOMPARE(labels[QStringLiteral("columns")], (QJsonObject {{QStringLiteral("2"), QStringLiteral("Ctrl+2")}}));
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
