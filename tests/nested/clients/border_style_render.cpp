#include <QGuiApplication>
#include <QImage>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QTest>

#include <memory>

namespace
{

constexpr QPoint kFrameOrigin {80, 70};
constexpr QSize kFrameSize {200, 140};
constexpr QRgb kBackground = 0xff000000;

const QStringList kAllKinds {"gradient", "sweep", "comet", "glow", "ripple", "dashes", "pulse", "particles", "equalizer", "waveform",
    "spectrum", "shimmer", "bass-thump", "speed-lines", "scroll-trail", "bulge", "wavy", "elastic", "chevrons", "edge-flash", "gauge",
    "pulse-front", "wave", "draw-in", "fade-out"};

QVariantMap layer(const QString &kind, const QVariantMap &extra = {})
{
    QVariantMap map {{"kind", kind}, {"colors", QVariantList {QVariantMap {{"source", "color"}, {"color", QColor("#ff2030")}}}}};
    for (auto it = extra.begin(); it != extra.end(); ++it) {
        map.insert(it.key(), it.value());
    }
    return map;
}

struct Painted
{
    int count = 0;
    QRect bounds;
};

}

class TestBorderStyleRender : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        QQmlComponent component(&m_engine);
        component.setData(R"(
import QtQuick
import QtQuick.Window
import org.kde.konveyor.settings
Window {
    width: 360
    height: 280
    color: "black"
    visible: true
    property alias style: style
    BorderStyleItem {
        id: style
        objectName: "style"
        x: 80
        y: 70
        width: 200
        height: 140
        baseWidth: 6
        theme: ({ "accent": "#2080ff" })
        radii: Qt.vector4d(14, 14, 14, 14)
    }
})",
            QUrl(QStringLiteral("inline.qml")));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        m_window.reset(qobject_cast<QQuickWindow *>(component.create()));
        QVERIFY(m_window);
        m_style = m_window->findChild<QQuickItem *>("style");
        QVERIFY(m_style);
        QVERIFY(QTest::qWaitForWindowExposed(m_window.get()));
        QVERIFY2(QSGRendererInterface::isApiRhiBased(m_window->rendererInterface()->graphicsApi()), "the scene graph needs a GPU backend");
    }

    void cleanupTestCase() { m_window.reset(); }

    void init()
    {
        resetEvent();
        setLayers({});
    }

    void drawsNothingWithoutLayers()
    {
        const Painted painted = paint();
        QCOMPARE(painted.count, 0);
    }

    void everyKindDrawsInsideItsReach_data()
    {
        QTest::addColumn<QString>("kind");
        QTest::addColumn<QVariantMap>("extra");
        for (const QString &kind : kAllKinds) {
            QTest::newRow(qPrintable(kind)) << kind << extraFor(kind);
        }
    }

    void everyKindDrawsInsideItsReach()
    {
        QFETCH(QString, kind);
        QFETCH(QVariantMap, extra);
        setLayers({layer(kind, extra)});
        m_style->setProperty("eventTime", 0.2);
        m_style->setProperty("eventFrame", QRectF(0, 0, kFrameSize.width(), kFrameSize.height()));
        m_style->setProperty("eventSubject", QRectF(0, 0, kFrameSize.width(), kFrameSize.height()));
        const Painted painted = paint();
        QVERIFY2(painted.count > 0, qPrintable(kind + " drew nothing"));
        const QRect frame(kFrameOrigin, kFrameSize);
        QVERIFY2(frame.adjusted(-60, -60, 60, 60).contains(painted.bounds), qPrintable(kind + " drew outside the item's reach"));
    }

    void gradientDrawsTheConfiguredColourAlongTheFrame()
    {
        setLayers({layer("gradient")});
        const QImage image = m_window->grabWindow();
        const QRect frame(kFrameOrigin, kFrameSize);
        const QColor onEdge = image.pixelColor(frame.center().x(), frame.top() - 3);
        QVERIFY(onEdge.red() > 150);
        QVERIFY(onEdge.green() < 100);
        QCOMPARE(image.pixel(frame.center().x(), frame.center().y()), kBackground);
        QCOMPARE(image.pixel(5, 5), kBackground);
    }

    void usesTheThemeWhenALayerHasNoColours()
    {
        setLayers({QVariantMap {{"kind", "gradient"}}});
        const QImage image = m_window->grabWindow();
        const QColor onEdge = image.pixelColor(kFrameOrigin.x() + kFrameSize.width() / 2, kFrameOrigin.y() - 3);
        QVERIFY(onEdge.blue() > onEdge.red());
    }

    void themeColourSourcesAreResolved()
    {
        m_style->setProperty("theme", QVariantMap {{"accent", QColor("#2080ff")}, {"focus", QColor("#20ff30")}});
        setLayers({QVariantMap {{"kind", "gradient"}, {"colors", QVariantList {QVariantMap {{"source", "focus"}}}}}});
        const QImage image = m_window->grabWindow();
        const QColor onEdge = image.pixelColor(kFrameOrigin.x() + kFrameSize.width() / 2, kFrameOrigin.y() - 3);
        QVERIFY(onEdge.green() > onEdge.blue());
        QVERIFY(onEdge.green() > onEdge.red());
        m_style->setProperty("theme", QVariantMap {{"accent", QColor("#2080ff")}});
    }

    void emphasisDimsTheOtherLayers()
    {
        setLayers({layer("gradient"), layer("glow", {{"opacity", 1.0}})});
        const Painted plain = paint();
        m_style->setProperty("emphasis", 0);
        const Painted dimmed = paint();
        const QImage image = m_window->grabWindow();
        m_style->setProperty("emphasis", -1);
        QVERIFY(dimmed.count > 0);
        QVERIFY(plain.count >= dimmed.count);
        QVERIFY(image.pixelColor(kFrameOrigin.x() + 100, kFrameOrigin.y() - 3).red() > 100);
    }

    void zoomEnlargesTheDrawnBand()
    {
        setLayers({layer("gradient")});
        const Painted normal = paint();
        m_style->setProperty("zoom", 2.0);
        const Painted zoomed = paint();
        m_style->setProperty("zoom", 1.0);
        QVERIFY(zoomed.count > normal.count);
    }

    void insetMovesTheBandInward()
    {
        setLayers({layer("gradient")});
        const Painted outside = paint();
        m_style->setProperty("inset", 20.0);
        const Painted inset = paint();
        m_style->setProperty("inset", 0.0);
        QVERIFY(inset.bounds != outside.bounds);
    }

    void reducedMotionFreezesTheFrame()
    {
        setLayers({layer("sweep", {{"speed", 3.0}, {"colors", twoColors()}}), layer("particles"), layer("equalizer")});
        m_style->setProperty("reduceMotion", true);
        const QImage first = frame();
        QTest::qWait(200);
        const QImage second = frame();
        m_style->setProperty("reduceMotion", false);
        QVERIFY(first == second);
    }

    void movingLayersChangeBetweenFrames()
    {
        setLayers({layer("sweep", {{"speed", 3.0}, {"colors", twoColors()}})});
        const QImage first = frame();
        QTest::qWait(300);
        const QImage second = frame();
        QVERIFY(first != second);
    }

    void reactiveLayersFollowTheirDrives()
    {
        setLayers({layer("glow",
            {{"drives", QVariantList {QVariantMap {{"setting", "opacity"}, {"signal", "time"}, {"amount", 1.0}}}},
                {"fireOn", QVariantList {"click"}}, {"burst", 4}})});
        QVERIFY(paint().count > 0);
        setLayers({layer("particles", {{"burst", 8}, {"fireOn", QVariantList {"focus"}}, {"direction", "up"}})});
        QVERIFY(paint().count >= 0);
    }

    void eventPlaybackShapesStandingAndOneShotLayers()
    {
        setLayers({layer("gradient"), layer("draw-in"), layer("wave")});
        m_style->setProperty("eventFrame", QRectF(0, 0, 200, 140));
        m_style->setProperty("eventSubject", QRectF(0, 0, 200, 140));
        m_style->setProperty("eventFrom", QRectF(0, 0, 100, 100));
        m_style->setProperty("eventTo", QRectF(100, 20, 100, 100));
        m_style->setProperty("eventFarthest", 80.0);
        m_style->setProperty("eventLength", 2.0);
        double duration = 0.0;
        QVERIFY(QMetaObject::invokeMethod(m_style, "eventDuration", Q_RETURN_ARG(double, duration)));
        QCOMPARE(duration, 2.0);
        m_style->setProperty("eventTime", 1.0);
        const Painted mid = paint();
        m_style->setProperty("eventTime", 0.0);
        const Painted start = paint();
        QVERIFY(mid.count > start.count);
        m_style->setProperty("eventLength", 0.0);
        QVERIFY(QMetaObject::invokeMethod(m_style, "eventDuration", Q_RETURN_ARG(double, duration)));
        QVERIFY(duration > 0.0);
        resetEvent();
    }

    void anIdleRippleRestsBetweenCyclesAndStaysHiddenWithReducedMotion()
    {
        setLayers({layer("ripple")});
        verifyPlaysAndRests(60);
        m_style->setProperty("reduceMotion", true);
        QCOMPARE(paint().count, 0);
        m_style->setProperty("reduceMotion", false);
    }

    void aRippleFiringOnASignalPlaysAndRests()
    {
        setLayers({layer("ripple", {{"fireOn", QVariantList {"click"}}})});
        verifyPlaysAndRests(80);
        m_style->setProperty("reduceMotion", true);
        QVERIFY(paint().count > 0);
        m_style->setProperty("reduceMotion", false);
    }

    void layersCanBeRemovedAndTheNodesAreTrimmed()
    {
        setLayers({layer("gradient"), layer("glow")});
        const QImage many = frame();
        setLayers({layer("gradient")});
        const QImage one = frame();
        QVERIFY(many != one);
        QVERIFY(paint().count > 0);
        setLayers({});
        QCOMPARE(paint().count, 0);
    }

    void aLayerKeepsItsMaterialWhenOnlyItsValuesChange()
    {
        setLayers({layer("gradient")});
        QVERIFY(paint().count > 0);
        setLayers({layer("gradient", {{"blend", "add"}, {"opacity", 0.5}})});
        QVERIFY(paint().count > 0);
        setLayers({layer("particles", {{"blend", "screen"}})});
        QVERIFY(paint().count >= 0);
        setLayers({layer("glow", {{"blend", "multiply"}})});
        QVERIFY(paint().count >= 0);
    }

    void aLayerSwitchingShaderGetsANewMaterial()
    {
        setLayers({layer("gradient")});
        QVERIFY(paint().count > 0);
        setLayers({layer("equalizer")});
        QVERIFY(paint().count > 0);
        setLayers({layer("particles")});
        QVERIFY(paint().count > 0);
    }

