#include "actionhelpers.h"

using namespace LayoutTest;

namespace
{

const QString Center = QStringLiteral("DP-1");
const QString Right = QStringLiteral("DP-2");
const QString Left = QStringLiteral("DP-3");
const QString Below = QStringLiteral("DP-4");

struct CrossOfOutputs
{
    Fixture fixture;

    CrossOfOutputs()
    {
        addOutputAt(fixture, Right, QRectF(1920, 0, 1920, 1080));
        addOutputAt(fixture, Left, QRectF(-1280, 200, 1280, 720));
        addOutputAt(fixture, Below, QRectF(0, 1080, 1920, 1080));
        fixture.engine().focusOutput(Center);
    }
};

void focusSteps(Fixture &fixture, const QString &prefix, const QList<std::pair<QString, QString>> &steps)
{
    for (const auto &[direction, expected] : steps) {
        QVERIFY(act(fixture, prefix + direction).ok);
        QCOMPARE(focusedOutput(fixture), expected);
    }
}

struct StackedOutputs
{
    Fixture fixture;
    Layout::WindowId above = placeAbove();
    WindowIds stack = stackOf(fixture, 2);

    Layout::WindowId placeAbove()
    {
        addOutputAt(fixture, QStringLiteral("DP-2"), QRectF(200, -720, 1280, 720));
        const Layout::WindowId id = addOn(fixture, QStringLiteral("DP-2"), QStringLiteral("above"));
        fixture.engine().focusOutput(Center);
        return id;
    }
};

}

