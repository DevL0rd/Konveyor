#include "helpers.h"

using namespace LayoutTest;

namespace
{

struct Grid
{
    Fixture fixture;
    QHash<QString, Layout::WindowId> ids;

    Grid()
    {
        ids.insert(QStringLiteral("a1"), fixture.add(QStringLiteral("a1")));
        ids.insert(QStringLiteral("a2"), fixture.add(QStringLiteral("a2")));
        fixture.perform(QStringLiteral("consume-or-expel-window-left"));
        ids.insert(QStringLiteral("a3"), fixture.add(QStringLiteral("a3")));
        fixture.perform(QStringLiteral("consume-or-expel-window-left"));
        ids.insert(QStringLiteral("b"), fixture.add(QStringLiteral("b")));
        ids.insert(QStringLiteral("c"), fixture.add(QStringLiteral("c")));
        fixture.engine().activateWindow(ids.value(QStringLiteral("a2")));
    }

    QString focusedName() const
    {
        const auto focused = fixture.focused();
        return focused ? ids.key(*focused) : QString();
    }
};

struct Monitors
{
    Fixture fixture;
    Layout::WindowId window = fixture.add(QStringLiteral("a"));

    explicit Monitors(const QList<std::pair<QString, QRectF>> &outputs)
    {
        for (const auto &[name, geometry] : outputs) {
            fixture.addOutput(name, geometry);
        }
        fixture.engine().focusOutput(QStringLiteral("DP-1"));
    }

    QString focusedOutput() { return fixture.engine().focusedOutput().value_or(QString()); }
};

QList<std::pair<QString, QRectF>> lShapedOutputs()
{
    return {{QStringLiteral("DP-2"), QRectF(1920, 0, 1920, 1080)}, {QStringLiteral("DP-3"), QRectF(0, 1080, 1920, 1080)}};
}

}

