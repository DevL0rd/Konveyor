#include "helpers.h"

#include "layout/gestures/gesturerouter.h"

using namespace LayoutTest;

namespace
{

const QString Output = QStringLiteral("DP-1");

int activeWorkspaceIndex(Layout::Engine &engine)
{
    for (const Layout::WorkspaceState &state : engine.workspaceStates()) {
        if (state.isActive) {
            return state.index;
        }
    }
    return 0;
}

Config::Config wideColumns()
{
    Config::Config config = instantConfig();
    config.layout.defaultColumnWidth = Config::PresetSize(Config::Fixed {900});
    return config;
}

struct Rig
{
    explicit Rig(const Config::Config &config = wideColumns())
        : fixture(config)
        , router(fixture.engine())
    {
        router.setConfig(config.gestures);
    }

    Layout::WindowId threeColumnsFocusedFirst()
    {
        fixture.add(QStringLiteral("a"));
        fixture.add(QStringLiteral("b"));
        fixture.add(QStringLiteral("c"));
        fixture.perform(QStringLiteral("focus-column-first"));
        return *fixture.focused();
    }

    void threeFingerTouch(QPointF start, qint64 time = 0)
    {
        for (qint32 id = 0; id < 3; ++id) {
            router.touchDown(id, start + QPointF(id * 40.0, 0.0), time, Output);
        }
    }

    void moveTouches(QPointF delta, qint64 time, int count = 3)
    {
        for (qint32 id = 0; id < count; ++id) {
            positions[id] += delta;
            router.touchMotion(id, QPointF(100.0 + id * 40.0, 500.0) + positions[id], time);
        }
    }

    void fourFingerTouchSwipe(QPointF origin, QPointF delta, int steps)
    {
        for (qint32 id = 0; id < 4; ++id) {
            router.touchDown(id, origin + QPointF((id - 1.5) * 40.0, 0.0), 0, Output);
        }
        for (int step = 1; step <= steps; ++step) {
            for (qint32 id = 0; id < 4; ++id) {
                router.touchMotion(id, origin + QPointF((id - 1.5) * 40.0, 0.0) + delta * step / steps, step * 16);
            }
        }
        for (qint32 id = 0; id < 4; ++id) {
            router.touchUp(id, 1000);
        }
        fixture.settle();
    }

    std::pair<Layout::WindowId, Layout::WindowId> twoRowColumn()
    {
        const Layout::WindowId top = fixture.add(QStringLiteral("a"));
        const Layout::WindowId bottom = fixture.add(QStringLiteral("b"));
        fixture.perform(QStringLiteral("consume-or-expel-window-left"));
        fixture.settle();
        return {top, bottom};
    }

    Fixture fixture;
    Layout::GestureRouter router;
    QHash<qint32, QPointF> positions;
};

}

