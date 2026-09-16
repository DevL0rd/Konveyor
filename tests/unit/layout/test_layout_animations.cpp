#include "helpers.h"

using namespace LayoutTest;

namespace
{

Config::Config animatedConfig()
{
    Config::Config config;
    config.animations.enabled = true;
    return config;
}

}

class TestLayoutAnimations : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void openAnimationFadesIn()
    {
        Fixture fixture(animatedConfig());
        const auto id = fixture.add();
        QVERIFY(fixture.state(id).renderAlpha < 1.0);
        QVERIFY(fixture.engine().isAnimating());
        fixture.advance(2000);
        QCOMPARE(fixture.state(id).renderAlpha, 1.0);
        QVERIFY(!fixture.engine().isAnimating());
        VERIFY_INVARIANTS(fixture);
    }

    void viewOffsetAnimatesToNewColumn()
    {
        Config::Config config = animatedConfig();
        config.layout.defaultColumnWidth = Config::PresetSize(Config::Fixed {1500});
        Fixture fixture(config);
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

    void windowMovementAnimates()
    {
        Fixture fixture(animatedConfig());
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

    void resizeAnimatesOnlyForLargeChanges()
    {
        Fixture fixture(animatedConfig());
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
