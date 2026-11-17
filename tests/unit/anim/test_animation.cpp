#include "anim/animation.h"
#include "animationinspector.h"
#include "manualclock.h"

#include <QTest>

#include <cmath>

using namespace std::chrono_literals;
using Konveyor::Anim::Animation;
using Konveyor::Anim::AnimationInspector;
using Konveyor::Anim::Clock;
using Konveyor::Anim::Curve;
using Konveyor::Anim::Duration;
using Konveyor::Anim::ManualClock;
using Konveyor::Anim::Spring;
using Konveyor::Anim::SpringParams;
using Konveyor::Config::AnimationParams;
using Konveyor::Config::EasingCurve;

namespace
{

AnimationParams springConfig(double dampingRatio, double stiffness, double epsilon)
{
    Konveyor::Config::SpringParams params;
    params.dampingRatio = dampingRatio;
    params.stiffness = stiffness;
    params.epsilon = epsilon;
    return AnimationParams {true, params};
}

AnimationParams easingConfig(double durationMs, EasingCurve curve)
{
    Konveyor::Config::EasingParams params;
    params.durationMs = durationMs;
    params.curve = curve;
    return AnimationParams {true, params};
}

AnimationParams disabled(AnimationParams params)
{
    params.enabled = false;
    return params;
}

AnimationParams defaultSpring()
{
    return springConfig(1.0, 800, 0.0001);
}

Spring referenceSpring(double from, double to, double velocity)
{
    return Spring {from, to, velocity, SpringParams(1.0, 800, 0.0001)};
}

Duration secondsToDuration(double seconds)
{
    return Duration(std::llround(seconds * 1e9));
}

}

