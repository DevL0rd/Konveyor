#include "layout/common/desktoppeek.h"

#include <QTest>

using namespace Konveyor;
using Layout::PeekAside;
using Layout::PeekKey;
using Layout::PeekSide;
using Step = PeekKey::Step;

namespace
{

const QRectF WorkArea(0, 0, 1920, 1040);
const QRectF Wide(16, 16, 1500, 1008);
const QRectF Narrow(700, 16, 500, 1008);
constexpr double Gap = 16;

}

class TestLayoutDesktopPeek : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void holdingMetaAlonePeeksAndReleaseEnds()
    {
        PeekKey key;
        QCOMPARE(key.metaPressed(true), Step::Arm);
        QVERIFY(!key.isPeeking());
        QCOMPARE(key.delayPassed(true), Step::Begin);
        QVERIFY(key.isPeeking());
        QCOMPARE(key.pointerUsed(), Step::None);
        QCOMPARE(key.delayPassed(true), Step::None);
        QCOMPARE(key.metaReleased(false), Step::End);
        QVERIFY(!key.isPeeking());
        QCOMPARE(key.dragEnded(), Step::None);
    }

    void aQuickTapNeverPeeks()
    {
        PeekKey key;
        QCOMPARE(key.metaPressed(true), Step::Arm);
        QCOMPARE(key.metaReleased(false), Step::Disarm);
        QCOMPARE(key.delayPassed(true), Step::None);
        QVERIFY(!key.isPeeking());
    }

    void anythingElseBeforeTheDelayCancels()
    {
        PeekKey key;
        QCOMPARE(key.metaPressed(true), Step::Arm);
        QCOMPARE(key.otherKeyPressed(), Step::Disarm);
        QCOMPARE(key.delayPassed(true), Step::None);
        QCOMPARE(key.metaReleased(false), Step::None);

        QCOMPARE(key.metaPressed(true), Step::Arm);
        QCOMPARE(key.pointerUsed(), Step::Disarm);
        QCOMPARE(key.delayPassed(true), Step::None);
        QCOMPARE(key.metaReleased(false), Step::None);

        QCOMPARE(key.metaPressed(false), Step::None);
        QCOMPARE(key.delayPassed(true), Step::None);

        QCOMPARE(key.metaPressed(true), Step::Arm);
        QCOMPARE(key.metaPressed(false), Step::Disarm);
        QCOMPARE(key.delayPassed(true), Step::None);
    }

    void aBlockedPeekStaysIdle()
    {
        PeekKey key;
        QCOMPARE(key.metaPressed(true), Step::Arm);
        QCOMPARE(key.delayPassed(false), Step::None);
        QVERIFY(!key.isPeeking());
        QCOMPARE(key.metaReleased(false), Step::None);
    }

    void anotherKeyDuringThePeekEndsIt()
    {
        PeekKey key;
        key.metaPressed(true);
        key.delayPassed(true);
        QCOMPARE(key.otherKeyPressed(), Step::End);
        QVERIFY(!key.isPeeking());
        QCOMPARE(key.metaReleased(false), Step::None);
    }

    void aDropOutlastsTheKey()
    {
        PeekKey key;
        key.metaPressed(true);
        key.delayPassed(true);
        QCOMPARE(key.metaReleased(true), Step::None);
        QVERIFY(key.isPeeking());
        QCOMPARE(key.otherKeyPressed(), Step::None);
        QCOMPARE(key.metaPressed(true), Step::None);
        QCOMPARE(key.metaReleased(true), Step::None);
        QCOMPARE(key.dragEnded(), Step::End);
        QVERIFY(!key.isPeeking());

        key.metaPressed(true);
        key.delayPassed(true);
        QCOMPARE(key.dragEnded(), Step::None);
        QCOMPARE(key.metaReleased(false), Step::End);
    }

    void resetEndsWhateverRuns()
    {
        PeekKey key;
        QCOMPARE(key.reset(), Step::None);
        key.metaPressed(true);
        QCOMPARE(key.reset(), Step::Disarm);
        key.metaPressed(true);
        key.delayPassed(true);
        QCOMPARE(key.reset(), Step::End);
        key.metaPressed(true);
        key.delayPassed(true);
        key.metaReleased(true);
        QCOMPARE(key.reset(), Step::End);
        QCOMPARE(key.dragEnded(), Step::None);
    }

    void leavingAWindowPushesItAwayOnce()
    {
        PeekAside aside;
        aside.begin(true);
        QCOMPARE(aside.pointerAt(QPointF(800, 500), Wide, WorkArea), std::nullopt);
        QCOMPARE(aside.pointerAt(QPointF(8, 500), Wide, WorkArea), PeekSide::Right);
        QCOMPARE(aside.pointerAt(QPointF(800, 500), Wide, WorkArea), std::nullopt);
        QCOMPARE(aside.pointerAt(QPointF(1700, 500), Wide, WorkArea), std::nullopt);
        QCOMPARE(aside.pointerAt(QPointF(8, 500), Wide, WorkArea), std::nullopt);

        aside.begin(true);
        QCOMPARE(aside.pointerAt(QPointF(1600, 500), Wide, WorkArea), PeekSide::Left);
    }

    void startingOutsideWaitsForTheWindowToBeEntered()
    {
        PeekAside aside;
        aside.begin(false);
        QCOMPARE(aside.pointerAt(QPointF(100, 500), Narrow, WorkArea), std::nullopt);
        QCOMPARE(aside.pointerAt(QPointF(1500, 500), Narrow, WorkArea), std::nullopt);
        QCOMPARE(aside.pointerAt(QPointF(900, 500), Narrow, WorkArea), std::nullopt);
        QCOMPARE(aside.pointerAt(QPointF(1500, 500), Narrow, WorkArea), PeekSide::Left);
    }

    void leavingThroughTheTopOrBottomUsesThePointersHalf()
    {
        PeekAside aside;
        aside.begin(true);
        QCOMPARE(aside.pointerAt(QPointF(800, 4), Wide, WorkArea), PeekSide::Right);
        aside.begin(true);
        QCOMPARE(aside.pointerAt(QPointF(1100, 1035), Wide, WorkArea), PeekSide::Left);
    }

    void wideWindowsShrinkToHalfAndNarrowOnesKeepTheirWidth()
    {
        const double half = (1920 - 3 * Gap) / 2;
        QCOMPARE(Layout::peekFrame(Wide, WorkArea, PeekSide::Right, 0, Gap), QRectF(1920 - Gap - half, 16, half, 1008));
        QCOMPARE(Layout::peekFrame(Wide, WorkArea, PeekSide::Left, 0, Gap), QRectF(Gap, 16, half, 1008));
        QCOMPARE(Layout::peekFrame(Narrow, WorkArea, PeekSide::Right, 0, Gap), QRectF(1920 - Gap - 500, 16, 500, 1008));
        QCOMPARE(Layout::peekFrame(Narrow, WorkArea, PeekSide::Left, 0, Gap), QRectF(Gap, 16, 500, 1008));
        QCOMPARE(Layout::peekFrame(Wide, WorkArea, PeekSide::Right, 1200, Gap), QRectF(1920 - Gap - 1200, 16, 1200, 1008));
        QCOMPARE(Layout::peekFrame(Wide, WorkArea, PeekSide::Right, 2400, Gap), QRectF(0, 16, 1920, 1008));
        const QRectF offset(1920, 30, 1920, 1050);
        QCOMPARE(Layout::peekFrame(QRectF(1936, 46, 1800, 1000), offset, PeekSide::Left, 0, 0), QRectF(1920, 46, 960, 1000));
    }
};

QTEST_MAIN(TestLayoutDesktopPeek)
#include "test_layout_desktoppeek.moc"