class TestLayoutMonitorFocus : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void focusMonitorDirectionsFollowTheOutputArrangement()
    {
        CrossOfOutputs f;
        focusSteps(f.fixture, QStringLiteral("focus-monitor-"),
            {{QStringLiteral("left"), Left}, {QStringLiteral("left"), Left}, {QStringLiteral("up"), Left},
                {QStringLiteral("right"), Center}, {QStringLiteral("right"), Right}, {QStringLiteral("right"), Right},
                {QStringLiteral("left"), Center}, {QStringLiteral("down"), Below}, {QStringLiteral("down"), Below},
                {QStringLiteral("up"), Center}, {QStringLiteral("up"), Center}});
    }

    void focusMonitorUpPicksTheOutputDirectlyAboveOverACloserDiagonalOne()
    {
        CrossOfOutputs f;
        f.fixture.engine().focusOutput(Below);
        focusSteps(f.fixture, QStringLiteral("focus-monitor-"), {{QStringLiteral("up"), Center}});
        f.fixture.engine().focusOutput(Below);
        focusSteps(f.fixture, QStringLiteral("focus-monitor-"), {{QStringLiteral("left"), Below}, {QStringLiteral("right"), Below}});
    }

    void focusMonitorNextAndPreviousCycleInConnectionOrder()
    {
        CrossOfOutputs f;
        focusSteps(f.fixture, QStringLiteral("focus-monitor-"),
            {{QStringLiteral("next"), Right}, {QStringLiteral("next"), Left}, {QStringLiteral("next"), Below},
                {QStringLiteral("next"), Center}, {QStringLiteral("previous"), Below}, {QStringLiteral("previous"), Left}});
    }

    void focusMonitorOnASingleOutputStaysThere()
    {
        Fixture fixture;
        const auto id = fixture.add();
        focusSteps(fixture, QStringLiteral("focus-monitor-"),
            {{QStringLiteral("left"), Center}, {QStringLiteral("down"), Center}, {QStringLiteral("next"), Center},
                {QStringLiteral("previous"), Center}});
        COMPARE_FOCUS(fixture, id);
    }

    void focusMonitorFocusesThatOutputsActiveWindow()
    {
        CrossOfOutputs f;
        const auto left = addOn(f.fixture, Left);
        const auto center = addOn(f.fixture, Center);
        QVERIFY(act(f.fixture, QStringLiteral("focus-monitor"), {QStringLiteral("dp-3")}).ok);
        COMPARE_FOCUS(f.fixture, left);
        QVERIFY(act(f.fixture, QStringLiteral("focus-monitor-right")).ok);
        COMPARE_FOCUS(f.fixture, center);
        QVERIFY(act(f.fixture, QStringLiteral("focus-monitor-down")).ok);
        QCOMPARE(f.fixture.focused(), std::optional<Layout::WindowId>());
        QVERIFY(!act(f.fixture, QStringLiteral("focus-monitor"), {QStringLiteral("HDMI-9")}).ok);
        QCOMPARE(focusedOutput(f.fixture), Below);
    }

    void focusColumnOrMonitorCrossesToTheNeighbourAtTheEdge()
    {
        CrossOfOutputs f;
        const auto right = addOn(f.fixture, Right);
        const WindowIds ids = [&f] {
            f.fixture.engine().focusOutput(Center);
            return columnsOf(f.fixture, 2);
        }();
        act(f.fixture, QStringLiteral("focus-column-first"));
        QVERIFY(act(f.fixture, QStringLiteral("focus-column-or-monitor-right")).ok);
        COMPARE_FOCUS(f.fixture, ids[1]);
        QVERIFY(act(f.fixture, QStringLiteral("focus-column-or-monitor-right")).ok);
        COMPARE_FOCUS(f.fixture, right);
        QVERIFY(act(f.fixture, QStringLiteral("focus-column-or-monitor-right")).ok);
        COMPARE_FOCUS(f.fixture, right);
        QVERIFY(act(f.fixture, QStringLiteral("focus-column-or-monitor-left")).ok);
        COMPARE_FOCUS(f.fixture, ids[1]);
        act(f.fixture, QStringLiteral("focus-column-or-monitor-left"));
        act(f.fixture, QStringLiteral("focus-column-or-monitor-left"));
        QCOMPARE(focusedOutput(f.fixture), Left);
        QCOMPARE(f.fixture.focused(), std::optional<Layout::WindowId>());
        VERIFY_INVARIANTS(f.fixture);
    }

    void focusWindowOrMonitorWalksTheStackBeforeCrossing()
    {
        StackedOutputs f;
        act(f.fixture, QStringLiteral("focus-window-bottom"));
        QVERIFY(act(f.fixture, QStringLiteral("focus-window-or-monitor-up")).ok);
        COMPARE_FOCUS(f.fixture, f.stack[0]);
        QVERIFY(act(f.fixture, QStringLiteral("focus-window-or-monitor-up")).ok);
        COMPARE_FOCUS(f.fixture, f.above);
        QVERIFY(act(f.fixture, QStringLiteral("focus-window-or-monitor-up")).ok);
        COMPARE_FOCUS(f.fixture, f.above);
        QVERIFY(act(f.fixture, QStringLiteral("focus-window-or-monitor-down")).ok);
        COMPARE_FOCUS(f.fixture, f.stack[0]);
        QVERIFY(act(f.fixture, QStringLiteral("focus-window-or-monitor-down")).ok);
        COMPARE_FOCUS(f.fixture, f.stack[1]);
        QVERIFY(act(f.fixture, QStringLiteral("focus-window-or-monitor-down")).ok);
        COMPARE_FOCUS(f.fixture, f.stack[1]);
        QCOMPARE(focusedOutput(f.fixture), Center);
    }

    void focusWindowOrMonitorLeavesAFocusedFloatingWindowForTheStack()
    {
        StackedOutputs f;
        const auto floating = addFloating(f.fixture);
        COMPARE_FOCUS(f.fixture, floating);
        QVERIFY(act(f.fixture, QStringLiteral("focus-window-or-monitor-up")).ok);
        COMPARE_FOCUS(f.fixture, f.stack[0]);
    }
};

QTEST_GUILESS_MAIN(TestLayoutMonitorFocus)
#include "test_layout_monitorfocus.moc"