class TestLayoutFocusGrid : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void gridHasTheExpectedShape()
    {
        Grid grid;
        for (const QString &name : {QStringLiteral("a1"), QStringLiteral("a2"), QStringLiteral("a3")}) {
            QCOMPARE(grid.fixture.state(grid.ids.value(name)).columnIndex, 0);
        }
        QCOMPARE(grid.fixture.state(grid.ids.value(QStringLiteral("a1"))).tileIndex, 0);
        QCOMPARE(grid.fixture.state(grid.ids.value(QStringLiteral("a3"))).tileIndex, 2);
        QCOMPARE(grid.fixture.state(grid.ids.value(QStringLiteral("b"))).columnIndex, 1);
        QCOMPARE(grid.fixture.state(grid.ids.value(QStringLiteral("c"))).columnIndex, 2);
        QCOMPARE(grid.focusedName(), QStringLiteral("a2"));
    }

    void focusActionMovesFocus_data()
    {
        QTest::addColumn<QString>("start");
        QTest::addColumn<QString>("name");
        QTest::addColumn<QString>("argument");
        QTest::addColumn<QString>("expected");
        const QList<QStringList> rows {
            {"b", "focus-column-left", "", "a2"},
            {"b", "focus-column-right", "", "c"},
            {"c", "focus-column-right", "", "c"},
            {"a2", "focus-column-left", "", "a2"},
            {"b", "focus-column-first", "", "a2"},
            {"a1", "focus-column-last", "", "c"},
            {"b", "focus-column-right-or-first", "", "c"},
            {"c", "focus-column-right-or-first", "", "a2"},
            {"b", "focus-column-left-or-last", "", "a2"},
            {"a2", "focus-column-left-or-last", "", "c"},
            {"a1", "focus-window-down", "", "a2"},
            {"a3", "focus-window-down", "", "a3"},
            {"a3", "focus-window-up", "", "a2"},
            {"a1", "focus-window-up", "", "a1"},
            {"b", "focus-window-down", "", "b"},
            {"a1", "focus-window-bottom", "", "a3"},
            {"a3", "focus-window-top", "", "a1"},
            {"a3", "focus-window-down-or-top", "", "a1"},
            {"a1", "focus-window-down-or-top", "", "a2"},
            {"a1", "focus-window-up-or-bottom", "", "a3"},
            {"a3", "focus-window-up-or-bottom", "", "a2"},
            {"a3", "focus-window-down-or-column-right", "", "b"},
            {"a1", "focus-window-down-or-column-right", "", "a2"},
            {"a3", "focus-window-down-or-column-left", "", "a3"},
            {"b", "focus-window-down-or-column-left", "", "a2"},
            {"a1", "focus-window-up-or-column-right", "", "b"},
            {"a1", "focus-window-up-or-column-left", "", "a1"},
            {"b", "focus-window-up-or-column-left", "", "a2"},
            {"c", "focus-window-up-or-column-left", "", "b"},
            {"c", "focus-column", "1", "a2"},
            {"a1", "focus-column", "2", "b"},
            {"a1", "focus-column", "9", "c"},
            {"c", "focus-column", "0", "a2"},
            {"a1", "focus-window-in-column", "3", "a3"},
            {"a3", "focus-window-in-column", "1", "a1"},
            {"a1", "focus-window-in-column", "9", "a3"},
            {"b", "focus-window-in-column", "2", "b"},
        };
        for (const QStringList &row : rows) {
            const QString tag = row[0] + QLatin1Char(' ') + row[1] + QLatin1Char(' ') + row[2];
            QTest::newRow(qPrintable(tag)) << row[0] << row[1] << row[2] << row[3];
        }
    }

    void focusActionMovesFocus()
    {
        QFETCH(QString, start);
        QFETCH(QString, name);
        QFETCH(QString, argument);
        QFETCH(QString, expected);
        Grid grid;
        grid.fixture.engine().activateWindow(grid.ids.value(start));
        const QStringList arguments = argument.isEmpty() ? QStringList() : QStringList {argument};
        const Layout::ActionResult result = grid.fixture.perform(name, arguments);
        QVERIFY2(result.ok, qPrintable(result.error));
        QCOMPARE(grid.focusedName(), expected);
        QVERIFY(grid.fixture.state(grid.ids.value(expected)).isFocused);
        VERIFY_INVARIANTS(grid.fixture);
    }

    void focusByIndexRejectsBadArguments()
    {
        Grid grid;
        QVERIFY(!grid.fixture.perform(QStringLiteral("focus-column"), {QStringLiteral("x")}).ok);
        QVERIFY(!grid.fixture.perform(QStringLiteral("focus-window-in-column")).ok);
        QCOMPARE(grid.focusedName(), QStringLiteral("a2"));
    }

    void focusWindowOrWorkspaceMovesBetweenWorkspacesAtTheEnds()
    {
        Grid grid;
        grid.fixture.engine().activateWindow(grid.ids.value(QStringLiteral("a3")));
        QVERIFY(grid.fixture.perform(QStringLiteral("focus-window-or-workspace-down")).ok);
        QCOMPARE(grid.fixture.focused(), std::optional<Layout::WindowId>());
        QVERIFY(grid.fixture.perform(QStringLiteral("focus-window-or-workspace-up")).ok);
        QCOMPARE(grid.focusedName(), QStringLiteral("a3"));
        QVERIFY(grid.fixture.perform(QStringLiteral("focus-window-or-workspace-up")).ok);
        QCOMPARE(grid.focusedName(), QStringLiteral("a2"));
        grid.fixture.perform(QStringLiteral("focus-window-top"));
        QVERIFY(grid.fixture.perform(QStringLiteral("focus-window-or-workspace-up")).ok);
        QCOMPARE(grid.focusedName(), QStringLiteral("a1"));
        VERIFY_INVARIANTS(grid.fixture);
    }

    void focusWindowOrMonitorCrossesToTheMonitorBelow()
    {
        Grid grid;
        grid.fixture.addOutput(QStringLiteral("DP-2"), QRectF(0, 1080, 1920, 1080));
        grid.fixture.engine().focusOutput(QStringLiteral("DP-1"));
        grid.fixture.engine().activateWindow(grid.ids.value(QStringLiteral("a2")));
        QVERIFY(grid.fixture.perform(QStringLiteral("focus-window-or-monitor-down")).ok);
        QCOMPARE(grid.focusedName(), QStringLiteral("a3"));
        QVERIFY(grid.fixture.perform(QStringLiteral("focus-window-or-monitor-down")).ok);
        QCOMPARE(grid.fixture.engine().focusedOutput(), std::optional(QStringLiteral("DP-2")));
        QCOMPARE(grid.fixture.focused(), std::optional<Layout::WindowId>());
        QVERIFY(grid.fixture.perform(QStringLiteral("focus-window-or-monitor-up")).ok);
        QCOMPARE(grid.fixture.engine().focusedOutput(), std::optional(QStringLiteral("DP-1")));
        QCOMPARE(grid.focusedName(), QStringLiteral("a3"));
        QVERIFY(grid.fixture.perform(QStringLiteral("focus-window-or-monitor-up")).ok);
        QCOMPARE(grid.focusedName(), QStringLiteral("a2"));
        VERIFY_INVARIANTS(grid.fixture);
    }

    void focusColumnOrMonitorCrossesToTheMonitorBeside()
    {
        Grid grid;
        grid.fixture.addOutput(QStringLiteral("DP-2"), QRectF(1920, 0, 1920, 1080));
        grid.fixture.engine().activateWindow(grid.ids.value(QStringLiteral("b")));
        QVERIFY(grid.fixture.perform(QStringLiteral("focus-column-or-monitor-right")).ok);
        QCOMPARE(grid.focusedName(), QStringLiteral("c"));
        QVERIFY(grid.fixture.perform(QStringLiteral("focus-column-or-monitor-right")).ok);
        QCOMPARE(grid.fixture.engine().focusedOutput(), std::optional(QStringLiteral("DP-2")));
        QVERIFY(grid.fixture.perform(QStringLiteral("focus-column-or-monitor-right")).ok);
        QCOMPARE(grid.fixture.engine().focusedOutput(), std::optional(QStringLiteral("DP-2")));
        QVERIFY(grid.fixture.perform(QStringLiteral("focus-column-or-monitor-left")).ok);
        QCOMPARE(grid.focusedName(), QStringLiteral("c"));
        QVERIFY(grid.fixture.perform(QStringLiteral("focus-column-or-monitor-left")).ok);
        QCOMPARE(grid.focusedName(), QStringLiteral("b"));
        VERIFY_INVARIANTS(grid.fixture);
    }

    void focusMonitorFollowsTheOutputLayout_data()
    {
        QTest::addColumn<QString>("from");
        QTest::addColumn<QString>("name");
        QTest::addColumn<QString>("expected");
        const QList<QStringList> rows {
            {"DP-1", "focus-monitor-right", "DP-2"},
            {"DP-1", "focus-monitor-down", "DP-3"},
            {"DP-1", "focus-monitor-left", "DP-1"},
            {"DP-1", "focus-monitor-up", "DP-1"},
            {"DP-1", "focus-monitor-next", "DP-2"},
            {"DP-1", "focus-monitor-previous", "DP-3"},
            {"DP-3", "focus-monitor-next", "DP-1"},
            {"DP-3", "focus-monitor-up", "DP-1"},
            {"DP-3", "focus-monitor-right", "DP-3"},
            {"DP-2", "focus-monitor-left", "DP-1"},
            {"DP-2", "focus-monitor-down", "DP-2"},
        };
        for (const QStringList &row : rows) {
            QTest::newRow(qPrintable(row[0] + QLatin1Char(' ') + row[1])) << row[0] << row[1] << row[2];
        }
    }

    void focusMonitorFollowsTheOutputLayout()
    {
        QFETCH(QString, from);
        QFETCH(QString, name);
        QFETCH(QString, expected);
        Monitors monitors(lShapedOutputs());
        monitors.fixture.engine().focusOutput(from);
        QVERIFY(monitors.fixture.perform(name).ok);
        QCOMPARE(monitors.focusedOutput(), expected);
    }

    void focusMonitorByName()
    {
        Monitors monitors(lShapedOutputs());
        QVERIFY(monitors.fixture.perform(QStringLiteral("focus-monitor"), {QStringLiteral("dp-3")}).ok);
        QCOMPARE(monitors.focusedOutput(), QStringLiteral("DP-3"));
        QVERIFY(!monitors.fixture.perform(QStringLiteral("focus-monitor"), {QStringLiteral("HDMI-9")}).ok);
        QCOMPARE(monitors.focusedOutput(), QStringLiteral("DP-3"));
        QVERIFY(monitors.fixture.perform(QStringLiteral("focus-monitor"), {QStringLiteral("DP-1")}).ok);
        QCOMPARE(monitors.fixture.focused(), std::optional(monitors.window));
    }

    void focusMonitorWithOneOutputStaysPut()
    {
        Fixture fixture;
        const auto id = fixture.add();
        for (const char *direction : {"left", "right", "up", "down", "next", "previous"}) {
            QVERIFY(fixture.perform(QStringLiteral("focus-monitor-") + QLatin1String(direction)).ok);
            QCOMPARE(fixture.engine().focusedOutput(), std::optional(QStringLiteral("DP-1")));
            QCOMPARE(fixture.focused(), std::optional(id));
        }
    }

    void focusWindowPreviousSwitchesWorkspaceAndMonitor()
    {
        Monitors monitors(lShapedOutputs());
        QVERIFY(monitors.fixture.perform(QStringLiteral("focus-monitor"), {QStringLiteral("DP-2")}).ok);
        const auto other = monitors.fixture.add(QStringLiteral("b"));
        QVERIFY(monitors.fixture.perform(QStringLiteral("focus-window-previous")).ok);
        QCOMPARE(monitors.fixture.focused(), std::optional(monitors.window));
        QCOMPARE(monitors.focusedOutput(), QStringLiteral("DP-1"));
        QVERIFY(monitors.fixture.perform(QStringLiteral("move-window-to-workspace-down")).ok);
        QVERIFY(monitors.fixture.perform(QStringLiteral("focus-window-previous")).ok);
        QCOMPARE(monitors.fixture.focused(), std::optional(other));
        QVERIFY(monitors.fixture.perform(QStringLiteral("focus-window-previous")).ok);
        QCOMPARE(monitors.fixture.focused(), std::optional(monitors.window));
        QVERIFY(monitors.fixture.state(monitors.window).onActiveWorkspace);
        monitors.fixture.remove(other);
        QVERIFY(monitors.fixture.perform(QStringLiteral("focus-window-previous")).ok);
        QCOMPARE(monitors.fixture.focused(), std::optional(monitors.window));
        VERIFY_INVARIANTS(monitors.fixture);
    }

    void focusWindowByIdSwitchesWorkspaceAndMonitor()
    {
        Monitors monitors(lShapedOutputs());
        QVERIFY(monitors.fixture.perform(QStringLiteral("move-window-to-workspace-down")).ok);
        QVERIFY(monitors.fixture.perform(QStringLiteral("focus-monitor"), {QStringLiteral("DP-3")}).ok);
        QVERIFY(monitors.fixture.perform(QStringLiteral("focus-window"), {}, idProperty(monitors.window)).ok);
        QCOMPARE(monitors.focusedOutput(), QStringLiteral("DP-1"));
        QCOMPARE(monitors.fixture.focused(), std::optional(monitors.window));
        QVERIFY(monitors.fixture.state(monitors.window).onActiveWorkspace);
        QVERIFY(!monitors.fixture.perform(QStringLiteral("focus-window")).ok);
    }

    void focusBetweenFloatingAndTiling()
    {
        Grid grid;
        const auto floating = grid.fixture.add(QStringLiteral("f"));
        QVERIFY(grid.fixture.perform(QStringLiteral("toggle-window-floating")).ok);
        QCOMPARE(grid.fixture.focused(), std::optional(floating));
        QVERIFY(grid.fixture.perform(QStringLiteral("focus-tiling")).ok);
        QVERIFY(grid.fixture.focused() != std::optional(floating));
        QVERIFY(!grid.fixture.state(*grid.fixture.focused()).isFloating);
        QVERIFY(grid.fixture.perform(QStringLiteral("focus-floating")).ok);
        QCOMPARE(grid.fixture.focused(), std::optional(floating));
        QVERIFY(grid.fixture.perform(QStringLiteral("switch-focus-between-floating-and-tiling")).ok);
        QVERIFY(!grid.fixture.state(*grid.fixture.focused()).isFloating);
        QVERIFY(grid.fixture.perform(QStringLiteral("switch-focus-between-floating-and-tiling")).ok);
        QCOMPARE(grid.fixture.focused(), std::optional(floating));
        QVERIFY(grid.fixture.perform(QStringLiteral("focus-column"), {QStringLiteral("2")}).ok);
        QCOMPARE(grid.focusedName(), QStringLiteral("b"));
        VERIFY_INVARIANTS(grid.fixture);
    }

    void focusActionsOnAnEmptyWorkspaceDoNothing()
    {
        Fixture fixture;
        for (const char *name : {"focus-column-left", "focus-column-right-or-first", "focus-window-down-or-top", "focus-window-previous",
                 "focus-floating", "focus-tiling", "focus-column-or-monitor-left", "focus-window-or-monitor-down"}) {
            QVERIFY(fixture.perform(QString::fromLatin1(name)).ok);
            QCOMPARE(fixture.focused(), std::optional<Layout::WindowId>());
        }
        QVERIFY(fixture.perform(QStringLiteral("focus-column"), {QStringLiteral("1")}).ok);
        VERIFY_INVARIANTS(fixture);
    }
};

QTEST_GUILESS_MAIN(TestLayoutFocusGrid)
#include "test_layout_focusgrid.moc"
