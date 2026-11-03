#include "helpers.h"
#include "render/eventlayer.h"

#include <QTest>

#include <cmath>

using namespace Konveyor;
using Kind = Config::BorderLayerKind;
using Render::Testing::compareClose;
using Render::Testing::layerOf;

namespace
{

const QRectF kWindow(0, 0, 400, 200);

Render::EventScene wideScene()
{
    Render::EventScene scene;
    scene.subject = QRectF(0, 0, 2000, 100);
    return scene;
}

}

class TestEventLayer : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void kindsAreSortedIntoOneShotsAndFronts()
    {
        for (const Kind kind : {Kind::PulseFront, Kind::Wave, Kind::DrawIn, Kind::FadeOut}) {
            QVERIFY(Render::isLightFrontLayer(kind));
            QVERIFY(Render::isOneShotLayer(kind));
        }
        QVERIFY(Render::isOneShotLayer(Kind::Ripple));
        QVERIFY(!Render::isLightFrontLayer(Kind::Ripple));
        QVERIFY(!Render::isOneShotLayer(Kind::Glow));
        QVERIFY(!Render::isLightFrontLayer(Kind::Gradient));
    }

    void lightFrontsGetTheirOwnShaderRange()
    {
        QCOMPARE(Render::bandShaderKind(Kind::PulseFront), 100);
        QCOMPARE(Render::bandShaderKind(Kind::Wave), 101);
        QCOMPARE(Render::bandShaderKind(Kind::DrawIn), 102);
        QCOMPARE(Render::bandShaderKind(Kind::FadeOut), 103);
        QCOMPARE(Render::bandShaderKind(Kind::Glow), static_cast<int>(Kind::Glow));
        QCOMPARE(Render::bandShaderKind(Kind::Gradient), 0);
    }

    void durationsScaleWithSpeed()
    {
        Render::EventScene scene = wideScene();
        scene.farthest = 1000.0;
        Config::BorderLayer layer = layerOf(Kind::DrawIn);
        compareClose(Render::oneShotSeconds(layer, scene), 0.75);
        layer.speed = 2.0;
        compareClose(Render::oneShotSeconds(layer, scene), 0.375);
        layer.speed = -2.0;
        compareClose(Render::oneShotSeconds(layer, scene), 0.375);
        layer.speed = 0.0;
        QCOMPARE(Render::oneShotSeconds(layer, scene), 0.0);
        compareClose(Render::oneShotSeconds(layerOf(Kind::FadeOut), scene), 0.45);
        compareClose(Render::oneShotSeconds(layerOf(Kind::Ripple), scene), 0.8);
        compareClose(Render::oneShotSeconds(layerOf(Kind::Glow), scene), 0.8);
    }

    void pulseFrontDurationFollowsTheDistance()
    {
        Render::EventScene scene = wideScene();
        scene.farthest = 1000.0;
        compareClose(Render::oneShotSeconds(layerOf(Kind::PulseFront), scene), 1560.0 / 2200.0);
        scene.farthest = 0.0;
        compareClose(Render::oneShotSeconds(layerOf(Kind::PulseFront), scene), 0.55);
    }

    void waveDurationFollowsTheTravel()
    {
        compareClose(Render::oneShotSeconds(layerOf(Kind::Wave), wideScene()), 2640.0 / 2400.0);
        Render::EventScene small;
        small.subject = QRectF(0, 0, 100, 100);
        compareClose(Render::oneShotSeconds(layerOf(Kind::Wave), small), 0.4);
    }

    void aSlowEnoughLayerEndsNothing()
    {
        Config::BorderLayer layer = layerOf(Kind::Ripple);
        QVERIFY(!Render::oneShotFrame(layer, wideScene(), kWindow, -0.1));
        QVERIFY(!Render::oneShotFrame(layer, wideScene(), kWindow, 0.8));
        QVERIFY(Render::oneShotFrame(layer, wideScene(), kWindow, 0.79));
        layer.speed = 0.0;
        QVERIFY(!Render::oneShotFrame(layer, wideScene(), kWindow, 0.0));
    }

    void rippleFrameIsJustProgress()
    {
        const auto frame = Render::oneShotFrame(layerOf(Kind::Ripple), wideScene(), kWindow, 0.4);
        QVERIFY(frame);
        compareClose(frame->phase, 0.5);
        QCOMPARE(frame->params, QVector4D());
        QCOMPARE(frame->front, QVector4D());
    }

    void pulseFrontStartsAtThePointOfTheEvent()
    {
        Render::EventScene scene = wideScene();
        scene.farthest = 1000.0;
        scene.point = QPointF(300, 150);
        const double duration = 1560.0 / 2200.0;
        const auto frame = Render::oneShotFrame(layerOf(Kind::PulseFront), scene, kWindow, duration / 2);
        QVERIFY(frame);
        compareClose(frame->phase, 1560.0 / 2);
        QCOMPARE(frame->params, QVector4D(70, 280, 0.75F, 0.5F));
        QCOMPARE(frame->front, QVector4D(100, 50, 0, 0));
    }

    void pulseFrontWithoutAPointSpreadsFromTheSubject()
    {
        Render::EventScene scene;
        scene.subject = QRectF(100, 100, 80, 40);
        const auto frame = Render::oneShotFrame(layerOf(Kind::PulseFront), scene, kWindow, 0.0);
        QVERIFY(frame);
        QCOMPARE(frame->phase, 0.0);
        QCOMPARE(frame->front, QVector4D(-60, 20, 40, 20));
    }

    void waveRunsFromTheSourceToTheTarget()
    {
        Render::EventScene scene;
        scene.subject = QRectF(0, 0, 100, 100);
        scene.from = QRectF(0, 0, 100, 100);
        scene.to = QRectF(1000, 0, 100, 100);
        const double total = 1000.0 + 50.0 + 640.0;
        const double duration = std::max(0.4, total / 2400.0);
        const auto frame = Render::oneShotFrame(layerOf(Kind::Wave), scene, kWindow, duration / 2);
        QVERIFY(frame);
        compareClose(frame->phase, total / 2);
        compareClose(frame->params.z(), 1.0);
        QCOMPARE(frame->params.w(), 0.5F);
        QCOMPARE(frame->front, QVector4D(-150, -50, 1, 0));
    }

    void waveFadesInAndOut()
    {
        const Render::EventScene scene = wideScene();
        const double duration = 2640.0 / 2400.0;
        const auto start = Render::oneShotFrame(layerOf(Kind::Wave), scene, kWindow, duration * 0.01);
        const auto end = Render::oneShotFrame(layerOf(Kind::Wave), scene, kWindow, duration * 0.99);
        QVERIFY(start && end);
        QVERIFY(start->params.z() < 0.5F);
        QVERIFY(end->params.z() < 0.1F);
        QVERIFY(end->params.z() > 0.0F);
    }

    void waveStartsAtAPointThatIsNotTheTarget()
    {
        Render::EventScene scene;
        scene.subject = QRectF(500, 0, 100, 100);
        scene.point = QPointF(100, 50);
        const auto frame = Render::oneShotFrame(layerOf(Kind::Wave), scene, kWindow, 0.0);
        QVERIFY(frame);
        QCOMPARE(frame->front, QVector4D(-100, -50, 1, 0));
    }

    void waveIgnoresAPointOnTheTarget()
    {
        Render::EventScene scene;
        scene.subject = QRectF(500, 0, 100, 100);
        scene.point = QPointF(550, 50);
        const auto frame = Render::oneShotFrame(layerOf(Kind::Wave), scene, kWindow, 0.0);
        QVERIFY(frame);
        QCOMPARE(frame->front, QVector4D(300, -50, 1, 0));
    }

    void waveBetweenIdenticalCentersPointsRight()
    {
        Render::EventScene scene;
        scene.subject = QRectF(0, 0, 100, 100);
        scene.from = QRectF(0, 0, 100, 100);
        scene.to = QRectF(0, 0, 100, 100);
        const auto frame = Render::oneShotFrame(layerOf(Kind::Wave), scene, kWindow, 0.0);
        QVERIFY(frame);
        QCOMPARE(frame->front.z(), 1.0F);
        QCOMPARE(frame->front.w(), 0.0F);
    }

    void waveDirectionFollowsTheDiagonal()
    {
        Render::EventScene scene;
        scene.subject = QRectF(0, 0, 100, 100);
        scene.from = QRectF(0, 0, 100, 100);
        scene.to = QRectF(300, 400, 100, 100);
        const auto frame = Render::oneShotFrame(layerOf(Kind::Wave), scene, kWindow, 0.0);
        QVERIFY(frame);
        QCOMPARE(frame->front.z(), 0.6F);
        QCOMPARE(frame->front.w(), 0.8F);
    }

    void drawInTracesThenFades()
    {
        const Config::BorderLayer layer = layerOf(Kind::DrawIn);
        const auto tracing = Render::oneShotFrame(layer, wideScene(), kWindow, 0.75 * 0.35);
        QVERIFY(tracing);
        compareClose(tracing->phase, 0.5);
        QCOMPARE(tracing->params, QVector4D(6, 0, 1, 0));
        QCOMPARE(tracing->front, QVector4D());
        const auto fading = Render::oneShotFrame(layer, wideScene(), kWindow, 0.75 * 0.85);
        QVERIFY(fading);
        compareClose(fading->params.z(), 0.5);
        compareClose(fading->phase, 0.85 / 0.7);
    }

    void fadeOutIsPlainProgress()
    {
        const auto frame = Render::oneShotFrame(layerOf(Kind::FadeOut), wideScene(), kWindow, 0.225);
        QVERIFY(frame);
        compareClose(frame->phase, 0.5);
    }

    void restingFramesRepeatAfterAGap()
    {
        const Config::BorderLayer layer = layerOf(Kind::Ripple);
        const auto playing = Render::restingFrame(layer, wideScene(), kWindow, 0.4, 3.0);
        QVERIFY(playing);
        compareClose(playing->phase, 0.5);
        QVERIFY(!Render::restingFrame(layer, wideScene(), kWindow, 1.0, 3.0));
        const auto again = Render::restingFrame(layer, wideScene(), kWindow, 2.4 + 0.4, 3.0);
        QVERIFY(again);
        compareClose(again->phase, 0.5);
        Config::BorderLayer stopped = layer;
        stopped.speed = 0.0;
        QVERIFY(!Render::restingFrame(stopped, wideScene(), kWindow, 0.4, 3.0));
    }

    void styleDurationTakesTheLongestOneShot()
    {
        Config::BorderStyle style;
        style.layers = {layerOf(Kind::Glow), layerOf(Kind::DrawIn), layerOf(Kind::Ripple)};
        compareClose(Render::styleSeconds(style, wideScene(), std::nullopt), 0.8);
        QCOMPARE(Render::styleSeconds(style, wideScene(), 2.5), 2.5);
        style.layers = {layerOf(Kind::Glow)};
        QCOMPARE(Render::styleSeconds(style, wideScene(), std::nullopt), 1.0);
        Config::BorderLayer stopped = layerOf(Kind::Ripple);
        stopped.speed = 0.0;
        style.layers = {stopped};
        QCOMPARE(Render::styleSeconds(style, wideScene(), std::nullopt), 1.0);
        QCOMPARE(Render::styleSeconds({}, wideScene(), std::nullopt), 1.0);
    }

    void standingEnvelopeRisesAndFalls()
    {
        compareClose(Render::standingEnvelope(0.1, 1.0), 0.5);
        compareClose(Render::standingEnvelope(0.9, 1.0), 0.5);
        QCOMPARE(Render::standingEnvelope(0.5, 1.0), 1.0);
        QCOMPARE(Render::standingEnvelope(-0.1, 1.0), 0.0);
        QCOMPARE(Render::standingEnvelope(1.1, 1.0), 0.0);
    }

    void distancesBetweenRectangles()
    {
        QCOMPARE(Render::distanceBetween(QRectF(0, 0, 10, 10), QRectF(5, 5, 10, 10)), 0.0);
        QCOMPARE(Render::distanceBetween(QRectF(0, 0, 10, 10), QRectF(20, 0, 10, 10)), 10.0);
        QCOMPARE(Render::distanceBetween(QRectF(20, 0, 10, 10), QRectF(0, 0, 10, 10)), 10.0);
        QCOMPARE(Render::distanceBetween(QRectF(0, 0, 10, 10), QRectF(13, 14, 10, 10)), 5.0);
        QCOMPARE(Render::distanceBetween(QRectF(0, 20, 10, 10), QRectF(0, 0, 10, 10)), 10.0);
    }

    void farthestDistanceReachesTheFarCorner()
    {
        compareClose(Render::farthestDistance(QRectF(10, 10, 10, 10), QRectF(0, 0, 100, 100)), std::hypot(80.0, 80.0));
        QCOMPARE(Render::farthestDistance(QRectF(0, 0, 100, 100), QRectF(10, 10, 10, 10)), 0.0);
        QCOMPARE(Render::farthestDistance(QRectF(0, 0, 100, 100), QRectF(0, 0, 130, 100)), 30.0);
        QCOMPARE(Render::farthestDistance(QRectF(0, 0, 100, 100), QRectF(0, -20, 100, 100)), 20.0);
    }

    void segmentsCrossRectanglesTheyPassThrough()
    {
        const QRectF rect(10, 10, 20, 20);
        QVERIFY(Render::segmentCrosses({0, 0}, {40, 40}, rect));
        QVERIFY(Render::segmentCrosses({0, 20}, {40, 20}, rect));
        QVERIFY(Render::segmentCrosses({20, 0}, {20, 40}, rect));
        QVERIFY(Render::segmentCrosses({15, 15}, {16, 16}, rect));
        QVERIFY(Render::segmentCrosses({20, 20}, {20, 20}, rect));
    }

    void segmentsMissRectanglesTheyAvoid()
    {
        const QRectF rect(10, 10, 20, 20);
        QVERIFY(!Render::segmentCrosses({0, 0}, {5, 5}, rect));
        QVERIFY(!Render::segmentCrosses({0, 0}, {40, 0}, rect));
        QVERIFY(!Render::segmentCrosses({0, 0}, {0, 40}, rect));
        QVERIFY(!Render::segmentCrosses({0, 5}, {40, 5}, rect));
        QVERIFY(!Render::segmentCrosses({0, 35}, {40, 35}, rect));
        QVERIFY(!Render::segmentCrosses({35, 0}, {35, 40}, rect));
        QVERIFY(!Render::segmentCrosses({5, 0}, {5, 40}, rect));
        QVERIFY(!Render::segmentCrosses({0, 25}, {25, 0}, QRectF(20, 20, 5, 5)));
        QVERIFY(!Render::segmentCrosses({0, 0}, {0, 0}, rect));
    }
};

QTEST_GUILESS_MAIN(TestEventLayer)
#include "test_eventlayer.moc"
