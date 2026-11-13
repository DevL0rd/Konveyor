#include "helpers.h"

#include <utility>

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

void addPresentationModes()
{
    QTest::addColumn<bool>("nativeDesktops");
    QTest::newRow("standalone") << false;
    QTest::newRow("native-desktops") << true;
}

Config::Config ringConfig(Config::FocusRingTransition transition)
{
    Config::Config config = animatedConfig();
    config.layout.focusRingTransition = transition;
    config.layout.focusRing.enabled = true;
    return config;
}

std::pair<Layout::WindowId, Layout::WindowId> twoWorkspaces(Fixture &fixture)
{
    const Layout::WindowId first = fixture.add();
    fixture.advance(2000);
    fixture.perform(QStringLiteral("focus-workspace-down"));
    fixture.advance(4000);
    const Layout::WindowId second = fixture.add();
    fixture.perform(QStringLiteral("focus-workspace-up"));
    fixture.advance(4000);
    return {first, second};
}

void expectSwitchSettled(Fixture &fixture, Layout::WindowId first, Layout::WindowId second)
{
    QVERIFY(!fixture.engine().isWorkspaceTransitioning());
    QCOMPARE(fixture.state(first).renderFrame, fixture.state(first).targetFrame);
    QCOMPARE(fixture.state(second).renderFrame, fixture.state(second).targetFrame);
}

}

