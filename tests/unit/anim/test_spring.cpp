#include "anim/spring.h"

#include <QTest>

#include <cmath>

using namespace std::chrono_literals;
using Konveyor::Anim::Duration;
using Konveyor::Anim::Spring;
using Konveyor::Anim::SpringParams;

namespace
{

struct Case
{
    double zeta;
    double stiffness;
    double from;
    double to;
    double velocity;
};

double expectedPosition(const Case &c, double t)
{
    const double naturalFrequency = std::sqrt(c.stiffness);
    const double decay = c.zeta * naturalFrequency;
    const double offset = c.from - c.to;
    const double slope = c.velocity + decay * offset;
    const double envelope = std::exp(-decay * t);
    if (c.zeta == 1.0) {
        return c.to + envelope * (offset + slope * t);
    }
    const double discriminant = naturalFrequency * std::sqrt(std::abs(1.0 - c.zeta * c.zeta));
    if (c.zeta < 1.0) {
        return c.to + envelope * (offset * std::cos(discriminant * t) + slope / discriminant * std::sin(discriminant * t));
    }
    return c.to + envelope * (offset * std::cosh(discriminant * t) + slope / discriminant * std::sinh(discriminant * t));
}

Spring makeSpring(const Case &c, double epsilon)
{
    return Spring {c.from, c.to, c.velocity, SpringParams(c.zeta, c.stiffness, epsilon)};
}

Duration seconds(double value)
{
    return Duration(std::llround(value * 1e9));
}

void addCases()
{
    QTest::addColumn<Case>("springCase");
    QTest::newRow("critical") << Case {1.0, 800, 0, 100, 0};
    QTest::newRow("critical-velocity") << Case {1.0, 1000, 0, 1, -3};
    QTest::newRow("underdamped") << Case {0.6, 1000, 0, 1, 0};
    QTest::newRow("underdamped-descending") << Case {0.3, 500, 50, -20, 40};
    QTest::newRow("overdamped") << Case {2.0, 800, 10, -5, 50};
}

}

