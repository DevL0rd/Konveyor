#include "tabhelpers.h"

using namespace LayoutTest;

namespace
{

Config::Config animatedConfig()
{
    Config::Config config;
    config.animations.enabled = true;
    config.layout.alwaysExpandSingleColumn = false;
    return config;
}

struct NativeWorkspaces
{
    Fixture fixture {animatedConfig()};
    Layout::WindowId first = 0;
    Layout::WindowId second = 0;

    NativeWorkspaces()
    {
        fixture.engine().setNativeDesktopPresentation(true);
        first = fixture.add();
        fixture.advance(4000);
        fixture.perform(QStringLiteral("focus-workspace-down"));
        second = fixture.add();
        fixture.advance(4000);
        fixture.perform(QStringLiteral("focus-workspace-up"));
        fixture.advance(4000);
    }
};

}

class TestLayoutNativeDesktops : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void workspaceSwitchKeepsBothDesktopsLocal_data()
    {
        QTest::addColumn<bool>("external");
        QTest::newRow("konveyor-action") << false;
        QTest::newRow("kde-desktop") << true;
    }

    void workspaceSwitchKeepsBothDesktopsLocal()
    {
        QFETCH(bool, external);
        NativeWorkspaces row;
        Fixture &fixture = row.fixture;
        const auto first = row.first;
        const auto second = row.second;
        const QRectF firstFrame = fixture.frame(first);
        const QRectF secondFrame = fixture.frame(second);
        QVERIFY(fixture.state(first).visible);
        QVERIFY(!fixture.state(second).visible);
        QCOMPARE(firstFrame.y(), secondFrame.y());

        if (external) {
            fixture.engine().focusWorkspace(QStringLiteral("DP-1"), 2);
            fixture.settle();
            for (int step : {0, 50, 4000}) {
                fixture.advance(step);
                QCOMPARE(fixture.state(first).renderFrame, firstFrame);
                QCOMPARE(fixture.state(first).targetFrame, firstFrame);
                QCOMPARE(fixture.state(second).renderFrame, secondFrame);
                QCOMPARE(fixture.state(second).targetFrame, secondFrame);
                QVERIFY(!fixture.state(first).visible);
                QVERIFY(fixture.state(second).visible);
                QCOMPARE(fixture.engine().windowAt(secondFrame.center()), std::optional(second));
                QCOMPARE(fixture.engine().outputStates().first().transitionProgress, 1.0);
            }
        } else {
            fixture.perform(QStringLiteral("focus-workspace-down"));
            QVERIFY(fixture.engine().isWorkspaceTransitioning());
            fixture.advance(50);
            QVERIFY(fixture.state(first).renderFrame.y() != firstFrame.y());
            QVERIFY(fixture.state(second).renderFrame.y() != secondFrame.y());
            fixture.advance(4000);
            QVERIFY(!fixture.engine().isWorkspaceTransitioning());
            QCOMPARE(fixture.state(first).renderFrame, fixture.state(first).targetFrame);
            QCOMPARE(fixture.state(second).renderFrame, fixture.state(second).targetFrame);
            QVERIFY(!fixture.state(first).visible);
            QVERIFY(fixture.state(second).visible);
            QCOMPARE(fixture.engine().windowAt(secondFrame.center()), std::optional(second));
            QCOMPARE(fixture.engine().outputStates().first().transitionProgress, 1.0);
        }
        fixture.setConfig(animatedConfig());
        QCOMPARE(fixture.state(first).targetFrame, firstFrame);
        fixture.engine().focusWorkspace(QStringLiteral("DP-1"), 1);
        QCOMPARE(fixture.state(first).renderFrame, firstFrame);
        QCOMPARE(fixture.state(second).renderFrame, secondFrame);
        VERIFY_INVARIANTS(fixture);
    }

    void tabIndicatorsFollowTheVisibleNativeDesktop()
    {
        Config::Config config = animatedConfig();
        config.layout.defaultColumnDisplay = Config::ColumnDisplay::Tabbed;
        Fixture fixture(config);
        fixture.engine().setNativeDesktopPresentation(true);
        const auto tabs = addTabs(fixture, 2);
        fixture.advance(4000);
        const auto tab = tabs.last();
        const QList<QRectF> rects = fixture.state(tab).tabBar.tabRects;
        QVERIFY(fixture.state(tab).tabBar.visible);
        fixture.perform(QStringLiteral("focus-workspace-down"));
        fixture.advance(50);
        QVERIFY(fixture.state(tab).visible);
        QVERIFY(fixture.state(tab).tabBar.visible);
        fixture.advance(4000);
        QVERIFY(!fixture.state(tab).visible);
        QVERIFY(!fixture.state(tab).tabBar.visible);
        fixture.perform(QStringLiteral("focus-workspace-up"));
        fixture.advance(4000);
        QCOMPARE(fixture.state(tab).tabBar.tabRects, rects);
        QVERIFY(fixture.state(tab).tabBar.visible);
        VERIFY_INVARIANTS(fixture);
    }

    void floatingFrameFeedbackUsesTheNativeDesktopOrigin()
    {
        Fixture fixture;
        fixture.engine().setNativeDesktopPresentation(true);
        fixture.add();
        fixture.perform(QStringLiteral("focus-workspace-down"));
        const auto floating = fixture.add();
        fixture.perform(QStringLiteral("toggle-window-floating"));
        fixture.perform(QStringLiteral("focus-workspace-up"));
        fixture.advance(1);
        const QRectF requested(200, 150, 400, 300);
        fixture.engine().setFloatingFrame(floating, requested);
        fixture.settle();
        QCOMPARE(fixture.state(floating).targetFrame, requested);
        QVERIFY(!fixture.state(floating).visible);
        fixture.perform(QStringLiteral("focus-workspace-down"));
        fixture.advance(1);
        QCOMPARE(fixture.state(floating).renderFrame, requested);
        VERIFY_INVARIANTS(fixture);
    }

    void workspaceGestureKeepsItsMotionAndSettlingAnimation_data()
    {
        QTest::addColumn<bool>("touchpad");
        QTest::newRow("touchpad") << true;
        QTest::newRow("touchscreen") << false;
    }

    void workspaceGestureKeepsItsMotionAndSettlingAnimation()
    {
        QFETCH(bool, touchpad);
        Fixture fixture(animatedConfig());
        fixture.engine().setNativeDesktopPresentation(true);
        const auto id = fixture.add();
        fixture.advance(4000);
        const QRectF local = fixture.frame(id);
        fixture.engine().beginWorkspaceSwipe(QStringLiteral("DP-1"), touchpad);
        fixture.engine().updateWorkspaceSwipe(120.0, 4001, touchpad);
        QVERIFY(fixture.state(id).renderFrame.y() != local.y());
        QVERIFY(fixture.state(id).visible);
        fixture.advance(200);
        fixture.engine().endWorkspaceSwipe(touchpad);
        QVERIFY(fixture.state(id).renderFrame.y() != fixture.state(id).targetFrame.y());
        fixture.advance(4000);
        QCOMPARE(fixture.state(id).renderFrame, local);
        QCOMPARE(fixture.state(id).targetFrame, local);
        VERIFY_INVARIANTS(fixture);
    }

    void workspaceTransitionQueryTracksGestureAndSettle()
    {
        NativeWorkspaces row;
        Fixture &fixture = row.fixture;
        QVERIFY(!fixture.engine().isWorkspaceTransitioning());

        fixture.engine().beginWorkspaceSwipe(QStringLiteral("DP-1"), true);
        QVERIFY(fixture.engine().isWorkspaceTransitioning());

        fixture.engine().updateWorkspaceSwipe(-120.0, fixture.elapsed() + 1, true);
        QVERIFY(fixture.engine().isWorkspaceTransitioning());

        fixture.advance(200);
        fixture.engine().endWorkspaceSwipe(true);
        QVERIFY(fixture.engine().isWorkspaceTransitioning());

        fixture.advance(4000);
        QVERIFY(!fixture.engine().isWorkspaceTransitioning());
        VERIFY_INVARIANTS(fixture);
    }

    void keyboardDesktopSwitchRunsOnePaintedTransition()
    {
        Fixture fixture(animatedConfig());
        fixture.engine().setNativeDesktopPresentation(true);
        const auto id = fixture.add();
        fixture.advance(4000);
        QVERIFY(!fixture.engine().isWorkspaceTransitioning());

        fixture.perform(QStringLiteral("focus-workspace-down"));
        QVERIFY(fixture.engine().isWorkspaceTransitioning());
        fixture.advance(50);
        QVERIFY(fixture.state(id).renderFrame.y() != fixture.state(id).targetFrame.y());

        fixture.advance(4000);
        QVERIFY(!fixture.engine().isWorkspaceTransitioning());
        QCOMPARE(fixture.state(id).renderFrame.y(), fixture.state(id).targetFrame.y());
        QCOMPARE(fixture.state(id).workspaceIndex, 1);
        QVERIFY(!fixture.state(id).onActiveWorkspace);
        VERIFY_INVARIANTS(fixture);
    }

    void movingWindowToTheNextDesktopPaintsAndPrunesTheSource()
    {
        NativeWorkspaces row;
        Fixture &fixture = row.fixture;
        const auto id = row.first;
        QCOMPARE(fixture.state(id).workspaceIndex, 1);
        const QRectF before = fixture.state(id).renderFrame;

        QVERIFY(fixture.perform(QStringLiteral("move-window-to-workspace-down")).ok);
        QVERIFY(fixture.engine().isWorkspaceTransitioning());
        fixture.advance(50);
        QVERIFY(fixture.state(id).renderFrame != before);
        QVERIFY(fixture.state(id).renderFrame != fixture.state(id).targetFrame);

        fixture.advance(4000);
        QVERIFY(!fixture.engine().isWorkspaceTransitioning());
        // The source desktop is empty now, so pruning collapses it and the
        // destination takes its place at the front of the stack.
        QCOMPARE(fixture.state(id).workspaceIndex, 1);
        QVERIFY(fixture.state(id).onActiveWorkspace);
        QCOMPARE(fixture.state(id).renderFrame, fixture.state(id).targetFrame);
        VERIFY_INVARIANTS(fixture);
    }

    void externalDesktopSwitchAdoptsImmediately()
    {
        NativeWorkspaces row;
        Fixture &fixture = row.fixture;
        const QRectF secondFrame = fixture.frame(row.second);
        fixture.engine().focusWorkspace(QStringLiteral("DP-1"), 2);
        QVERIFY(!fixture.engine().isWorkspaceTransitioning());
        QCOMPARE(fixture.engine().outputStates().first().transitionProgress, 1.0);
        QCOMPARE(fixture.state(row.second).workspaceIndex, 2);
        QCOMPARE(fixture.state(row.second).renderFrame, secondFrame);
        VERIFY_INVARIANTS(fixture);
    }

    void swipeAfterDesktopSwitchStartsAtTheVisibleWorkspace()
    {
        NativeWorkspaces row;
        Fixture &fixture = row.fixture;
        const auto id = row.second;
        fixture.perform(QStringLiteral("focus-workspace-down"));
        const QRectF frame = fixture.state(id).renderFrame;
        fixture.engine().beginWorkspaceSwipe(QStringLiteral("DP-1"), true);
        QCOMPARE(fixture.state(id).renderFrame, frame);
        fixture.engine().updateWorkspaceSwipe(-60.0, fixture.elapsed() + 1, true);
        QVERIFY(fixture.state(id).renderFrame.y() > frame.y());
        fixture.engine().endWorkspaceSwipe(true);
        fixture.advance(4000);
        VERIFY_INVARIANTS(fixture);
    }

    void dragAfterDesktopSwitchTargetsTheVisibleWorkspace()
    {
        NativeWorkspaces row;
        Fixture &fixture = row.fixture;
        const auto id = row.second;
        const auto workspace = fixture.state(id).workspace;
        fixture.perform(QStringLiteral("focus-workspace-down"));
        fixture.advance(4000);
        const QPointF start = fixture.frame(id).center();
        QVERIFY(fixture.engine().beginWindowDrag(id, start));
        fixture.engine().updateWindowDrag(start + QPointF(300, 0), QStringLiteral("DP-1"));
        QCOMPARE(fixture.engine().movingWindow(), std::optional(id));
        fixture.engine().endWindowDrag();
        fixture.settle();
        QCOMPARE(fixture.state(id).workspace, workspace);
        VERIFY_INVARIANTS(fixture);
    }

    void overviewKeepsInactiveWindowsInTheirLocalFrame()
    {
        Fixture fixture(animatedConfig());
        fixture.engine().setNativeDesktopPresentation(true);
        const auto id = fixture.add();
        fixture.advance(4000);
        fixture.perform(QStringLiteral("focus-workspace-down"));
        fixture.advance(50);
        QVERIFY(fixture.state(id).visible);
        fixture.advance(4000);
        QVERIFY(!fixture.state(id).visible);
        fixture.engine().setOverviewOpen(true);
        QCOMPARE(fixture.state(id).renderFrame.y(), 16.0);
        QCOMPARE(fixture.state(id).targetFrame.y(), 16.0);
        QVERIFY(!fixture.state(id).visible);
        fixture.engine().setOverviewOpen(false);
        QCOMPARE(fixture.state(id).renderFrame.y(), 16.0);
        VERIFY_INVARIANTS(fixture);
    }

    void overviewAfterDesktopSwitchStartsAtTheVisibleWorkspace_data()
    {
        QTest::addColumn<QString>("action");
        QTest::newRow("kde") << QString();
        QTest::newRow("open-overview") << QStringLiteral("open-overview");
        QTest::newRow("toggle-overview") << QStringLiteral("toggle-overview");
    }

    void overviewAfterDesktopSwitchStartsAtTheVisibleWorkspace()
    {
        QFETCH(QString, action);
        NativeWorkspaces row;
        Fixture &fixture = row.fixture;
        const auto id = row.second;
        fixture.perform(QStringLiteral("focus-workspace-down"));
        fixture.advance(4000);
        const QRectF frame = fixture.state(id).renderFrame;
        if (action.isEmpty()) {
            fixture.engine().setOverviewOpen(true);
        } else {
            QVERIFY(fixture.perform(action).ok);
        }
        QCOMPARE(fixture.state(id).renderFrame, frame);
        QCOMPARE(fixture.engine().outputStates().first().transitionProgress, 1.0);
        fixture.engine().setOverviewOpen(false);
        VERIFY_INVARIANTS(fixture);
    }
};

QTEST_GUILESS_MAIN(TestLayoutNativeDesktops)
#include "test_layout_native_desktops.moc"
