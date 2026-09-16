#include "anim/easing.h"

#include <QTest>

#include <cmath>

using Konveyor::Anim::Curve;
using Konveyor::Config::EasingCurve;

namespace
{

double bernstein(double p1, double p2, double t)
{
    const double u = 1.0 - t;
    return 3.0 * u * u * t * p1 + 3.0 * u * t * t * p2 + t * t * t;
}

struct Controls
{
    double x1;
    double y1;
    double x2;
    double y2;

    Curve curve() const
    {
        Konveyor::Config::EasingParams params;
        params.curve = EasingCurve::CubicBezier;
        params.x1 = x1;
        params.y1 = y1;
        params.x2 = x2;
        params.y2 = y2;
        return Curve::fromConfig(params);
    }
};

Controls fetchControls()
{
    QFETCH(double, x1);
    QFETCH(double, y1);
    QFETCH(double, x2);
    QFETCH(double, y2);
    return Controls {x1, y1, x2, y2};
}

void addBezierRows()
{
    QTest::addColumn<double>("x1");
    QTest::addColumn<double>("y1");
    QTest::addColumn<double>("x2");
    QTest::addColumn<double>("y2");
    QTest::newRow("ease") << 0.25 << 0.1 << 0.25 << 1.0;
    QTest::newRow("ease-in-out") << 0.42 << 0.0 << 0.58 << 1.0;
    QTest::newRow("emphasized") << 0.05 << 0.7 << 0.1 << 1.0;
}

}

class TestEasing : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void namedCurves_data()
    {
        QTest::addColumn<EasingCurve>("curve");
        QTest::addColumn<double>("x");
        QTest::addColumn<double>("expected");
        QTest::newRow("linear-mid") << EasingCurve::Linear << 0.3 << 0.3;
        QTest::newRow("quad-start") << EasingCurve::EaseOutQuad << 0.0 << 0.0;
        QTest::newRow("quad-mid") << EasingCurve::EaseOutQuad << 0.5 << 0.75;
        QTest::newRow("quad-end") << EasingCurve::EaseOutQuad << 1.0 << 1.0;
        QTest::newRow("cubic-start") << EasingCurve::EaseOutCubic << 0.0 << 0.0;
        QTest::newRow("cubic-mid") << EasingCurve::EaseOutCubic << 0.5 << 0.875;
        QTest::newRow("cubic-end") << EasingCurve::EaseOutCubic << 1.0 << 1.0;
        QTest::newRow("expo-start") << EasingCurve::EaseOutExpo << 0.0 << 0.0;
        QTest::newRow("expo-tenth") << EasingCurve::EaseOutExpo << 0.1 << 0.5;
        QTest::newRow("expo-end") << EasingCurve::EaseOutExpo << 1.0 << 1.0 - 1.0 / 1024.0;
    }

    void namedCurves()
    {
        QFETCH(EasingCurve, curve);
        QFETCH(double, x);
        QFETCH(double, expected);
        QCOMPARE_LE(std::abs(Curve(curve).y(x) - expected), 1e-15);
    }

    void bezierEndpoints_data() { addBezierRows(); }

    void bezierEndpoints()
    {
        const Controls controls = fetchControls();
        const Curve bezier = controls.curve();
        QCOMPARE(bezier.y(0.0), 0.0);
        QCOMPARE(bezier.y(-0.5), 0.0);
        QCOMPARE(bezier.y(1.0), 1.0);
        QCOMPARE(bezier.y(1.5), 1.0);
    }

    void bezierMatchesParametricForm_data() { addBezierRows(); }

    void bezierMatchesParametricForm()
    {
        const Controls controls = fetchControls();
        const Curve bezier = controls.curve();
        for (int i = 1; i < 20; ++i) {
            const double t = i / 20.0;
            QCOMPARE_LE(std::abs(bezier.y(bernstein(controls.x1, controls.x2, t)) - bernstein(controls.y1, controls.y2, t)), 2e-4);
        }
    }

    void bezierSamplesAreMonotonic_data() { addBezierRows(); }

    void bezierSamplesAreMonotonic()
    {
        const Controls controls = fetchControls();
        const Curve bezier = controls.curve();
        double previous = 0.0;
        for (int i = 1; i <= 100; ++i) {
            const double current = bezier.y(i / 100.0);
            QVERIFY(current >= previous);
            previous = current;
        }
    }

    void identityBezierIsLinear()
    {
        const Curve bezier = Controls {0.0, 0.0, 1.0, 1.0}.curve();
        for (int i = 1; i < 10; ++i) {
            QCOMPARE_LE(std::abs(bezier.y(i / 10.0) - i / 10.0), 2e-4);
        }
    }

    void curveFromConfig()
    {
        Konveyor::Config::EasingParams params;
        params.curve = EasingCurve::CubicBezier;
        params.x1 = 0.25;
        params.y1 = 0.1;
        params.x2 = 0.25;
        params.y2 = 1.0;
        const Curve curve = Curve::fromConfig(params);
        QCOMPARE(curve.kind(), EasingCurve::CubicBezier);
        QCOMPARE(curve.y(0.4), (Controls {0.25, 0.1, 0.25, 1.0}).curve().y(0.4));

        params.curve = EasingCurve::EaseOutQuad;
        QCOMPARE(Curve::fromConfig(params).y(0.4), 0.4 * 1.6);
    }
};

QTEST_GUILESS_MAIN(TestEasing)
#include "test_easing.moc"
