#include "layout/common/edgehover.h"

#include <QTest>

using namespace Konveyor;
using Layout::EdgeHoverTarget;
using Layout::EdgeHoverTracker;
using Layout::HoverSide;
using Monitors = Config::EdgeSwitchMonitorEdges;

namespace
{

const QRectF Left(0, 0, 1920, 1080);
const QRectF Right(1920, 0, 1920, 1080);
const QList<QRectF> Pair {Left, Right};
constexpr double Corner = 40;

std::optional<HoverSide> sideAt(QPointF pointer, const QRectF &output, Monitors monitors = Monitors::Outer, QRectF workArea = {})
{
    return Layout::hoverSideAt(pointer, {output, workArea.isNull() ? output : workArea}, Pair, monitors, Corner);
}

}

class TestLayoutEdgeHover : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void outerEdgesSwitchBothWays()
    {
        QCOMPARE(sideAt(QPointF(0, 540), Left), HoverSide::Left);
        QCOMPARE(sideAt(QPointF(0.5, 540), Left), HoverSide::Left);
        QCOMPARE(sideAt(QPointF(3839, 540), Right), HoverSide::Right);
        QCOMPARE(sideAt(QPointF(3839.6, 540), Right), HoverSide::Right);
        QCOMPARE(sideAt(QPointF(1, 540), Left), std::nullopt);
        QCOMPARE(sideAt(QPointF(960, 540), Left), std::nullopt);
        QCOMPARE(sideAt(QPointF(3838, 540), Right), std::nullopt);
    }

    void sharedEdgesFollowTheMonitorSetting()
    {
        QCOMPARE(sideAt(QPointF(1919, 540), Left), std::nullopt);
        QCOMPARE(sideAt(QPointF(1920, 540), Right), std::nullopt);
        QCOMPARE(sideAt(QPointF(1919, 540), Left, Monitors::All), HoverSide::Right);
        QCOMPARE(sideAt(QPointF(1920, 540), Right, Monitors::All), HoverSide::Left);
        QCOMPARE(sideAt(QPointF(0, 540), Left, Monitors::All), HoverSide::Left);
    }

    void edgesBesideAShorterNeighbourStayOuter()
    {
        const QRectF tall(0, 0, 1920, 2160);
        const QRectF shortNeighbour(1920, 0, 1920, 1080);
        const QList<QRectF> outputs {tall, shortNeighbour};
        const auto at = [&](double y) { return Layout::hoverSideAt(QPointF(1919, y), {tall, tall}, outputs, Monitors::Outer, Corner); };
        QCOMPARE(at(500), std::nullopt);
        QCOMPARE(at(1500), HoverSide::Right);
    }

    void cornersAndPanelsDoNotCount()
    {
        QCOMPARE(sideAt(QPointF(0, 10), Left), std::nullopt);
        QCOMPARE(sideAt(QPointF(0, 1075), Left), std::nullopt);
        QCOMPARE(sideAt(QPointF(0, 40), Left), HoverSide::Left);
        QCOMPARE(sideAt(QPointF(0, 1039), Left), HoverSide::Left);
        const QRectF bottomPanel(0, 0, 1920, 1000);
        QCOMPARE(sideAt(QPointF(0, 1010), Left, Monitors::Outer, bottomPanel), std::nullopt);
        QCOMPARE(sideAt(QPointF(0, 990), Left, Monitors::Outer, bottomPanel), HoverSide::Left);
        const QRectF sidePanel(60, 0, 1860, 1080);
        QCOMPARE(sideAt(QPointF(0, 540), Left, Monitors::Outer, sidePanel), HoverSide::Left);
        QCOMPARE(sideAt(QPointF(60, 540), Left, Monitors::Outer, sidePanel), std::nullopt);
    }

    void pointerOutsideTheOutputIsIgnored()
    {
        QCOMPARE(sideAt(QPointF(-1, 540), Left), std::nullopt);
        QCOMPARE(sideAt(QPointF(0, 540), Right), std::nullopt);
        QCOMPARE(sideAt(QPointF(1920, 540), Left, Monitors::All), std::nullopt);
        QCOMPARE(sideAt(QPointF(0, 1080), Left), std::nullopt);
    }

    void trackerSwitchesOncePerEntry()
    {
        EdgeHoverTracker tracker;
        const EdgeHoverTarget left {QStringLiteral("DP-1"), HoverSide::Left};
        const EdgeHoverTarget right {QStringLiteral("DP-1"), HoverSide::Right};
        QCOMPARE(tracker.take(), std::nullopt);
        QCOMPARE(tracker.move(left), EdgeHoverTracker::Change::Entered);
        QCOMPARE(tracker.move(left), EdgeHoverTracker::Change::None);
        QCOMPARE(tracker.take(), left);
        QCOMPARE(tracker.take(), std::nullopt);
        QCOMPARE(tracker.move(left), EdgeHoverTracker::Change::None);
        QCOMPARE(tracker.take(), std::nullopt);
        QCOMPARE(tracker.move(std::nullopt), EdgeHoverTracker::Change::Left);
        QCOMPARE(tracker.move(std::nullopt), EdgeHoverTracker::Change::None);
        QCOMPARE(tracker.take(), std::nullopt);
        QCOMPARE(tracker.move(left), EdgeHoverTracker::Change::Entered);
        QCOMPARE(tracker.take(), left);
        QCOMPARE(tracker.move(right), EdgeHoverTracker::Change::Entered);
        QCOMPARE(tracker.take(), right);
        const EdgeHoverTarget otherOutput {QStringLiteral("DP-2"), HoverSide::Left};
        QCOMPARE(tracker.move(otherOutput), EdgeHoverTracker::Change::Entered);
        tracker.reset();
        QCOMPARE(tracker.take(), std::nullopt);
        QCOMPARE(tracker.move(otherOutput), EdgeHoverTracker::Change::Entered);
        QCOMPARE(tracker.take(), otherOutput);
    }
};

QTEST_MAIN(TestLayoutEdgeHover)
#include "test_layout_edgehover.moc"
