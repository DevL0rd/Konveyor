#include "helpers.h"

using namespace LayoutTest;

namespace
{

const QString Primary = QStringLiteral("DP-1");
const QString Secondary = QStringLiteral("DP-2");

struct OverviewRow : WideRow
{
    using WideRow::WideRow;

    void holdWindowDragAt(QPointF pointer, int moves)
    {
        for (int move = 0; move < moves; ++move) {
            fixture.passTime(100);
            fixture.engine().updateWindowDrag(pointer + QPointF(0, move % 2), output);
        }
        fixture.settle();
    }

    Layout::WorkspaceState activeWorkspace(const QString &on)
    {
        for (const Layout::WorkspaceState &state : fixture.engine().workspaceStates()) {
            if (state.output == on && state.isActive) {
                return state;
            }
        }
        return {};
    }
};

}

class TestLayoutOverviewDrag : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void windowDragHeldAtTheBottomEdgeOfTheOverviewDropsOnTheNextWorkspace_data()
    {
        QTest::addColumn<bool>("overview");
        QTest::addColumn<int>("workspace");
        QTest::newRow("overview open") << true << 2;
        QTest::newRow("overview closed") << false << 1;
    }

    void windowDragHeldAtTheBottomEdgeOfTheOverviewDropsOnTheNextWorkspace()
    {
        QFETCH(bool, overview);
        QFETCH(int, workspace);
        OverviewRow row(Primary, animatedWideColumns());
        row.fixture.engine().setOverviewOpen(overview);
        row.fixture.advanceInSteps(1000);
        const Layout::WorkspaceId home = row.fixture.state(row.first).workspace;
        QVERIFY(startMove(row.fixture, row.last, QPointF(960, 500)));
        row.holdWindowDragAt(QPointF(960, 1075), 16);
        row.fixture.engine().endWindowDrag();
        row.fixture.advanceInSteps(1000);
        const Layout::WorkspaceState active = row.activeWorkspace(Primary);
        QCOMPARE(active.index, workspace);
        QCOMPARE(row.fixture.state(row.last).workspace, active.id);
        QVERIFY(row.fixture.state(row.last).onActiveWorkspace);
        QCOMPARE(row.fixture.state(row.first).workspace, home);
        QCOMPARE(row.fixture.focused(), std::optional(row.last));
        QVERIFY(!row.fixture.engine().isAnimating());
        VERIFY_INVARIANTS(row.fixture);
    }

    void removingTheOutputDuringAnOverviewEdgeDragDropsOnTheRemainingOutput()
    {
        OverviewRow row(Secondary, animatedWideColumns());
        row.fixture.engine().setOverviewOpen(true);
        row.fixture.advanceInSteps(1000);
        QVERIFY(startMove(row.fixture, row.last, QPointF(2880, 500), Secondary));
        row.holdWindowDragAt(QPointF(2880, 1075), 6);
        QVERIFY(row.fixture.engine().isAnimating());
        row.fixture.removeOutput(Secondary);
        row.fixture.engine().endWindowDrag();
        row.fixture.advanceInSteps(1000);
        QCOMPARE(row.fixture.state(row.last).output, Primary);
        QVERIFY(!row.fixture.state(row.last).isFloating);
        QVERIFY(row.fixture.state(row.last).onActiveWorkspace);
        QVERIFY2(QRectF(0, 0, 1920, 1080).contains(row.fixture.frame(row.last)), "the dropped window is off screen");
        QCOMPARE(row.fixture.focused(), std::optional(row.last));
        QVERIFY(!row.fixture.engine().outputStates().constFirst().dropHint);
        QVERIFY2(!row.fixture.engine().isAnimating(), "the edge scroll kept running after the drop");
        VERIFY_INVARIANTS(row.fixture);
    }
};

QTEST_GUILESS_MAIN(TestLayoutOverviewDrag)
#include "test_layout_overviewdrag.moc"