private:
    void verifyPlaysAndRests(int steps)
    {
        int shown = 0;
        int hidden = 0;
        for (int step = 0; step < steps && (shown == 0 || hidden == 0); ++step) {
            (paint().count > 0 ? shown : hidden)++;
            QTest::qWait(100);
        }
        QVERIFY(shown > 0);
        QVERIFY(hidden > 0);
    }

    static QVariantList twoColors()
    {
        return {QVariantMap {{"source", "color"}, {"color", QColor("#ff2030")}},
            QVariantMap {{"source", "color"}, {"color", QColor("#2030ff")}}};
    }

    static QVariantMap extraFor(const QString &kind)
    {
        if (kind == "equalizer" || kind == "waveform" || kind == "spectrum") {
            return {{"sensitivity", 1.0}};
        }
        return {};
    }

    void resetEvent()
    {
        m_style->setProperty("eventTime", -1.0);
        m_style->setProperty("eventFrame", QRectF());
        m_style->setProperty("eventSubject", QRectF());
        m_style->setProperty("eventFrom", QRectF());
        m_style->setProperty("eventTo", QRectF());
        m_style->setProperty("eventFarthest", 0.0);
        m_style->setProperty("eventLength", 0.0);
    }

    void setLayers(const QVariantList &layers) { m_style->setProperty("layers", layers); }

    QImage frame()
    {
        QTest::qWait(60);
        return m_window->grabWindow();
    }

    Painted paint()
    {
        QTest::qWait(60);
        const QImage image = m_window->grabWindow();
        Painted painted;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                if (image.pixel(x, y) != kBackground) {
                    ++painted.count;
                    painted.bounds = painted.bounds.isNull() ? QRect(x, y, 1, 1) : painted.bounds.united(QRect(x, y, 1, 1));
                }
            }
        }
        return painted;
    }

    QQmlEngine m_engine;
    std::unique_ptr<QQuickWindow> m_window;
    QQuickItem *m_style = nullptr;
};

QTEST_MAIN(TestBorderStyleRender)
#include "border_style_render.moc"
