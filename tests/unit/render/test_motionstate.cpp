#include "helpers.h"
#include "render/drive.h"

#include <QTest>

#include <cmath>

using namespace Konveyor;
using Config::Signal;
using Kind = Config::BorderLayerKind;
using Render::Testing::compareClose;
using Render::Testing::driveOf;
using Render::Testing::layerOf;

namespace
{

}

class TestMotionState : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void elasticStartsAtItsBaseline()
    {
        Config::BorderLayer layer = layerOf(Kind::Elastic);
        layer.drives.clear();
        layer.baseline = 0.5;
        Render::LayerMotion motion;
        Render::advanceMotion(motion, layer, {}, 1.0, 3.0);
        QCOMPARE(motion.stretch, 0.5F);
        QCOMPARE(Render::motionUniforms(motion).x(), 0.5F);
    }

    void elasticSpringsTowardsANewBaseline()
    {
        Config::BorderLayer layer = layerOf(Kind::Elastic);
        layer.drives.clear();
        layer.baseline = 0.5;
        Render::LayerMotion motion;
        Render::advanceMotion(motion, layer, {}, 1.0, 3.0);
        layer.baseline = 1.0;
        Render::advanceMotion(motion, layer, {}, 1.05, 3.0);
        compareClose(motion.stretchSpeed, 6.5);
        compareClose(motion.stretch, 0.825);
        for (int step = 2; step < 200; ++step) {
            Render::advanceMotion(motion, layer, {}, 1.0 + step * 0.05, 3.0);
        }
        compareClose(motion.stretch, 1.0, 1e-3);
    }

    void elasticFollowsTheDirectionSign()
    {
        Config::BorderLayer layer = layerOf(Kind::Elastic);
        layer.drives = {driveOf(QStringLiteral("direction"), Signal::ScrollDirection)};
        layer.baseline = 0.5;
        Render::SignalFrame frame;
        frame.set(Signal::ScrollDirection, -1.0F);
        Render::LayerMotion motion;
        Render::advanceMotion(motion, layer, frame, 1.0, 3.0);
        QCOMPARE(motion.stretch, -0.5F);
    }

    void uniformsCarryLevelPositionDirectionAndThickness()
    {
        Config::BorderLayer layer = layerOf(Kind::Bulge);
        layer.drives.clear();
        layer.baseline = 0.4;
        layer.position = 1.25;
        layer.thickness = 5.0;
        Render::LayerMotion motion;
        Render::advanceMotion(motion, layer, {}, 1.0, 3.0);
        QCOMPARE(Render::motionUniforms(motion), QVector4D(0.4F, 0.25F, 1.0F, 5.0F));
        layer.position = -0.25;
        Render::advanceMotion(motion, layer, {}, 1.1, 3.0);
        QCOMPARE(Render::motionUniforms(motion).y(), 0.75F);
    }

    void stillPoseShowsTheLayerMidAction()
    {
        Config::BorderLayer layer = layerOf(Kind::Particles);
        layer.fireOn = {Signal::Click};
        Render::LayerMotion motion;
        motion.phase = 0.9;
        Render::poseMotion(motion, layer, 5.0, 3.0);
        QCOMPARE(motion.clock, 5.0);
        compareClose(motion.lastFire, 4.7);
        QCOMPARE(motion.bursts.number, quint64(1));
        compareClose(motion.bursts.times[1], 4.7);
        QVERIFY(motion.started);
        QCOMPARE(motion.lastSeconds, 5.0);
    }

    void stillPoseRunsTheSimulatedSignals()
    {
        Config::BorderLayer layer = layerOf(Kind::Glow);
        layer.width = 2.0;
        layer.drives = {driveOf(QStringLiteral("width"), Signal::AudioBass, 10.0)};
        Render::LayerMotion motion;
        Render::poseMotion(motion, layer, 1.0, 3.0);
        QVERIFY(motion.resolved.width > 2.0);
        QCOMPARE(motion.resolved.opacity, 1.0);
    }

    void reactiveMeansDrivenFiredBurstingOrAudioVisual()
    {
        QVERIFY(!Render::isReactive(layerOf(Kind::Gradient)));
        QVERIFY(Render::isReactive(layerOf(Kind::Equalizer)));
        Config::BorderLayer layer = layerOf(Kind::Glow);
        layer.burst = 2;
        QVERIFY(Render::isReactive(layer));
        layer.burst = 0;
        layer.fireOn = {Signal::Click};
        QVERIFY(Render::isReactive(layer));
        layer.fireOn.clear();
        layer.drives = {driveOf(QStringLiteral("width"), Signal::AudioBass)};
        QVERIFY(Render::isReactive(layer));
    }

    void signalsOfAStyleAreTheUnionOfItsLayers()
    {
        Config::BorderStyle style;
        Config::BorderLayer first = layerOf(Kind::Glow);
        first.drives = {driveOf(QStringLiteral("width"), Signal::ScrollSpeed)};
        Config::BorderLayer second = layerOf(Kind::Particles);
        second.fireOn = {Signal::Click};
        style.layers = {first, second, layerOf(Kind::Gradient)};
        const Render::SignalSet found = Render::signalsOf(style);
        QVERIFY(found.test(static_cast<std::size_t>(Signal::ScrollSpeed)));
        QVERIFY(found.test(static_cast<std::size_t>(Signal::Click)));
        QCOMPARE(found.count(), std::size_t(2));
        QVERIFY(Render::signalsOf(Config::BorderStyle {}).none());
    }

    void colorMixSignalsAreWantedButDoNotGate()
    {
        Config::BorderLayer layer = layerOf(Kind::Gradient);
        layer.drives
            = {driveOf(QStringLiteral("color-mix"), Signal::AudioTreble), driveOf(QStringLiteral("color-mix"), Signal::PointerSpeed, 0.0)};
        const Render::SignalSet found = Render::signalsOf(layer);
        QVERIFY(found.test(static_cast<std::size_t>(Signal::AudioTreble)));
        QVERIFY(found.test(static_cast<std::size_t>(Signal::AudioLevel)));
        QVERIFY(!found.test(static_cast<std::size_t>(Signal::PointerSpeed)));
        QCOMPARE(Render::layerPresence(layer, {}), 1.0F);
        QVERIFY(!Render::hiddenWhenStill(layer));
    }

    void drivesWithoutAmountAreNotGating()
    {
        Config::BorderLayer layer = layerOf(Kind::Glow);
        layer.drives = {driveOf(QStringLiteral("width"), Signal::ScrollSpeed, 0.0)};
        QVERIFY(Render::signalsOf(layer).none());
        QVERIFY(!Render::hiddenWhenStill(layer));
        QCOMPARE(Render::layerPresence(layer, {}), 1.0F);
    }

    void presenceFollowsTheMostPresentGatingSource()
    {
        Config::BorderLayer layer = layerOf(Kind::Glow);
        layer.drives = {driveOf(QStringLiteral("width"), Signal::ScrollSpeed), driveOf(QStringLiteral("width"), Signal::PointerSpeed)};
        Render::SignalFrame frame;
        QCOMPARE(Render::layerPresence(layer, frame), 0.0F);
        frame.set(Signal::PointerSpeed, 0.5F);
        QCOMPARE(Render::layerPresence(layer, frame), 1.0F);
        frame.presences[static_cast<std::size_t>(Config::SignalSource::Pointer)].level = 0.25F;
        frame.presences[static_cast<std::size_t>(Config::SignalSource::Scroll)].level = 0.5F;
        QCOMPARE(Render::layerPresence(layer, frame), 0.5F);
    }

    void sourcelessGatesNeverHideALayer()
    {
        Config::BorderLayer layer = layerOf(Kind::Particles);
        layer.fireOn = {Signal::Click};
        QCOMPARE(Render::layerPresence(layer, {}), 1.0F);
        QVERIFY(!Render::hiddenWhenStill(layer));
    }

    void audioLayersAreNotHiddenWhenStill()
    {
        Config::BorderLayer layer = layerOf(Kind::Glow);
        layer.drives = {driveOf(QStringLiteral("width"), Signal::AudioBass)};
        QVERIFY(!Render::hiddenWhenStill(layer));
        QVERIFY(!Render::hiddenWhenStill(layerOf(Kind::Equalizer)));
        layer.drives = {driveOf(QStringLiteral("width"), Signal::ViewSpeed)};
        QVERIFY(Render::hiddenWhenStill(layer));
        layer.drives = {driveOf(QStringLiteral("width"), Signal::PointerSpeed)};
        QVERIFY(Render::hiddenWhenStill(layer));
    }

    void restingNeedsNothingToMove()
    {
        Config::BorderLayer layer = layerOf(Kind::Gradient);
        layer.speed = 0.0;
        QVERIFY(!Render::layerRests(layer, {}));
        layer.drives = {driveOf(QStringLiteral("color-mix"), Signal::PointerSpeed)};
        QVERIFY(Render::layerRests(layer, {}));
        layer.speed = 1.0;
        QVERIFY(!Render::layerRests(layer, {}));
        layer.speed = 0.0;
        layer.kind = Kind::Particles;
        QVERIFY(!Render::layerRests(layer, {}));
        layer.kind = Kind::Equalizer;
        QVERIFY(!Render::layerRests(layer, {}));
        layer.kind = Kind::Gradient;
        layer.drives.append(driveOf(QStringLiteral("width"), Signal::AudioBass));
        QVERIFY(!Render::layerRests(layer, {}));
    }

    void restingIgnoresDrivesThatDoNotMixColor()
    {
        Config::BorderLayer layer = layerOf(Kind::Gradient);
        layer.speed = 0.0;
        layer.drives
            = {driveOf(QStringLiteral("width"), Signal::PointerSpeed, 0.0), driveOf(QStringLiteral("color-mix"), Signal::PointerSpeed)};
        QVERIFY(Render::layerRests(layer, {}));
        layer.drives = {driveOf(QStringLiteral("width"), Signal::PointerSpeed, 0.0)};
        QVERIFY(!Render::layerRests(layer, {}));
    }

    void restingStopsWhenAColorSourceIsAlive()
    {
        Config::BorderLayer layer = layerOf(Kind::Gradient);
        layer.speed = 0.0;
        layer.drives = {driveOf(QStringLiteral("color-mix"), Signal::Time)};
        QVERIFY(!Render::layerRests(layer, {}));
        layer.drives = {driveOf(QStringLiteral("color-mix"), Signal::ScrollSpeed), driveOf(QStringLiteral("color-mix"), Signal::ViewSpeed)};
        Render::SignalFrame frame;
        QVERIFY(Render::layerRests(layer, frame));
        frame.hear(Config::SignalSource::View);
        QVERIFY(!Render::layerRests(layer, frame));
    }

    void colorMixAddsUpAndIsClamped()
    {
        Config::BorderLayer layer = layerOf(Kind::Gradient);
        layer.drives
            = {driveOf(QStringLiteral("color-mix"), Signal::AudioBass, 0.6), driveOf(QStringLiteral("color-mix"), Signal::AudioMid, 0.6)};
        Render::SignalFrame frame;
        frame.set(Signal::AudioBass, 1.0F);
        frame.set(Signal::AudioMid, 1.0F);
        Render::LayerMotion motion;
        Render::advanceMotion(motion, layer, frame, 1.0, 3.0);
        QVERIFY(motion.colorMix);
        QCOMPARE(Render::colorMixUniform(&motion), 1.0F);
        frame.set(Signal::AudioBass, 0.0F);
        frame.set(Signal::AudioMid, -1.0F);
        Render::advanceMotion(motion, layer, frame, 1.1, 3.0);
        QCOMPARE(Render::colorMixUniform(&motion), 0.0F);
        Render::advanceMotion(motion, layerOf(Kind::Gradient), frame, 1.2, 3.0);
        QVERIFY(!motion.colorMix);
        QCOMPARE(Render::colorMixUniform(&motion), -1.0F);
    }
};

QTEST_GUILESS_MAIN(TestMotionState)
#include "test_motionstate.moc"