class TestAnimation : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void easingFollowsCurve()
    {
        ManualClock clock = ManualClock(Duration::zero());
        const Animation animation(clock, 0, 100, 0, easingConfig(150, EasingCurve::EaseOutExpo));
        QCOMPARE(AnimationInspector::duration(animation), Duration(150ms));
        QCOMPARE(animation.value(), 0.0);
        clock.setRawNow(75ms);
        QCOMPARE_LE(std::abs(animation.value() - (1.0 - std::pow(2.0, -5.0)) * 100.0), 1e-9);
        clock.setRawNow(149ms);
        QVERIFY(!animation.isFinished());
        clock.setRawNow(150ms);
        QVERIFY(animation.isFinished());
        QCOMPARE(animation.value(), 100.0);
        QCOMPARE(AnimationInspector::endTime(animation), Duration(150ms));
    }

    void springMatchesReference()
    {
        ManualClock clock = ManualClock(10ms);
        const Animation animation(clock, 0, 1, 0, defaultSpring());
        const Spring spring = referenceSpring(0, 1, 0);
        QCOMPARE(AnimationInspector::startTime(animation), Duration(10ms));
        QCOMPARE(AnimationInspector::duration(animation), spring.duration());
        QCOMPARE(animation.valueAt(10ms), 0.0);
        QCOMPARE(animation.valueAt(110ms), spring.valueAt(100ms));
        QCOMPARE(animation.valueAt(AnimationInspector::endTime(animation)), 1.0);
        clock.setRawNow(AnimationInspector::endTime(animation) - 1ns);
        QVERIFY(!animation.isFinished());
        clock.setRawNow(AnimationInspector::endTime(animation));
        QVERIFY(animation.isFinished());
    }

    void clampedValueStopsAtTarget()
    {
        ManualClock clock = ManualClock(Duration::zero());
        const Animation animation(clock, 0, 1, 0, springConfig(0.6, 1000, 0.001));
        const Spring spring {0, 1, 0, SpringParams(0.6, 1000, 0.001)};
        clock.setRawNow(*spring.timeToTarget() + 20ms);
        QVERIFY(animation.hasReachedTarget());
        QVERIFY(!animation.isFinished());
        QVERIFY(animation.value() > 1.0);
        QCOMPARE(animation.valueWithoutOvershoot(), 1.0);
    }

    void slowdownScalesDuration_data()
    {
        QTest::addColumn<double>("slowdown");
        QTest::addColumn<AnimationParams>("config");
        for (const double slowdown : {1.0, 2.0, 5.0}) {
            const QByteArray suffix = QByteArray::number(slowdown);
            QTest::newRow(("easing-" + suffix).constData()) << slowdown << easingConfig(150, EasingCurve::EaseOutQuad);
            QTest::newRow(("spring-" + suffix).constData()) << slowdown << defaultSpring();
        }
    }

    void slowdownScalesDuration()
    {
        QFETCH(double, slowdown);
        QFETCH(AnimationParams, config);
        Konveyor::Config::Animations animations;
        animations.slowdown = slowdown;
        ManualClock clock = ManualClock(Duration::zero());
        clock.applyConfig(animations);
        const Animation animation(clock, 0, 1, 0, config);
        const Duration scaled
            = secondsToDuration(std::chrono::duration<double>(AnimationInspector::duration(animation)).count() * slowdown);
        clock.setRawNow(scaled - 1ms);
        QVERIFY(!animation.isFinished());
        QVERIFY(animation.value() < 1.0);
        clock.setRawNow(scaled + 1ms);
        QVERIFY(animation.isFinished());
        QCOMPARE(animation.value(), 1.0);
    }

    void disabledParamsCompleteInstantly()
    {
        ManualClock clock = ManualClock(5ms);
        const Animation animation(clock, 0, 1, 0, disabled(defaultSpring()));
        QCOMPARE(AnimationInspector::duration(animation), Duration::zero());
        QVERIFY(animation.isFinished());
        QVERIFY(animation.hasReachedTarget());
        QCOMPARE(animation.value(), 0.0);
        clock.setRawNow(5ms + 1ns);
        QCOMPARE(animation.value(), 1.0);

        const Animation retargeted = animation.retargeted(10, 20, 0);
        QCOMPARE(AnimationInspector::from(retargeted), 0.0);
        QCOMPARE(retargeted.to(), 1.0);
        QCOMPARE(AnimationInspector::startTime(retargeted), Duration(5ms));
    }

    void clockCompleteInstantly()
    {
        ManualClock clock = ManualClock(Duration::zero());
        const Animation animation(clock, 0, 1, 0, defaultSpring());
        clock.setSkipAnimations(true);
        QVERIFY(animation.isFinished());
        QVERIFY(animation.hasReachedTarget());
        QCOMPARE(animation.value(), 0.0);
        QCOMPARE(animation.valueWithoutOvershoot(), 1.0);
        clock.setRawNow(1ms);
        QCOMPARE(animation.value(), 1.0);
    }

    void velocityContinuityOnRetarget_data()
    {
        QTest::addColumn<double>("rate");
        QTest::newRow("normal") << 1.0;
        QTest::newRow("slowed") << 0.5;
    }

    void velocityContinuityOnRetarget()
    {
        QFETCH(double, rate);
        ManualClock clock = ManualClock(Duration::zero());
        clock.setRate(rate);
        const Animation first(clock, 0, 100, 0, defaultSpring());
        clock.setRawNow(Duration(std::llround(50e6 / rate)));
        const Duration elapsed = clock.now();
        const double position = first.value();
        const Duration step = 1us;
        const double gestureVelocity
            = (referenceSpring(0, 100, 0).valueAt(elapsed + step) - referenceSpring(0, 100, 0).valueAt(elapsed - step)) / 2e-6 * rate;

        const Animation second(clock, position, 300, gestureVelocity, defaultSpring());
        QCOMPARE(AnimationInspector::startTime(second), elapsed);
        QCOMPARE(second.value(), position);

        const Spring expected = referenceSpring(position, 300, gestureVelocity / rate);
        QCOMPARE(AnimationInspector::duration(second), expected.duration());
        QCOMPARE(second.valueAt(elapsed + 30ms), expected.valueAt(30ms));

        const double slope = (second.valueAt(elapsed + 1us) - first.valueAt(elapsed - 1us)) / 2e-6;
        QCOMPARE_LE(std::abs(slope - gestureVelocity / rate), 1e-3 * std::abs(slope));
    }

    void restartedEasingKeepsDurationAndCurve()
    {
        ManualClock clock = ManualClock(Duration::zero());
        const Animation animation(clock, 0, 1, 0, easingConfig(150, EasingCurve::EaseOutQuad));
        clock.setRawNow(40ms);
        const Animation retargeted = animation.retargeted(5, 10, 0);
        QCOMPARE(AnimationInspector::startTime(retargeted), Duration(40ms));
        QCOMPARE(AnimationInspector::duration(retargeted), Duration(150ms));
        QCOMPARE(retargeted.valueAt(115ms), 5.0 + 5.0 * 0.75);
    }

    void restartedSpringKeepsParamsAndStoredVelocity()
    {
        ManualClock clock = ManualClock(Duration::zero());
        const Animation animation(clock, 0, 1, 2.0, defaultSpring());
        clock.setRawNow(40ms);
        const Animation retargeted = animation.retargeted(10, 20, 999.0);
        const Spring expected = referenceSpring(10, 20, 2.0);
        QCOMPARE(AnimationInspector::duration(retargeted), expected.duration());
        QCOMPARE(retargeted.valueAt(60ms), expected.valueAt(20ms));
    }

    void replaceConfigKeepsStartTime()
    {
        ManualClock clock = ManualClock(Duration::zero());
        Animation animation(clock, 0, 1, 0, easingConfig(150, EasingCurve::Linear));
        clock.setRawNow(50ms);
        animation.applyParams(defaultSpring());
        QCOMPARE(AnimationInspector::startTime(animation), Duration::zero());
        QCOMPARE(AnimationInspector::duration(animation), referenceSpring(0, 1, 0).duration());
        QCOMPARE(animation.value(), referenceSpring(0, 1, 0).valueAt(50ms));

        animation.applyParams(disabled(defaultSpring()));
        QCOMPARE(AnimationInspector::duration(animation), Duration::zero());
        QVERIFY(animation.isFinished());
    }

    void offsetShiftsSpring()
    {
        ManualClock clock = ManualClock(Duration::zero());
        Animation animation(clock, 0, 100, 0, defaultSpring());
        clock.setRawNow(30ms);
        const double before = animation.value();
        animation.offset(50);
        QCOMPARE(AnimationInspector::from(animation), 50.0);
        QCOMPARE(animation.to(), 150.0);
        QCOMPARE_LE(std::abs(animation.value() - (before + 50.0)), 1e-9);
    }

    void offsetShiftsEasingWithoutSpringState()
    {
        ManualClock clock = ManualClock(Duration::zero());
        Animation animation = Animation::ease(clock, 2, 4, 0, 100ms, Curve(EasingCurve::Linear));
        animation.offset(10);
        QCOMPARE(AnimationInspector::from(animation), 12.0);
        QCOMPARE(animation.to(), 14.0);
        QCOMPARE(animation.valueAt(50ms), 13.0);
    }

    void descendingSpringStaysBetweenItsEndpoints()
    {
        ManualClock clock = ManualClock(Duration::zero());
        Animation animation(clock, 100, 0, 0, springConfig(0.2, 800, 0.0001));
        clock.setRawNow(30ms);
        const double value = animation.value();
        QVERIFY(value < 100.0);
        QVERIFY(value > -1000.0);
        clock.setRawNow(Duration::zero());
        QCOMPARE(animation.value(), 100.0);
    }

    void valueWithoutOvershootFollowsTheSpringUntilTheTarget()
    {
        ManualClock clock = ManualClock(Duration::zero());
        Animation animation(clock, 0, 1, 0, defaultSpring());
        clock.setRawNow(5ms);
        QCOMPARE(animation.valueWithoutOvershoot(), animation.value());
        clock.setRawNow(2s);
        QCOMPARE(animation.valueWithoutOvershoot(), 1.0);
    }

    void easeFactoryIgnoresVelocity()
    {
        ManualClock clock = ManualClock(Duration::zero());
        const Animation animation = Animation::ease(clock, 2, 4, 1000, 100ms, Curve(EasingCurve::Linear));
        QCOMPARE(animation.valueAt(25ms), 2.5);
    }

    void decelerationFollowsExponentialDecay()
    {
        ManualClock clock = ManualClock(Duration::zero());
        const double rate = 0.997;
        const double velocity = 1000.0;
        const Animation animation = Animation::decelerate(clock, 10, velocity, rate, 0.5);
        const double coeff = 1000.0 * std::log(rate);
        QCOMPARE_LE(std::abs(animation.to() - (10.0 - velocity / coeff)), 1e-9);
        QCOMPARE_LE(
            std::abs((AnimationInspector::duration(animation) - secondsToDuration(std::log(-coeff * 0.5 / velocity) / coeff)).count()), 1);
        QCOMPARE_LE(std::abs(animation.valueAt(100ms) - (10.0 + (std::pow(rate, 100.0) - 1.0) / coeff * velocity)), 1e-9);

        const Animation retargeted = animation.retargeted(-4, 0, 0);
        QCOMPARE(AnimationInspector::from(retargeted), -4.0);
        const double threshold = 0.001;
        QCOMPARE_LE(
            std::abs(
                (AnimationInspector::duration(retargeted) - secondsToDuration(std::log(-coeff * threshold / velocity) / coeff)).count()),
            1);
    }

    void stationaryDecelerationIsDone()
    {
        const Animation animation = Animation::decelerate(ManualClock(Duration::zero()), 3, 0, 0.99, 0.5);
        QCOMPARE(AnimationInspector::duration(animation), Duration::zero());
        QCOMPARE(animation.to(), 3.0);
        QVERIFY(animation.isFinished());
    }
};

QTEST_GUILESS_MAIN(TestAnimation)
#include "test_animation.moc"
