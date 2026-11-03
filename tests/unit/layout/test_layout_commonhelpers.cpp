#include "striphelpers.h"

#include "layout/common/geometry.h"
#include "layout/common/sizechange.h"
#include "layout/common/slideanimation.h"
#include "layout/common/velocitytracker.h"
#include "layout/monitor/monitorprofiles.h"
#include "layout/monitor/workspacetransition.h"

using namespace LayoutTest;
using Konveyor::Anim::Duration;

namespace
{

Config::MonitorProfile profileFor(const Config::MonitorMatch &match)
{
    Config::MonitorProfile profile;
    profile.name = QStringLiteral("profile");
    profile.matches.append(match);
    return profile;
}

bool matches(const Config::MonitorMatch &match, QSizeF size = QSizeF(1920, 1080), const QString &name = QStringLiteral("DP-1"))
{
    Config::Config config;
    config.monitorProfiles.append(profileFor(match));
    return Layout::monitorProfileFor(config, name, size) != nullptr;
}

}

class TestLayoutCommonHelpers : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void geometryUpdatesCompareEveryEdge()
    {
        using Layout::GeometryUpdate;
        const QRectF frame(10, 10, 100, 100);
        const QRectF same = frame;
        QCOMPARE(Layout::geometryUpdateFor(same, std::nullopt, frame), GeometryUpdate::None);
        QCOMPARE(Layout::geometryUpdateFor(QRectF(10, 30, 100, 100), std::nullopt, frame), GeometryUpdate::MoveResize);
        QCOMPARE(Layout::geometryUpdateFor(QRectF(30, 10, 100, 100), std::nullopt, frame), GeometryUpdate::MoveResize);
        QCOMPARE(Layout::geometryUpdateFor(QRectF(10, 10, 100, 100), QSizeF(100, 100), frame), GeometryUpdate::None);
        QCOMPARE(Layout::geometryUpdateFor(QRectF(30, 10, 100, 100), QSizeF(100, 100), frame), GeometryUpdate::Move);
        QCOMPARE(Layout::geometryUpdateFor(QRectF(10, 10, 100, 100), QSizeF(100, 300), frame), GeometryUpdate::None);
        QCOMPARE(Layout::geometryUpdateFor(QRectF(10, 10, 100, 100), QSizeF(300, 100), frame), GeometryUpdate::None);
        QCOMPARE(Layout::geometryUpdateFor(QRectF(10, 10, 200, 100), std::nullopt, frame), GeometryUpdate::MoveResize);
        QCOMPARE(Layout::geometryUpdateFor(QRectF(10, 10, 100, 200), std::nullopt, frame), GeometryUpdate::MoveResize);
        QCOMPARE(Layout::geometryUpdateFor(same, std::nullopt, frame, false), GeometryUpdate::None);
        QCOMPARE(Layout::geometryUpdateFor(QRectF(30, 10, 100, 100), std::nullopt, frame, false), GeometryUpdate::Move);
    }

    void parkedFramesFallBackToTheFirstCandidate()
    {
        const QRectF home(500, 0, 100, 100);
        const QList<QRectF> outputs {home, QRectF(0, -1000, 1000, 1000), QRectF(0, 100, 1000, 1000)};
        const QRectF frame(520, 20, 50, 50);
        QCOMPARE(Layout::parkedFrame(frame, home, outputs), QRectF(520, 1100 + 256 + 20, 50, 50));
    }

    void parkedFramesPickTheFirstCandidateNearestToHome()
    {
        const QRectF home(0, 0, 1000, 1000);
        const QList<QRectF> outputs {home};
        const QRectF frame(100, 100, 50, 50);
        QCOMPARE(Layout::parkedFrame(frame, home, outputs).topLeft(), QPointF(100, 1000 + 256 + 100));
        QCOMPARE(Layout::parkedFrame(frame, home, {}).topLeft(), QPointF(100, 1000 + 256 + 100));
    }

    void borderExtentOnlyCountsEnabledBorders()
    {
        Config::Border border;
        border.enabled = false;
        border.width = 4;
        QCOMPARE(Layout::borderExtent(border), 0.0);
        border.enabled = true;
        QCOMPARE(Layout::borderExtent(border), 8.0);
    }

    void workspaceReferencesNeedAValue()
    {
        QVERIFY(!Layout::parseWorkspaceReference(QString()).has_value());
        QVERIFY(!Layout::parseWorkspaceReference(QStringLiteral("300")).has_value());
        const auto index = Layout::parseWorkspaceReference(QStringLiteral("3"));
        QVERIFY(index.has_value());
        QCOMPARE(index->kind, Config::WorkspaceReferenceKind::Index);
        const auto name = Layout::parseWorkspaceReference(QStringLiteral("web"));
        QVERIFY(name.has_value());
        QCOMPARE(name->name, QStringLiteral("web"));
    }

    void velocityTrackerWithoutEventsIsStill()
    {
        Layout::VelocityTracker tracker;
        QCOMPARE(tracker.velocity(), 0.0);
        tracker.push(10.0, std::chrono::milliseconds(100));
        QCOMPARE(tracker.velocity(), 0.0);
        tracker.push(10.0, std::chrono::milliseconds(120));
        QVERIFY(tracker.velocity() > 0.0);
        tracker.push(5.0, std::chrono::milliseconds(110));
        QCOMPARE(tracker.pos(), 20.0);
        tracker.push(10.0, std::chrono::milliseconds(500));
        QCOMPARE(tracker.velocity(), 0.0);
        QCOMPARE(tracker.pos(), 30.0);
    }

    void finishedSlidesIgnoreShifts()
    {
        Clock clock = Clock::frozenAt(Duration::zero());
        const Config::AnimationParams params = linearAnimationConfig(100).animations.windowMovement;
        std::optional<Layout::SlideAnimation> slide = Layout::startSlide(std::nullopt, clock, 40.0, params, false);
        QVERIFY(slide);
        const double before = slide->from;
        Layout::shiftSlide(slide, 10.0);
        QVERIFY(slide->from > before);
        clock.setRawNow(std::chrono::seconds(5));
        const double settled = slide->from;
        Layout::shiftSlide(slide, 10.0);
        QCOMPARE(slide->from, settled);
        Layout::clearIfDone(slide);
        QVERIFY(!slide);
        std::optional<Layout::SlideAnimation> none;
        Layout::shiftSlide(none, 1.0);
        QCOMPARE(Layout::optionalOffset(none), 0.0);
    }

    void transitionsWithoutMotionStandStill()
    {
        Layout::WorkspaceSwipe swipe;
        swipe.centerIndex = 3;
        swipe.startIndex = 3.0;
        swipe.currentIndex = 3.0;
        Layout::WorkspaceTransition gesture = Layout::WorkspaceTransition::withSwipe(swipe);
        gesture.offset(2);
        QCOMPARE(gesture.swipe()->centerIndex, std::size_t(5));
        gesture.offset(-3);
        QCOMPARE(gesture.swipe()->centerIndex, std::size_t(2));
        QCOMPARE(gesture.currentIndex(), 2.0);
        QCOMPARE(gesture.targetIndex(), 2.0);
        QVERIFY(!gesture.isAnimating());

        Clock clock = Clock::frozenAt(Duration::zero());
        const Config::AnimationParams params = linearAnimationConfig(100).animations.workspaceSwitch;
        Layout::WorkspaceTransition animated
            = Layout::WorkspaceTransition::withAnimation(Konveyor::Anim::Animation(clock, 0.0, 2.0, 0.0, params));
        animated.offset(1);
        QCOMPARE(animated.targetIndex(), 3.0);
        QVERIFY(animated.isAnimating());
    }

    void monitorProfilesMatchByNameAndShape()
    {
        Config::MonitorMatch named;
        named.name = QRegularExpression(QStringLiteral("^HDMI"));
        QVERIFY(!matches(named));
        QVERIFY(matches(named, QSizeF(1920, 1080), QStringLiteral("HDMI-1")));

        Config::MonitorMatch aspect;
        aspect.aspectRatioAbove = 2.0;
        QVERIFY(!matches(aspect));
        aspect.aspectRatioAbove = 1.0;
        QVERIFY(matches(aspect));
        aspect.aspectRatioBelow = 1.5;
        QVERIFY(!matches(aspect));
        QVERIFY(!matches(aspect, QSizeF(100, 0)));

        Config::MonitorMatch width;
        width.widthAbove = 2000;
        QVERIFY(!matches(width));
        width.widthAbove = 1000;
        width.widthBelow = 1500;
        QVERIFY(!matches(width));
        width.widthBelow = 3000;
        QVERIFY(matches(width));

        Config::MonitorMatch height;
        height.heightAbove = 2000;
        QVERIFY(!matches(height));
        height.heightAbove = 500;
        height.heightBelow = 800;
        QVERIFY(!matches(height));
        height.heightBelow = 2000;
        QVERIFY(matches(height));

        Config::Config none;
        QVERIFY(!Layout::monitorProfileFor(none, QStringLiteral("DP-1"), QSizeF(1920, 1080)));
        Config::MonitorProfile everything;
        none.monitorProfiles.append(everything);
        QVERIFY(Layout::monitorProfileFor(none, QStringLiteral("DP-1"), QSizeF(1920, 1080)));
    }
};

QTEST_GUILESS_MAIN(TestLayoutCommonHelpers)
#include "test_layout_commonhelpers.moc"
