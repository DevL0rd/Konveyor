#include "stylescene.h"

#include <QQuickItem>
#include <QQuickWindow>

using namespace Konveyor::Settings::Testing;

class TestSettingsBorderStyleQml : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase() { QVERIFY(m_home.setUp()); }

    void aStyleWithoutOneShotLayersLastsTheDefaultSecond()
    {
        const Scene scene = open(R"(layers: [{ kind: "gradient" }, { kind: "glow" }])");
        QCOMPARE(duration(scene.item), 1.0);
    }

    void aRippleLastsLongerWhenItIsSlower()
    {
        const Scene scene = open(R"(layers: [{ kind: "ripple", speed: 1.0 }])");
        const double normal = duration(scene.item);
        scene.item->setProperty("layers", QVariantList {QVariantMap {{"kind", "ripple"}, {"speed", 0.5}}});
        QCOMPARE(duration(scene.item), normal * 2.0);
        scene.item->setProperty("layers", QVariantList {QVariantMap {{"kind", "ripple"}, {"speed", 0.0}}});
        QCOMPARE(duration(scene.item), 1.0);
    }

    void theLongestOneShotLayerSetsTheDuration()
    {
        const Scene scene = open(R"(layers: [{ kind: "ripple" }, { kind: "draw-in" }, { kind: "fade-out" }])");
        const double combined = duration(scene.item);
        scene.item->setProperty("layers", QVariantList {QVariantMap {{"kind", "fade-out"}}});
        QVERIFY(combined > duration(scene.item));
    }

    void anExplicitEventLengthOverridesTheDuration()
    {
        const Scene scene = open(R"(layers: [{ kind: "ripple" }]; eventLength: 3.5)");
        QCOMPARE(duration(scene.item), 3.5);
    }

    void aWaveFollowsTheDistanceBetweenTheEventFrames()
    {
        const Scene scene = open(R"(layers: [{ kind: "wave" }]; eventSubject: Qt.rect(0, 0, 100, 100))");
        const double alone = duration(scene.item);
        scene.item->setProperty("eventFrom", QRectF(0, 0, 100, 100));
        scene.item->setProperty("eventTo", QRectF(3000, 0, 100, 100));
        QVERIFY(duration(scene.item) > alone);
    }

    void aPulseFrontFollowsTheFarthestDistance()
    {
        const Scene scene = open(R"(layers: [{ kind: "pulse-front" }])");
        const double near = duration(scene.item);
        scene.item->setProperty("eventFarthest", 6000.0);
        QVERIFY(duration(scene.item) > near);
    }

    void theSoftwareBackendDrawsNothingAndKeepsRunning()
    {
        const Scene scene = open(R"(layers: [{ kind: "gradient" }, { kind: "particles" }])");
        QTest::qWait(100);
        const QImage image = scene.window->grabWindow();
        QVERIFY(!image.isNull());
        for (int y = 0; y < image.height(); y += 7) {
            for (int x = 0; x < image.width(); x += 7) {
                QCOMPARE(image.pixel(x, y), image.pixel(0, 0));
            }
        }
    }

    void movingLayersKeepRequestingFrames()
    {
        const Scene scene = open(R"(layers: [{ kind: "sweep", speed: 2.0 }])");
        QSignalSpy frames(scene.window.get(), &QQuickWindow::frameSwapped);
        QTRY_VERIFY_WITH_TIMEOUT(frames.count() >= 3, 3000);
    }

    void stillLayersStopRequestingFrames()
    {
        const Scene scene = open(R"(layers: [{ kind: "gradient", speed: 0.0 }])");
        QTest::qWait(300);
        QSignalSpy frames(scene.window.get(), &QQuickWindow::frameSwapped);
        QTest::qWait(400);
        QCOMPARE(frames.count(), 0);
    }

    void reducedMotionStopsTheAnimation()
    {
        const Scene scene = open(R"(layers: [{ kind: "sweep", speed: 2.0 }]; reduceMotion: true)");
        QTest::qWait(300);
        QSignalSpy frames(scene.window.get(), &QQuickWindow::frameSwapped);
        QTest::qWait(400);
        QCOMPARE(frames.count(), 0);
        scene.item->setProperty("reduceMotion", false);
        QTRY_VERIFY_WITH_TIMEOUT(frames.count() >= 2, 3000);
    }

    void hidingTheItemStopsTheAnimation()
    {
        const Scene scene = open(R"(layers: [{ kind: "sweep", speed: 2.0 }])");
        QSignalSpy frames(scene.window.get(), &QQuickWindow::frameSwapped);
        QTRY_VERIFY_WITH_TIMEOUT(frames.count() >= 2, 3000);
        scene.item->setVisible(false);
        QTest::qWait(300);
        frames.clear();
        QTest::qWait(400);
        QCOMPARE(frames.count(), 0);
        scene.item->setVisible(true);
        QTRY_VERIFY_WITH_TIMEOUT(frames.count() >= 2, 3000);
    }

    void aSecondItemSharesTheRunningClock()
    {
        const Scene first = open(R"(layers: [{ kind: "sweep", speed: 2.0 }])");
        {
            const Scene second = open(R"(layers: [{ kind: "sweep", speed: 2.0 }])");
            QTest::qWait(150);
        }
        QSignalSpy frames(first.window.get(), &QQuickWindow::frameSwapped);
        QTRY_VERIFY_WITH_TIMEOUT(frames.count() >= 2, 3000);
    }

    void resizingAndChangingAppearanceRepaintsWithoutError()
    {
        const Scene scene = open(R"(layers: [{ kind: "gradient" }])");
        QSignalSpy frames(scene.window.get(), &QQuickWindow::frameSwapped);
        scene.item->setWidth(180);
        QTRY_VERIFY_WITH_TIMEOUT(frames.count() >= 1, 3000);
        frames.clear();
        scene.item->setProperty("zoom", 2.0);
        QTRY_VERIFY_WITH_TIMEOUT(frames.count() >= 1, 3000);
        QCOMPARE(scene.item->property("zoom").toDouble(), 2.0);
    }

private:
    using Scene = StyleScene;

    Scene open(const char *properties)
    {
        return openStyleScene(m_home, m_engine,
            QByteArray("Window { width: 300; height: 220; visible: true; BorderStyleItem { objectName: \"style\"; x: 40; y: 40; "
                       "width: 200; height: 120; ")
                + properties + " } }");
    }

    static double duration(QObject *item)
    {
        double seconds = 0.0;
        QMetaObject::invokeMethod(item, "eventDuration", Q_RETURN_ARG(double, seconds));
        return seconds;
    }

    SettingsHome m_home;
    QQmlEngine m_engine;
};

QTEST_MAIN(TestSettingsBorderStyleQml)
#include "test_settings_border_style_qml.moc"
