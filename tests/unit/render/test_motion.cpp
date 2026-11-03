#include "helpers.h"
#include "render/drive.h"

#include <QTest>

#include <cmath>

using namespace Konveyor;
using Config::DriveCurve;
using Config::Signal;
using Kind = Config::BorderLayerKind;
using Render::Testing::compareClose;
using Render::Testing::driveOf;
using Render::Testing::layerOf;

namespace
{

Config::BorderLayer glowDrivenBy(Signal signal, const QString &setting, double amount, DriveCurve curve = DriveCurve::Linear)
{
    Config::BorderLayer layer = layerOf(Kind::Glow);
    layer.width = 4.0;
    layer.drives = {driveOf(setting, signal, amount, 0.0, curve)};
    return layer;
}

Render::LayerMotion advancedOnce(const Config::BorderLayer &layer, const Render::SignalFrame &frame)
{
    Render::LayerMotion motion;
    Render::advanceMotion(motion, layer, frame, 1.0, 3.0);
    return motion;
}

}

class TestMotion : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void curvesShapeTheDrivenValue_data()
    {
        QTest::addColumn<int>("curve");
        QTest::addColumn<float>("input");
        QTest::addColumn<double>("width");
        QTest::newRow("linear up") << static_cast<int>(DriveCurve::Linear) << 0.25F << 5.0;
        QTest::newRow("linear down") << static_cast<int>(DriveCurve::Linear) << -0.25F << 3.0;
        QTest::newRow("square up") << static_cast<int>(DriveCurve::Square) << 0.25F << 4.25;
        QTest::newRow("square down") << static_cast<int>(DriveCurve::Square) << -0.25F << 3.75;
        QTest::newRow("sqrt up") << static_cast<int>(DriveCurve::Sqrt) << 0.25F << 6.0;
        QTest::newRow("sqrt down") << static_cast<int>(DriveCurve::Sqrt) << -0.25F << 2.0;
        QTest::newRow("invert up") << static_cast<int>(DriveCurve::Invert) << 0.25F << 7.0;
        QTest::newRow("invert down") << static_cast<int>(DriveCurve::Invert) << -0.25F << 9.0;
    }

    void curvesShapeTheDrivenValue()
    {
        QFETCH(int, curve);
        QFETCH(float, input);
        QFETCH(double, width);
        Render::SignalFrame frame;
        frame.set(Signal::ScrollDirection, input);
        const auto motion
            = advancedOnce(glowDrivenBy(Signal::ScrollDirection, QStringLiteral("width"), 4.0, static_cast<DriveCurve>(curve)), frame);
        compareClose(motion.resolved.width, width);
    }

    void aDriveWithoutAmountChangesNothing()
    {
        Render::SignalFrame frame;
        frame.set(Signal::AudioBass, 1.0F);
        const auto motion = advancedOnce(glowDrivenBy(Signal::AudioBass, QStringLiteral("width"), 0.0), frame);
        QCOMPARE(motion.resolved.width, 4.0);
    }

    void drivenSettingsAreClampedToWhatTheyMean()
    {
        Config::BorderLayer layer = layerOf(Kind::Glow);
        layer.opacity = 5.0;
        layer.tail = 2.0;
        layer.gap = -1.0;
        layer.dash = 0.0;
        layer.size = 0.0;
        layer.lifetime = 0.0;
        layer.rate = 0.0;
        layer.thickness = 0.0;
        layer.baseline = 2.0;
        layer.count = 0;
        layer.burst = -3;
        layer.width = -5.0;
        const auto motion = advancedOnce(layer, {});
        QCOMPARE(motion.resolved.opacity, 1.0);
        QCOMPARE(motion.resolved.tail, 1.0);
        QCOMPARE(motion.resolved.gap, 0.0);
        QCOMPARE(motion.resolved.dash, 1.0);
        QCOMPARE(motion.resolved.size, 0.1);
        QCOMPARE(motion.resolved.lifetime, 0.05);
        QCOMPARE(motion.resolved.thickness, 0.5);
        QCOMPARE(motion.resolved.baseline, 1.0);
        QCOMPARE(motion.resolved.count, 1);
        QCOMPARE(motion.resolved.burst, 0);
        QCOMPARE(motion.resolved.width, 3.0);
        QCOMPARE(motion.resolved.rate, 0.0);
    }

    void aNegativeBaseWidthIsClampedAfterDriving()
    {
        Config::BorderLayer layer = glowDrivenBy(Signal::AudioBass, QStringLiteral("width"), -10.0);
        Render::SignalFrame frame;
        frame.set(Signal::AudioBass, 1.0F);
        QCOMPARE(advancedOnce(layer, frame).resolved.width, 0.0);
    }

    void smoothingFollowsTheSignalOverTime()
    {
        Config::BorderLayer layer = glowDrivenBy(Signal::AudioBass, QStringLiteral("width"), 10.0);
        layer.drives[0].smooth = 1.0;
        Render::SignalFrame frame;
        Render::LayerMotion motion;
        Render::advanceMotion(motion, layer, frame, 1.0, 3.0);
        frame.set(Signal::AudioBass, 1.0F);
        Render::advanceMotion(motion, layer, frame, 1.1, 3.0);
        compareClose(motion.resolved.width, 4.0 + 10.0 * (1.0 - std::exp(-0.1)));
        Render::advanceMotion(motion, layer, frame, 5.0, 3.0);
        compareClose(motion.smoothed[0], (1.0 - std::exp(-0.1)) + (1.0 - (1.0 - std::exp(-0.1))) * (1.0 - std::exp(-0.1)));
        Render::advanceMotion(motion, layer, frame, 4.0, 3.0);
        compareClose(motion.smoothed[0], (1.0 - std::exp(-0.1)) + (1.0 - (1.0 - std::exp(-0.1))) * (1.0 - std::exp(-0.1)));
    }

    void firstFrameJumpsStraightToTheSignal()
    {
        Config::BorderLayer layer = glowDrivenBy(Signal::AudioBass, QStringLiteral("width"), 10.0);
        layer.drives[0].smooth = 5.0;
        Render::SignalFrame frame;
        frame.set(Signal::AudioBass, 0.5F);
        compareClose(advancedOnce(layer, frame).resolved.width, 9.0);
    }

    void directionDriveFlipsSpeedWithTheSign()
    {
        Config::BorderLayer layer = layerOf(Kind::Sweep);
        layer.drives = {driveOf(QStringLiteral("direction"), Signal::ScrollDirection)};
        Render::SignalFrame frame;
        Render::LayerMotion motion;
        frame.set(Signal::ScrollDirection, 1.0F);
        Render::advanceMotion(motion, layer, frame, 1.0, 3.0);
        QCOMPARE(motion.sign, 1.0F);
        QCOMPARE(motion.resolved.speed, 1.0);
        frame.set(Signal::ScrollDirection, -1.0F);
        Render::advanceMotion(motion, layer, frame, 1.05, 3.0);
        QCOMPARE(motion.sign, -1.0F);
        QCOMPARE(motion.resolved.speed, -1.0);
        frame.set(Signal::ScrollDirection, 0.0F);
        Render::advanceMotion(motion, layer, frame, 1.1, 3.0);
        QCOMPARE(motion.sign, -1.0F);
        QCOMPARE(motion.resolved.speed, -1.0);
        QCOMPARE(Render::motionUniforms(motion).z(), -1.0F);
    }

    void directionDriveSteersParticlesUpOrDown()
    {
        Config::BorderLayer layer = layerOf(Kind::Particles);
        layer.drives = {driveOf(QStringLiteral("direction"), Signal::ScrollDirection)};
        Render::SignalFrame frame;
        frame.set(Signal::ScrollDirection, 1.0F);
        QCOMPARE(advancedOnce(layer, frame).resolved.direction, Config::ParticleDirection::Down);
        frame.set(Signal::ScrollDirection, -1.0F);
        QCOMPARE(advancedOnce(layer, frame).resolved.direction, Config::ParticleDirection::Up);
        layer.drives = {driveOf(QStringLiteral("width"), Signal::ScrollDirection)};
        QCOMPARE(advancedOnce(layer, frame).resolved.direction, Config::ParticleDirection::Out);
    }

    void rateDriveSpeedsUpTheClock()
    {
        Config::BorderLayer layer = layerOf(Kind::Particles);
        layer.rate = 24.0;
        layer.drives = {driveOf(QStringLiteral("rate"), Signal::AudioBass, 24.0)};
        Render::SignalFrame frame;
        frame.set(Signal::AudioBass, 1.0F);
        Render::LayerMotion motion;
        Render::advanceMotion(motion, layer, frame, 1.0, 3.0);
        Render::advanceMotion(motion, layer, frame, 1.05, 3.0);
        compareClose(motion.clock, 0.1);
        QCOMPARE(motion.resolved.rate, 24.0);
    }

    void speedDriveAdvancesThePhase()
    {
        Config::BorderLayer layer = layerOf(Kind::Gradient);
        layer.speed = 1.0;
        layer.drives = {driveOf(QStringLiteral("speed"), Signal::AudioBass, 1.0)};
        Render::SignalFrame frame;
        frame.set(Signal::AudioBass, 1.0F);
        Render::LayerMotion motion;
        Render::advanceMotion(motion, layer, frame, 0.0, 3.0);
        QCOMPARE(motion.phase, 0.0);
        for (int step = 1; step <= 10; ++step) {
            Render::advanceMotion(motion, layer, frame, step * 0.1, 3.0);
        }
        compareClose(motion.phase, 2.0 / 8.0);
        Render::advanceMotion(motion, layer, frame, 1.1, 3.0);
        QVERIFY(motion.phase >= 0.0 && motion.phase < 1.0);
    }

    void stepsNeverExceedTenthOfASecond()
    {
        Config::BorderLayer layer = layerOf(Kind::Gradient);
        Render::SignalFrame frame;
        Render::LayerMotion motion;
        Render::advanceMotion(motion, layer, frame, 1.0, 3.0);
        Render::advanceMotion(motion, layer, frame, 50.0, 3.0);
        compareClose(motion.phase, 0.1 / 8.0);
        Render::advanceMotion(motion, layer, frame, 10.0, 3.0);
        compareClose(motion.phase, 0.1 / 8.0);
        QCOMPARE(motion.lastSeconds, 10.0);
    }

    void firingNotesOnlyListenedSignals()
    {
        Config::BorderLayer layer = layerOf(Kind::Particles);
        layer.fireOn = {Config::Signal::Click, Config::Signal::Keypress};
        Render::SignalFrame frame;
        Render::LayerMotion motion;
        Render::advanceMotion(motion, layer, frame, 1.0, 3.0);
        frame.fire(Signal::ScrollStep);
        Render::advanceMotion(motion, layer, frame, 1.1, 3.0);
        QCOMPARE(motion.bursts.number, quint64(0));
        frame.fire(Signal::Keypress);
        Render::advanceMotion(motion, layer, frame, 1.2, 3.0);
        QCOMPARE(motion.bursts.number, quint64(1));
        QCOMPARE(motion.lastFire, 1.2);
        Render::advanceMotion(motion, layer, frame, 1.3, 3.0);
        QCOMPARE(motion.bursts.number, quint64(1));
        frame.fire(Signal::Click);
        frame.fire(Signal::Click);
        Render::advanceMotion(motion, layer, frame, 1.4, 3.0);
        QCOMPARE(motion.bursts.number, quint64(2));
    }

    void firstFrameNeverFiresOldImpulses()
    {
        Config::BorderLayer layer = layerOf(Kind::Particles);
        layer.fireOn = {Config::Signal::Click};
        Render::SignalFrame frame;
        frame.fire(Signal::Click);
        frame.fire(Signal::Click);
        QCOMPARE(advancedOnce(layer, frame).bursts.number, quint64(0));
    }

    void blocksFollowTheSpectrumWithSensitivity()
    {
        Config::BorderLayer layer = layerOf(Kind::Equalizer);
        layer.sensitivity = 2.0;
        Render::SignalFrame frame;
        frame.bands[0] = 0.3F;
        frame.bands[1] = 0.8F;
        frame.wave[0] = 0.3F;
        frame.wave[1] = -0.9F;
        const auto motion = advancedOnce(layer, frame);
        QCOMPARE(motion.bands[0], 0.6F);
        QCOMPARE(motion.bands[1], 1.0F);
        QCOMPARE(motion.wave[0], 0.6F);
        QCOMPARE(motion.wave[1], -1.0F);
    }

    void blocksRiseFasterThanTheyFall()
    {
        Config::BorderLayer layer = layerOf(Kind::Spectrum);
        layer.smoothing = 0.3;
        Render::SignalFrame frame;
        Render::LayerMotion motion;
        Render::advanceMotion(motion, layer, frame, 1.0, 3.0);
        frame.bands[0] = 1.0F;
        frame.bands[1] = 0.0F;
        motion.bands[1] = 0.8F;
        Render::advanceMotion(motion, layer, frame, 1.1, 3.0);
        compareClose(motion.bands[0], 1.0 - std::exp(-0.1 / 0.015));
        compareClose(motion.bands[1], 0.8 * std::exp(-0.1 / 0.15));
        QVERIFY(motion.bands[0] > 0.99F);
    }

    void waveformMovesWithoutSmoothingOnTheFirstFrame()
    {
        Config::BorderLayer layer = layerOf(Kind::Waveform);
        layer.smoothing = 5.0;
        Render::SignalFrame frame;
        frame.wave[5] = 0.5F;
        QCOMPARE(advancedOnce(layer, frame).wave[5], 0.5F);
        Render::LayerMotion motion;
        Render::advanceMotion(motion, layer, {}, 1.0, 3.0);
        Render::advanceMotion(motion, layer, frame, 1.1, 3.0);
        compareClose(motion.wave[5], 0.5 * (1.0 - std::exp(-0.1 / (0.05 * 5.0))));
    }

    void nonBlockKindsLeaveTheBlocksAlone()
    {
        Render::SignalFrame frame;
        frame.bands[0] = 1.0F;
        QCOMPARE(advancedOnce(layerOf(Kind::Glow), frame).bands[0], 0.0F);
    }
};

QTEST_GUILESS_MAIN(TestMotion)
#include "test_motion.moc"