class TestLayoutAnimations : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void openAnimationFadesIn_data() { addPresentationModes(); }

    void openAnimationFadesIn()
    {
        QFETCH(bool, nativeDesktops);
        Fixture fixture(animatedConfig());
        fixture.engine().setNativeDesktopPresentation(nativeDesktops);
        const auto id = fixture.add();
        QVERIFY(fixture.state(id).renderAlpha < 1.0);
        QVERIFY(fixture.engine().isAnimating());
        fixture.advance(2000);
        QCOMPARE(fixture.state(id).renderAlpha, 1.0);
        QVERIFY(!fixture.engine().isAnimating());
        VERIFY_INVARIANTS(fixture);
    }

    void viewOffsetAnimatesToNewColumn_data() { addPresentationModes(); }

    void viewOffsetAnimatesToNewColumn()
    {
        QFETCH(bool, nativeDesktops);
        Config::Config config = animatedConfig();
        config.layout.defaultColumnWidth = Config::PresetSize(Config::Fixed {1500});
        Fixture fixture(config);
        fixture.engine().setNativeDesktopPresentation(nativeDesktops);
        fixture.add(QStringLiteral("a"));
        const auto second = fixture.add(QStringLiteral("b"));
        fixture.advance(2000);
        const double settled = fixture.state(second).renderFrame.x();
        QCOMPARE(settled, fixture.state(second).targetFrame.x());

        fixture.perform(QStringLiteral("focus-column-left"));
        QVERIFY(fixture.engine().isAnimating());
        QVERIFY(fixture.state(second).renderFrame.x() != fixture.state(second).targetFrame.x());
        fixture.advance(2000);
        QCOMPARE(fixture.state(second).renderFrame.x(), fixture.state(second).targetFrame.x());
        QVERIFY(!fixture.engine().isAnimating());
        VERIFY_INVARIANTS(fixture);
    }

    void windowMovementAnimates_data() { addPresentationModes(); }

    void windowMovementAnimates()
    {
        QFETCH(bool, nativeDesktops);
        Fixture fixture(animatedConfig());
        fixture.engine().setNativeDesktopPresentation(nativeDesktops);
        const auto first = fixture.add(QStringLiteral("a"));
        fixture.add(QStringLiteral("b"));
        fixture.advance(2000);
        const double before = fixture.state(first).renderFrame.x();
        fixture.perform(QStringLiteral("move-column-left"));
        QVERIFY(fixture.state(first).renderFrame.x() != fixture.state(first).targetFrame.x());
        fixture.advance(2000);
        QCOMPARE(fixture.state(first).renderFrame.x(), fixture.state(first).targetFrame.x());
        QVERIFY(fixture.state(first).renderFrame.x() != before);
        VERIFY_INVARIANTS(fixture);
    }

    void resizeAnimatesOnlyForLargeChanges_data() { addPresentationModes(); }

    void resizeAnimatesOnlyForLargeChanges()
    {
        QFETCH(bool, nativeDesktops);
        Fixture fixture(animatedConfig());
        fixture.engine().setNativeDesktopPresentation(nativeDesktops);
        const auto id = fixture.add();
        fixture.advance(2000);
        fixture.perform(QStringLiteral("set-column-width"), {QStringLiteral("400")});
        QVERIFY(fixture.state(id).renderFrame.width() != fixture.state(id).targetFrame.width());
        fixture.advance(2000);
        QCOMPARE(fixture.state(id).renderFrame.width(), 400.0);

        fixture.perform(QStringLiteral("set-column-width"), {QStringLiteral("+5")});
        QCOMPARE(fixture.state(id).renderFrame.width(), fixture.state(id).targetFrame.width());
        VERIFY_INVARIANTS(fixture);
    }

    void keyboardSwitchPaintsItsMotion()
    {
        Fixture fixture(animatedConfig());
        const auto [first, second] = twoWorkspaces(fixture);
        const QRectF firstSettled = fixture.state(first).targetFrame;
        const QRectF secondSettled = fixture.state(second).targetFrame;
        QCOMPARE(fixture.engine().outputStates().first().transitionProgress, 0.0);

        fixture.perform(QStringLiteral("focus-workspace-down"));
        QVERIFY(fixture.engine().isWorkspaceTransitioning());
        fixture.advance(50);
        QVERIFY(fixture.state(first).renderFrame.y() != firstSettled.y());
        QVERIFY(fixture.state(second).renderFrame.y() != secondSettled.y());
        const double progress = fixture.engine().outputStates().first().transitionProgress;
        QVERIFY(progress > 0.0);
        QVERIFY(progress < 1.0);

        fixture.advance(4000);
        expectSwitchSettled(fixture, first, second);
        QCOMPARE(fixture.engine().outputStates().first().transitionProgress, 1.0);
        VERIFY_INVARIANTS(fixture);
    }

    void heldSwipeCrossfadeFollowsAndReverses()
    {
        Fixture fixture(ringConfig(Config::FocusRingTransition::Crossfade));
        const auto [first, second] = twoWorkspaces(fixture);

        fixture.engine().beginWorkspaceSwipe(QStringLiteral("DP-1"), true);
        fixture.engine().updateWorkspaceSwipe(120.0, fixture.elapsed() + 1, true);
        const double firstForward = fixture.state(first).focusRing.opacity;
        const double secondForward = fixture.state(second).focusRing.opacity;
        QVERIFY(firstForward > 0.0);
        QVERIFY(secondForward > 0.0);
        QVERIFY(firstForward > secondForward);

        fixture.engine().updateWorkspaceSwipe(-120.0, fixture.elapsed() + 2, true);
        const double firstBack = fixture.state(first).focusRing.opacity;
        QCOMPARE(firstBack, 1.0);
        QVERIFY(!fixture.state(second).focusRing.enabled);

        fixture.engine().endWorkspaceSwipe(true);
        fixture.advance(4000);
        QVERIFY(!fixture.engine().isWorkspaceTransitioning());
        QCOMPARE(fixture.state(first).focusRing.opacity, 1.0);
        QVERIFY(fixture.state(first).focusRing.enabled);
        QVERIFY(!fixture.state(second).focusRing.enabled);
        VERIFY_INVARIANTS(fixture);
    }

    void workspaceRingOpacityFollowsDistance()
    {
        using Config::FocusRingTransition;
        const auto opacity = [](FocusRingTransition mode, double distance) { return Layout::workspaceRingOpacity(mode, distance); };

        for (double distance : {0.0, 0.25, 0.5, 1.0}) {
            QCOMPARE(opacity(FocusRingTransition::Ride, distance), 1.0);
            QCOMPARE(opacity(FocusRingTransition::Delayed, distance), 0.0);
        }
        QCOMPARE(opacity(FocusRingTransition::Crossfade, 0.0), 1.0);
        QCOMPARE(opacity(FocusRingTransition::Crossfade, 0.25), 0.84375);
        QCOMPARE(opacity(FocusRingTransition::Crossfade, 0.5), 0.5);
        QCOMPARE(opacity(FocusRingTransition::Crossfade, 1.0), 0.0);
    }

    void delayedHidesRingsUntilTheSwitchSettles()
    {
        Fixture fixture(ringConfig(Config::FocusRingTransition::Delayed));
        const auto [first, second] = twoWorkspaces(fixture);

        fixture.perform(QStringLiteral("focus-workspace-down"));
        fixture.advance(50);
        QVERIFY(fixture.engine().isWorkspaceTransitioning());
        QCOMPARE(fixture.state(first).focusRing.opacity, 0.0);
        QCOMPARE(fixture.state(second).focusRing.opacity, 0.0);

        fixture.advance(4000);
        QVERIFY(!fixture.engine().isWorkspaceTransitioning());
        QVERIFY(fixture.state(second).focusRing.enabled);
        QCOMPARE(fixture.state(second).focusRing.opacity, 1.0);
        VERIFY_INVARIANTS(fixture);
    }

    void rideKeepsBothRingsDuringTheSwitch()
    {
        Fixture fixture(ringConfig(Config::FocusRingTransition::Ride));
        const auto [first, second] = twoWorkspaces(fixture);

        fixture.perform(QStringLiteral("focus-workspace-down"));
        fixture.advance(50);
        QVERIFY(fixture.engine().isWorkspaceTransitioning());
        QVERIFY(fixture.state(first).focusRing.enabled);
        QVERIFY(fixture.state(second).focusRing.enabled);
        QCOMPARE(fixture.state(first).focusRing.opacity, 1.0);
        QCOMPARE(fixture.state(second).focusRing.opacity, 1.0);
        VERIFY_INVARIANTS(fixture);
    }

    void decorationStateOpacityDefaultsAndCompares()
    {
        Layout::DecorationState decoration;
        QCOMPARE(decoration.opacity, 1.0);
        Layout::DecorationState faded = decoration;
        faded.opacity = 0.25;
        QVERIFY(!(faded == decoration));
    }

    void workspaceSwitchAnimatesVertically()
    {
        Fixture fixture(animatedConfig());
        const auto id = fixture.add();
        fixture.advance(2000);
        QCOMPARE(fixture.engine().outputStates().first().transitionProgress, 0.0);

        fixture.perform(QStringLiteral("focus-workspace-down"));
        QVERIFY(fixture.state(id).renderFrame.y() != fixture.state(id).targetFrame.y());
        fixture.advance(50);
        const double progress = fixture.engine().outputStates().first().transitionProgress;
        QVERIFY(progress > 0.0);
        QVERIFY(progress < 1.0);

        fixture.advance(4000);
        QCOMPARE(fixture.engine().outputStates().first().transitionProgress, 1.0);
        QVERIFY(!fixture.engine().isAnimating());
        VERIFY_INVARIANTS(fixture);
    }

    void windowsOnOtherWorkspacesAreHidden()
    {
        Fixture fixture;
        const auto id = fixture.add();
        fixture.perform(QStringLiteral("focus-workspace-down"));
        fixture.advance(1);
        QVERIFY(!fixture.state(id).onActiveWorkspace);
        QVERIFY(!fixture.state(id).visible);
        VERIFY_INVARIANTS(fixture);
    }

    void windowsAreVisibleDuringWorkspaceSwitch()
    {
        Fixture fixture(animatedConfig());
        const auto id = fixture.add();
        fixture.advance(2000);
        fixture.perform(QStringLiteral("focus-workspace-down"));
        QVERIFY(fixture.state(id).visible);
        QVERIFY(!fixture.state(id).onActiveWorkspace);
        VERIFY_INVARIANTS(fixture);
    }

    void overviewStateTogglesWithoutAnimating()
    {
        Fixture fixture(animatedConfig());
        fixture.add();
        fixture.advance(2000);
        fixture.engine().setOverviewOpen(true);
        QVERIFY(fixture.engine().isOverviewOpen());
        QVERIFY(!fixture.engine().isAnimating());
        fixture.engine().setOverviewOpen(false);
        QVERIFY(!fixture.engine().isOverviewOpen());
        QVERIFY(!fixture.engine().isAnimating());
        VERIFY_INVARIANTS(fixture);
    }

    void disabledAnimationsCompleteInstantly()
    {
        Fixture fixture;
        const auto id = fixture.add();
        QCOMPARE(fixture.state(id).renderAlpha, 1.0);
        QCOMPARE(fixture.state(id).renderFrame, fixture.state(id).targetFrame);
        fixture.advance(1);
        QVERIFY(!fixture.engine().isAnimating());
        VERIFY_INVARIANTS(fixture);
    }

    void slowdownScalesTheClock()
    {
        Config::Config config = animatedConfig();
        config.animations.slowdown = 2.0;
        Fixture fixture(config);
        fixture.add();
        QCOMPARE(fixture.clock().rate(), 0.5);
    }
};

QTEST_GUILESS_MAIN(TestLayoutAnimations)
#include "test_layout_animations.moc"