class TestLayoutTouch : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void touchpadThreeFingerSwipeScrollsTheRow()
    {
        Rig rig;
        const auto first = rig.threeColumnsFocusedFirst();

        QVERIFY(rig.router.touchpadSwipeBegin(3, Output));
        QVERIFY(rig.router.touchpadSwipeUpdate(QPointF(-40.0, 2.0), 10));
        QVERIFY(rig.router.touchpadSwipeUpdate(QPointF(-1200.0, 0.0), 20));
        QVERIFY(rig.router.touchpadSwipeEnd());
        rig.fixture.settle();
        QVERIFY(rig.fixture.focused() != first);
        VERIFY_INVARIANTS(rig.fixture);
    }

    void touchpadWrongFingerCountIsLeftAlone()
    {
        Rig rig;
        rig.fixture.add();
        QVERIFY(!rig.router.touchpadSwipeBegin(5, Output));
        QVERIFY(!rig.router.touchpadSwipeUpdate(QPointF(-400.0, 0.0), 10));
        QVERIFY(!rig.router.touchpadSwipeEnd());
    }

    void touchpadVerticalSwipeSwitchesWorkspace()
    {
        Rig rig;
        rig.fixture.add();
        QCOMPARE(activeWorkspaceIndex(rig.fixture.engine()), 1);
        QVERIFY(rig.router.touchpadSwipeBegin(3, Output));
        rig.router.touchpadSwipeUpdate(QPointF(0.0, -30.0), 10);
        rig.router.touchpadSwipeUpdate(QPointF(0.0, -300.0), 20);
        rig.router.touchpadSwipeEnd();
        rig.fixture.advance(1);
        QCOMPARE(activeWorkspaceIndex(rig.fixture.engine()), 2);
        VERIFY_INVARIANTS(rig.fixture);
    }

    void swipeBackFromAnEmptyWorkspace()
    {
        Rig rig;
        rig.fixture.add();
        rig.fixture.perform(QStringLiteral("focus-workspace-down"));
        rig.fixture.advance(1);
        QCOMPARE(activeWorkspaceIndex(rig.fixture.engine()), 2);
        QVERIFY(rig.router.touchpadSwipeBegin(3, Output));
        rig.router.touchpadSwipeUpdate(QPointF(0.0, 30.0), 10);
        rig.router.touchpadSwipeUpdate(QPointF(0.0, 300.0), 20);
        rig.router.touchpadSwipeEnd();
        rig.fixture.advance(1);
        QCOMPARE(activeWorkspaceIndex(rig.fixture.engine()), 1);
    }

    void unnaturalSwipeGoesTheOtherWay()
    {
        Config::Config config = wideColumns();
        config.gestures.touchpad.naturalSwipe = false;
        Rig rig(config);
        rig.fixture.add();
        rig.fixture.perform(QStringLiteral("focus-workspace-down"));
        rig.fixture.add();
        rig.fixture.advance(1);
        const int start = activeWorkspaceIndex(rig.fixture.engine());
        rig.router.touchpadSwipeBegin(3, Output);
        rig.router.touchpadSwipeUpdate(QPointF(0.0, -30.0), 10);
        rig.router.touchpadSwipeUpdate(QPointF(0.0, -300.0), 20);
        rig.router.touchpadSwipeEnd();
        rig.fixture.advance(1);
        QCOMPARE(activeWorkspaceIndex(rig.fixture.engine()), start - 1);
    }

    void disabledDirectionPassesThrough()
    {
        Config::Config config = wideColumns();
        config.gestures.touchpad.verticalSwipe = Config::VerticalSwipe::Off;
        Rig rig(config);
        rig.fixture.add();
        QVERIFY(rig.router.touchpadSwipeBegin(3, Output));
        rig.router.touchpadSwipeUpdate(QPointF(0.0, -30.0), 10);
        QVERIFY(!rig.router.touchpadSwipeUpdate(QPointF(0.0, -300.0), 20));
        QVERIFY(!rig.router.touchpadSwipeEnd());
        rig.fixture.advance(1);
        QCOMPARE(activeWorkspaceIndex(rig.fixture.engine()), 1);
    }

    void touchpadFourFingerSwipeMergesColumns()
    {
        Rig rig;
        const auto a = rig.fixture.add(QStringLiteral("a"));
        const auto b = rig.fixture.add(QStringLiteral("b"));
        QCOMPARE(rig.fixture.state(b).columnIndex, 1);
        QVERIFY(rig.router.touchpadSwipeBegin(4, Output));
        QVERIFY(rig.router.touchpadSwipeUpdate(QPointF(-30.0, 0.0), 10));
        QVERIFY(rig.router.touchpadSwipeUpdate(QPointF(-150.0, 0.0), 20));
        QVERIFY(rig.router.touchpadSwipeEnd());
        rig.fixture.settle();
        QCOMPARE(rig.fixture.state(b).columnIndex, rig.fixture.state(a).columnIndex);
        QCOMPARE(rig.fixture.state(b).tileIndex, 1);

        QVERIFY(rig.router.touchpadSwipeBegin(4, Output));
        rig.router.touchpadSwipeUpdate(QPointF(30.0, 0.0), 10);
        rig.router.touchpadSwipeUpdate(QPointF(150.0, 0.0), 20);
        rig.router.touchpadSwipeEnd();
        rig.fixture.settle();
        QCOMPARE(rig.fixture.state(b).columnIndex, rig.fixture.state(a).columnIndex + 1);
        VERIFY_INVARIANTS(rig.fixture);
    }

    void touchscreenWindowSwipesRepeat()
    {
        Rig rig;
        const auto a = rig.fixture.add(QStringLiteral("a"));
        const auto b = rig.fixture.add(QStringLiteral("b"));
        rig.fourFingerTouchSwipe(QPointF(1060.0, 600.0), QPointF(-400.0, 0.0), 30);
        QCOMPARE(rig.fixture.state(b).columnIndex, rig.fixture.state(a).columnIndex);
        rig.fourFingerTouchSwipe(QPointF(1060.0, 600.0), QPointF(400.0, 0.0), 30);
        QCOMPARE(rig.fixture.state(b).columnIndex, rig.fixture.state(a).columnIndex + 1);
    }

    void fourFingerVerticalSwipeCarriesTheWindowToAnotherWorkspace()
    {
        Rig rig;
        rig.fixture.add(QStringLiteral("a"));
        const auto b = rig.fixture.add(QStringLiteral("b"));
        const int start = rig.fixture.state(b).workspaceIndex;
        rig.fourFingerTouchSwipe(rig.fixture.frame(b).center(), QPointF(0.0, 400.0), 8);
        QCOMPARE(rig.fixture.state(b).workspaceIndex, start + 1);
        VERIFY_INVARIANTS(rig.fixture);
    }

    void touchpadFourFingerVerticalSwipeMovesThroughTheColumnFirst()
    {
        Rig rig;
        const auto [a, b] = rig.twoRowColumn();
        rig.fixture.perform(QStringLiteral("focus-window-up"));
        rig.fixture.settle();
        QCOMPARE(rig.fixture.focused(), std::optional(a));
        QCOMPARE(rig.fixture.state(a).tileIndex, 0);
        const int start = rig.fixture.state(a).workspaceIndex;
        const auto swipeDown = [&rig] {
            QVERIFY(rig.router.touchpadSwipeBegin(4, Output));
            QVERIFY(rig.router.touchpadSwipeUpdate(QPointF(0.0, 30.0), 10));
            QVERIFY(rig.router.touchpadSwipeUpdate(QPointF(0.0, 150.0), 20));
            QVERIFY(rig.router.touchpadSwipeEnd());
            rig.fixture.settle();
        };
        swipeDown();
        QCOMPARE(rig.fixture.state(a).workspaceIndex, start);
        QCOMPARE(rig.fixture.state(a).columnIndex, rig.fixture.state(b).columnIndex);
        QCOMPARE(rig.fixture.state(a).tileIndex, 1);
        QCOMPARE(rig.fixture.state(b).tileIndex, 0);
        swipeDown();
        QCOMPARE(rig.fixture.state(a).workspaceIndex, start + 1);
        QCOMPARE(rig.fixture.state(b).workspaceIndex, start);
        VERIFY_INVARIANTS(rig.fixture);
    }

    void touchscreenFourFingerVerticalSwipeMovesTheWindowUnderTheFingers()
    {
        Rig rig;
        const auto [a, b] = rig.twoRowColumn();
        QCOMPARE(rig.fixture.focused(), std::optional(b));
        QCOMPARE(rig.fixture.state(a).tileIndex, 0);
        const int start = rig.fixture.state(a).workspaceIndex;
        rig.fourFingerTouchSwipe(rig.fixture.frame(a).center() - QPointF(0.0, 100.0), QPointF(0.0, 400.0), 20);
        QCOMPARE(rig.fixture.state(a).workspaceIndex, start);
        QCOMPARE(rig.fixture.state(a).columnIndex, rig.fixture.state(b).columnIndex);
        QCOMPARE(rig.fixture.state(a).tileIndex, 1);
        QCOMPARE(rig.fixture.state(b).tileIndex, 0);
        rig.fourFingerTouchSwipe(rig.fixture.frame(a).center() - QPointF(0.0, 150.0), QPointF(0.0, 400.0), 20);
        QCOMPARE(rig.fixture.state(a).workspaceIndex, start + 1);
        QCOMPARE(rig.fixture.state(b).workspaceIndex, start);
        VERIFY_INVARIANTS(rig.fixture);
    }

    void touchpadPinchTogglesTheOverview()
    {
        Rig rig;
        rig.fixture.add();
        QVERIFY(!rig.router.touchpadPinchBegin(3));
        QVERIFY(rig.router.touchpadPinchBegin(4));
        rig.router.touchpadPinchUpdate(0.9);
        QVERIFY(!rig.fixture.engine().isOverviewOpen());
        rig.router.touchpadPinchUpdate(0.7);
        QVERIFY(rig.fixture.engine().isOverviewOpen());
        rig.router.touchpadPinchUpdate(1.5);
        QVERIFY(rig.fixture.engine().isOverviewOpen());
        QVERIFY(rig.router.touchpadPinchEnd());
        QVERIFY(rig.router.touchpadPinchBegin(4));
        rig.router.touchpadPinchUpdate(1.4);
        QVERIFY(!rig.fixture.engine().isOverviewOpen());
    }

    void touchscreenThreeFingerSwipeScrollsTheRow()
    {
        Rig rig;
        const auto first = rig.threeColumnsFocusedFirst();

        QVERIFY(!rig.router.touchDown(0, QPointF(100, 500), 0, Output));
        QVERIFY(!rig.router.touchDown(1, QPointF(140, 500), 0, Output));
        QVERIFY(rig.router.touchDown(2, QPointF(180, 500), 0, Output));
        QVERIFY(rig.router.isTouchGestureActive());
        for (int step = 1; step <= 10; ++step) {
            rig.moveTouches(QPointF(-150.0, 0.0), step * 10);
        }
        QVERIFY(rig.router.touchUp(2, 1000));
        QVERIFY(!rig.router.isTouchGestureActive());
        QVERIFY(rig.router.touchUp(1, 1000));
        QVERIFY(rig.router.touchUp(0, 1000));
        rig.fixture.settle();
        QVERIFY(rig.fixture.focused() != first);
        VERIFY_INVARIANTS(rig.fixture);
    }

    void staleTouchesAreClearedForANewSequence()
    {
        Rig rig;
        rig.fixture.add();
        rig.router.touchDown(7, QPointF(100, 100), 0, Output);
        rig.router.touchDown(8, QPointF(140, 100), 0, Output);
        QCOMPARE(rig.router.touchPointCount(), 2);
        QVERIFY(!rig.router.resetTouches());
        QCOMPARE(rig.router.touchPointCount(), 0);
        rig.threeFingerTouch(QPointF(100, 500));
        QVERIFY(rig.router.isTouchGestureActive());
    }

    void touchscreenSingleFingerIsForApps()
    {
        Rig rig;
        rig.fixture.add();
        QVERIFY(!rig.router.touchDown(0, QPointF(100, 100), 0, Output));
        QVERIFY(!rig.router.touchMotion(0, QPointF(300, 100), 10));
        QVERIFY(!rig.router.touchUp(0, 1000));
        QCOMPARE(rig.router.lastTouchPosition(), std::optional(QPointF(300, 100)));
    }

    void touchscreenFourFingerPinchOpensTheOverview()
    {
        Rig rig;
        rig.fixture.add();
        const QList<QPointF> start {QPointF(400, 400), QPointF(600, 400), QPointF(400, 600), QPointF(600, 600)};
        for (qint32 id = 0; id < 4; ++id) {
            rig.router.touchDown(id, start[id], 0, Output);
        }
        const QPointF center(500, 500);
        for (qint32 id = 0; id < 4; ++id) {
            rig.router.touchMotion(id, center + (start[id] - center) * 0.5, 10);
        }
        QVERIFY(rig.fixture.engine().isOverviewOpen());
        rig.router.touchCancel();
        QVERIFY(!rig.router.isTouchGestureActive());
    }

    void longPressNeedsTheFingerHeldStillFirst()
    {
        Rig rig;
        rig.router.touchDown(0, QPointF(100, 100), 1000, Output);
        QVERIFY(!rig.router.isLongPress(1200));
        QVERIFY(rig.router.isLongPress(1600));
        rig.router.touchMotion(0, QPointF(140, 100), 1650);
        QVERIFY(rig.router.hasFirstTouchMoved());
        QVERIFY(rig.router.isLongPress(1700));
        rig.router.touchUp(0, 1000);

        rig.router.touchDown(0, QPointF(100, 100), 3000, Output);
        rig.router.touchMotion(0, QPointF(160, 100), 3100);
        QVERIFY(!rig.router.isLongPress(3800));
        rig.router.touchUp(0, 1000);

        Config::Config off = wideColumns();
        off.gestures.touchscreen.longPressToMove = false;
        Rig disabled(off);
        disabled.router.touchDown(0, QPointF(100, 100), 0, Output);
        QVERIFY(!disabled.router.isLongPress(2000));
    }

    void disabledTouchscreenIgnoresEverything()
    {
        Config::Config config = wideColumns();
        config.gestures.touchscreen.enabled = false;
        Rig rig(config);
        rig.fixture.add();
        rig.threeFingerTouch(QPointF(100, 500));
        QVERIFY(!rig.router.isTouchGestureActive());
    }
};

QTEST_GUILESS_MAIN(TestLayoutTouch)
#include "test_layout_touch.moc"
