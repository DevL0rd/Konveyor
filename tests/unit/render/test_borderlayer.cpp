#include "helpers.h"
#include "render/borderlayer.h"

#include <QTest>

#include <cmath>
#include <numbers>

using namespace Konveyor;
using Kind = Config::BorderLayerKind;
using Render::Testing::compareClose;
using Render::Testing::driveOf;
using Render::Testing::layerOf;

namespace
{

constexpr double kInvSqrt2 = 0.70710678118654752;

void compareSame(QPointF actual, QPointF expected)
{
    compareClose(actual.x(), expected.x());
    compareClose(actual.y(), expected.y());
}

}

class TestBorderLayer : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void perimeterStartsOnTheTopEdge()
    {
        const Render::PerimeterPoint start = Render::perimeterPoint({100, 50}, {}, 0.0);
        compareSame(start.position, {0, 0});
        compareSame(start.normal, {0, -1});
        compareSame(start.tangent, {1, 0});
        compareSame(Render::perimeterPoint({100, 50}, {}, 0.25).position, {75, 0});
    }

    void perimeterWalksAllFourSides()
    {
        compareSame(Render::perimeterPoint({100, 50}, {}, 0.5).position, {100, 50});
        const Render::PerimeterPoint bottom = Render::perimeterPoint({100, 50}, {}, 0.6);
        compareSame(bottom.position, {70, 50});
        compareSame(bottom.normal, {0, 1});
        compareSame(bottom.tangent, {-1, 0});
        const Render::PerimeterPoint left = Render::perimeterPoint({100, 50}, {}, 0.9);
        compareSame(left.position, {0, 30});
        compareSame(left.normal, {-1, 0});
        compareSame(left.tangent, {0, -1});
        const Render::PerimeterPoint right = Render::perimeterPoint({100, 50}, {}, 0.4);
        compareSame(right.position, {100, 20});
        compareSame(right.normal, {1, 0});
    }

    void perimeterFractionWrapsAround()
    {
        compareSame(Render::perimeterPoint({100, 50}, {}, 1.25).position, Render::perimeterPoint({100, 50}, {}, 0.25).position);
        compareSame(Render::perimeterPoint({100, 50}, {}, -0.25).position, {25, 50});
        compareSame(Render::perimeterPoint({100, 50}, {}, 1.0).position, {0, 0});
    }

    void perimeterFollowsRoundedCorners()
    {
        const Config::CornerRadius radius {10, 10, 10, 10};
        const double total = 4 * 80 + 4 * std::numbers::pi * 5;
        const Render::PerimeterPoint corner = Render::perimeterPoint({100, 100}, radius, (80 + std::numbers::pi * 10 / 4) / total);
        compareSame(corner.position, {90 + 10 * kInvSqrt2, 10 - 10 * kInvSqrt2});
        compareSame(corner.normal, {kInvSqrt2, -kInvSqrt2});
        compareSame(corner.tangent, {kInvSqrt2, kInvSqrt2});
        compareSame(Render::perimeterPoint({100, 100}, radius, 0.0).position, {10, 0});
    }

    void perimeterOfNothingIsTheOrigin()
    {
        const Render::PerimeterPoint point = Render::perimeterPoint({0, 0}, {}, 0.7);
        compareSame(point.position, {0, 0});
        compareSame(point.normal, {0, -1});
    }

    void widthFallsBackToTheDecoration()
    {
        Config::BorderLayer layer = layerOf(Kind::Glow);
        QCOMPARE(Render::layerWidth(layer, 3.0), 3.0);
        layer.width = 5.0;
        QCOMPARE(Render::layerWidth(layer, 3.0), 5.0);
    }

    void lightFrontsAreAtLeastTwelvePixelsWide_data()
    {
        QTest::addColumn<int>("kind");
        QTest::newRow("pulse-front") << static_cast<int>(Kind::PulseFront);
        QTest::newRow("wave") << static_cast<int>(Kind::Wave);
        QTest::newRow("draw-in") << static_cast<int>(Kind::DrawIn);
        QTest::newRow("fade-out") << static_cast<int>(Kind::FadeOut);
    }

    void lightFrontsAreAtLeastTwelvePixelsWide()
    {
        QFETCH(int, kind);
        const Config::BorderLayer layer = layerOf(static_cast<Kind>(kind));
        QCOMPARE(Render::layerWidth(layer, 3.0), 12.0);
        QCOMPARE(Render::layerWidth(layer, 20.0), 20.0);
    }

    void outerReachDependsOnTheKind_data()
    {
        QTest::addColumn<int>("kind");
        QTest::addColumn<double>("reach");
        QTest::newRow("gradient") << static_cast<int>(Kind::Gradient) << 7.0;
        QTest::newRow("pulse") << static_cast<int>(Kind::Pulse) << 13.0;
        QTest::newRow("ripple") << static_cast<int>(Kind::Ripple) << 19.0;
        QTest::newRow("bass-thump") << static_cast<int>(Kind::BassThump) << 13.0;
        QTest::newRow("wavy") << static_cast<int>(Kind::Wavy) << 6.0 + 3.0 + 1.0;
        QTest::newRow("elastic") << static_cast<int>(Kind::Elastic) << 6.0 * 1.3 + 3.0 + 1.0;
    }

    void outerReachDependsOnTheKind()
    {
        QFETCH(int, kind);
        QFETCH(double, reach);
        Config::BorderLayer layer = layerOf(static_cast<Kind>(kind));
        layer.width = 6.0;
        layer.thickness = 3.0;
        layer.drives.clear();
        compareClose(Render::layerOuterReach(layer, 2.0), reach);
    }

    void particleReachCoversTheirTravel()
    {
        Config::BorderLayer layer = layerOf(Kind::Particles);
        layer.speed = -2.0;
        layer.lifetime = 1.0;
        layer.size = 4.0;
        layer.width = 6.0;
        compareClose(Render::layerOuterReach(layer, 2.0), 6.0 + 2.0 * 40.0 * 1.4 + 4.0);
    }

    void drivenReachUsesThePeakOfEveryDrive()
    {
        Config::BorderLayer layer = layerOf(Kind::Glow);
        layer.width = 4.0;
        layer.drives = {driveOf(QStringLiteral("width"), Config::Signal::AudioBass, 10.0),
            driveOf(QStringLiteral("width"), Config::Signal::AudioMid, -6.0)};
        compareClose(Render::layerOuterReach(layer, 2.0), 15.0);
        layer.width = 0.0;
        compareClose(Render::layerOuterReach(layer, 2.0), 13.0);
    }

    void phaseRunsOneCyclePerKindPeriod_data()
    {
        QTest::addColumn<int>("kind");
        QTest::addColumn<double>("period");
        QTest::newRow("gradient") << static_cast<int>(Kind::Gradient) << 8.0;
        QTest::newRow("sweep") << static_cast<int>(Kind::Sweep) << 4.0;
        QTest::newRow("comet") << static_cast<int>(Kind::Comet) << 4.0;
        QTest::newRow("glow") << static_cast<int>(Kind::Glow) << 3.0;
        QTest::newRow("pulse") << static_cast<int>(Kind::Pulse) << 1.5;
        QTest::newRow("dashes") << static_cast<int>(Kind::Dashes) << 0.5;
        QTest::newRow("ripple") << static_cast<int>(Kind::Ripple) << 8.0;
        QTest::newRow("shimmer") << static_cast<int>(Kind::Shimmer) << 8.0;
        QTest::newRow("speed-lines") << static_cast<int>(Kind::SpeedLines) << 4.0;
        QTest::newRow("particles") << static_cast<int>(Kind::Particles) << 1.0;
    }

    void phaseRunsOneCyclePerKindPeriod()
    {
        QFETCH(int, kind);
        QFETCH(double, period);
        Config::BorderLayer layer;
        layer.kind = static_cast<Kind>(kind);
        compareClose(Render::layerCyclesPerSecond(layer), 1.0 / period);
        compareClose(Render::layerPhase(layer, period * 0.25), 0.25);
        compareClose(Render::layerPhase(layer, period * 3.5), 0.5);
        layer.speed = 2.0;
        compareClose(Render::layerPhase(layer, period * 0.25), 0.5);
        layer.speed = -1.0;
        compareClose(Render::layerPhase(layer, period * 0.25), 0.75);
    }

    void reactiveKindsHaveTheirOwnShader_data()
    {
        QTest::addColumn<int>("kind");
        QTest::addColumn<int>("shader");
        QTest::newRow("equalizer") << static_cast<int>(Kind::Equalizer) << 0;
        QTest::newRow("waveform") << static_cast<int>(Kind::Waveform) << 1;
        QTest::newRow("spectrum") << static_cast<int>(Kind::Spectrum) << 2;
        QTest::newRow("shimmer") << static_cast<int>(Kind::Shimmer) << 3;
        QTest::newRow("bass-thump") << static_cast<int>(Kind::BassThump) << 4;
        QTest::newRow("speed-lines") << static_cast<int>(Kind::SpeedLines) << 5;
        QTest::newRow("scroll-trail") << static_cast<int>(Kind::ScrollTrail) << 6;
        QTest::newRow("bulge") << static_cast<int>(Kind::Bulge) << 7;
        QTest::newRow("wavy") << static_cast<int>(Kind::Wavy) << 8;
        QTest::newRow("elastic") << static_cast<int>(Kind::Elastic) << 9;
        QTest::newRow("chevrons") << static_cast<int>(Kind::Chevrons) << 10;
        QTest::newRow("edge-flash") << static_cast<int>(Kind::EdgeFlash) << 11;
        QTest::newRow("gauge") << static_cast<int>(Kind::Gauge) << 12;
        QTest::newRow("gradient") << static_cast<int>(Kind::Gradient) << -1;
        QTest::newRow("particles") << static_cast<int>(Kind::Particles) << -1;
        QTest::newRow("pulse-front") << static_cast<int>(Kind::PulseFront) << -1;
    }

    void reactiveKindsHaveTheirOwnShader()
    {
        QFETCH(int, kind);
        QFETCH(int, shader);
        QCOMPARE(Render::reactiveShaderKind(static_cast<Kind>(kind)), shader);
    }

    void onlyParticlesWithABurstOrFireBurst()
    {
        Config::BorderLayer layer = layerOf(Kind::Particles);
        QVERIFY(!Render::layerBursts(layer));
        layer.burst = 3;
        QVERIFY(Render::layerBursts(layer));
        layer.burst = 0;
        layer.fireOn = {Config::Signal::Click};
        QVERIFY(Render::layerBursts(layer));
        Config::BorderLayer glow = layerOf(Kind::Glow);
        glow.burst = 3;
        glow.fireOn = {Config::Signal::Click};
        QVERIFY(!Render::layerBursts(glow));
    }

    void staticLayersStopRedrawing()
    {
        Config::BorderLayer layer = layerOf(Kind::Glow);
        QVERIFY(Render::layerRunsContinuously(layer));
        layer.speed = 0.0;
        QVERIFY(!Render::layerRunsContinuously(layer));
        layer.drives = {driveOf(QStringLiteral("width"), Config::Signal::AudioBass)};
        QVERIFY(Render::layerRunsContinuously(layer));
        layer.drives.clear();
        layer.fireOn = {Config::Signal::Click};
        QVERIFY(Render::layerRunsContinuously(layer));
        Config::BorderLayer still = layerOf(Kind::Equalizer);
        still.speed = 0.0;
        QVERIFY(Render::layerRunsContinuously(still));
        Config::BorderLayer particles;
        particles.kind = Kind::Particles;
        particles.speed = 0.0;
        QVERIFY(Render::layerRunsContinuously(particles));
    }

    void paramsCarryWhatEachShaderReads()
    {
        Config::BorderLayer layer;
        layer.angle = 90.0;
        layer.count = 3;
        layer.tail = 0.4;
        layer.dash = 10.0;
        layer.gap = 5.0;
        layer.kind = Kind::Gradient;
        QCOMPARE(Render::layerParams(layer), QVector4D(static_cast<float>(std::numbers::pi / 2), 0, 0, 0));
        layer.kind = Kind::Comet;
        QCOMPARE(Render::layerParams(layer), QVector4D(3, 0.4F, 1, 0));
        layer.speed = -1.0;
        QCOMPARE(Render::layerParams(layer), QVector4D(3, 0.4F, -1, 0));
        layer.kind = Kind::Equalizer;
        QCOMPARE(Render::layerParams(layer), QVector4D(3, 5, 0, 0));
        layer.kind = Kind::Dashes;
        QCOMPARE(Render::layerParams(layer), QVector4D(10, 5, 0, 0));
        layer.kind = Kind::Glow;
        QCOMPARE(Render::layerParams(layer), QVector4D());
        for (const Kind kind :
            {Kind::SpeedLines, Kind::ScrollTrail, Kind::Bulge, Kind::Wavy, Kind::Chevrons, Kind::EdgeFlash, Kind::Gauge}) {
            layer.kind = kind;
            QCOMPARE(Render::layerParams(layer), QVector4D(3, 0.4F, 0, 0));
        }
    }
};

QTEST_GUILESS_MAIN(TestBorderLayer)
#include "test_borderlayer.moc"