class TestSpring : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void paramsFromDampingRatio()
    {
        const SpringParams params(0.5, 400, 0.001);
        QCOMPARE(params.mass, 1.0);
        QCOMPARE(params.stiffness, 400.0);
        QCOMPARE(params.epsilon, 0.001);
        QCOMPARE(params.damping, 20.0);
    }

    void paramsClampNegative()
    {
        const SpringParams params(-1, -5, -1);
        QCOMPARE(params.damping, 0.0);
        QCOMPARE(params.stiffness, 0.0);
        QCOMPARE(params.epsilon, 0.0);
    }

    void valueMatchesClosedForm_data() { addCases(); }

    void valueMatchesClosedForm()
    {
        QFETCH(Case, springCase);
        const Spring spring = makeSpring(springCase, 0.0001);
        for (const double t : {0.0, 0.004, 0.02, 0.05, 0.1, 0.25, 0.5}) {
            const double expected = expectedPosition(springCase, t);
            QCOMPARE_LE(std::abs(spring.valueAt(seconds(t)) - expected), 1e-9 * std::max(1.0, std::abs(expected)));
        }
    }

    void startsAtFromWithInitialVelocity_data() { addCases(); }

    void startsAtFromWithInitialVelocity()
    {
        QFETCH(Case, springCase);
        const Spring spring = makeSpring(springCase, 0.0001);
        QCOMPARE_LE(std::abs(spring.valueAt(Duration::zero()) - springCase.from), 1e-12);
        QCOMPARE_LE(std::abs(spring.velocityAt(Duration::zero()) - springCase.velocity), 1e-9);
    }

    void velocityMatchesDerivative_data() { addCases(); }

    void velocityMatchesDerivative()
    {
        QFETCH(Case, springCase);
        const Spring spring = makeSpring(springCase, 0.0001);
        const Duration step = 1us;
        for (const Duration t : {Duration(3ms), Duration(40ms), Duration(120ms)}) {
            const double derivative = (spring.valueAt(t + step) - spring.valueAt(t - step)) / 2e-6;
            QCOMPARE_LE(std::abs(spring.velocityAt(t) - derivative), 1e-4 * std::max(1.0, std::abs(derivative)));
        }
    }

    void satisfiesEquationOfMotion_data() { addCases(); }

    void satisfiesEquationOfMotion()
    {
        QFETCH(Case, springCase);
        const Spring spring = makeSpring(springCase, 0.0001);
        const Duration step = 1us;
        for (const Duration t : {Duration(5ms), Duration(60ms), Duration(200ms)}) {
            const double acceleration = (spring.velocityAt(t + step) - spring.velocityAt(t - step)) / 2e-6;
            const double force = -spring.params.stiffness * (spring.valueAt(t) - spring.to) - spring.params.damping * spring.velocityAt(t);
            QCOMPARE_LE(std::abs(acceleration - force), 1e-3 * std::max(1.0, std::abs(force)));
        }
    }

    void envelopeDuration_data()
    {
        QTest::addColumn<Case>("springCase");
        QTest::addColumn<double>("epsilon");
        QTest::newRow("default") << Case {1.0, 800, 0, 1, 0} << 0.0001;
        QTest::newRow("workspace-switch") << Case {1.0, 1000, 0, 1, 0} << 0.0001;
        QTest::newRow("underdamped") << Case {0.6, 1000, 0, 1, 0} << 0.001;
    }

    void envelopeDuration()
    {
        QFETCH(Case, springCase);
        QFETCH(double, epsilon);
        const Spring spring = makeSpring(springCase, epsilon);
        const double beta = springCase.zeta * std::sqrt(springCase.stiffness);
        const Duration expected = seconds(-std::log(epsilon) / beta);
        QCOMPARE_LE(std::abs((spring.duration() - expected).count()), 1);
    }

    void overdampedDurationReachesEpsilon()
    {
        const Spring spring = makeSpring(Case {2.0, 800, 0, 1, 0}, 0.0001);
        const Duration duration = spring.duration();
        QVERIFY(duration > Duration::zero());
        QCOMPARE_LE(std::abs(spring.valueAt(duration) - spring.to), 0.0001 + 1e-9);
        const double envelopeSeconds = -std::log(0.0001) / (2.0 * std::sqrt(800.0));
        QVERIFY(duration > seconds(envelopeSeconds));
    }

    void clampedDurationIsFirstMillisecondWithinEpsilon_data() { addCases(); }

    void clampedDurationIsFirstMillisecondWithinEpsilon()
    {
        QFETCH(Case, springCase);
        const Spring spring = makeSpring(springCase, 0.0001);
        const auto clamped = spring.timeToTarget();
        QVERIFY(clamped.has_value());
        const double direction = springCase.to > springCase.from ? 1.0 : -1.0;
        QCOMPARE_LE(direction * (springCase.to - spring.valueAt(*clamped)), 0.0001);
        if (*clamped > 1ms) {
            QVERIFY(direction * (springCase.to - spring.valueAt(*clamped - 1ms)) > 0.0001);
        }
    }

    void underdampedOvershootsBeforeSettling()
    {
        const Spring spring = makeSpring(Case {0.6, 1000, 0, 1, 0}, 0.001);
        QVERIFY(*spring.timeToTarget() < spring.duration());
        QVERIFY(spring.valueAt(*spring.timeToTarget() + 20ms) > 1.0);
    }

    void clampedDurationGivesUpAfterThreeSeconds()
    {
        const Spring spring = makeSpring(Case {1.0, 1, 0, 1, 0}, 0.0001);
        QVERIFY(!spring.timeToTarget().has_value());
    }

    void zeroDampingNeverSettles()
    {
        const Spring spring = makeSpring(Case {0.0, 800, 0, 1, 0}, 0.0001);
        QCOMPARE(spring.duration(), Duration::max());
        QCOMPARE(spring.timeToTarget(), std::optional<Duration>(Duration::max()));
    }

    void equalEndpointsSettleImmediately()
    {
        const Spring spring = makeSpring(Case {1.15, 850, 0, 0, 0}, 0.0001);
        QCOMPARE(spring.duration(), Duration::zero());
        QCOMPARE(spring.timeToTarget(), std::optional<Duration>(Duration::zero()));
        QVERIFY(std::isfinite(spring.valueAt(Duration::zero())));
    }

    void stiffOverdampedSpringStaysFinite()
    {
        const Spring spring = makeSpring(Case {6.0, 1200, 0, 1, 0}, 0.0001);
        const Duration duration = spring.duration();
        QVERIFY(duration >= Duration::zero());
        QVERIFY(duration < Duration::max());
        QVERIFY(std::isfinite(spring.valueAt(Duration::zero())));
        QVERIFY(std::isfinite(spring.valueAt(duration)));
    }
};

QTEST_GUILESS_MAIN(TestSpring)
#include "test_spring.moc"
