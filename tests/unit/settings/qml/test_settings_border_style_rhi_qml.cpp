#include "config/types/borderstyle.h"
#include "stylescene.h"

#include <QGuiApplication>
#include <QQuickGraphicsDevice>
#include <QQuickItem>
#include <QQuickRenderControl>
#include <QQuickRenderTarget>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <rhi/qrhi.h>

namespace
{

class HostedControl : public QQuickRenderControl
{
public:
    explicit HostedControl(QWindow *host)
        : m_host(host)
    { }

    QWindow *renderWindow(QPoint *) override { return m_host; }

private:
    QWindow *m_host;
};

class Embedded
{
public:
    Embedded(QWindow *host, const QByteArray &properties)
        : control(host)
        , window(&control)
    {
        QRhiNullInitParams params;
        rhi.reset(QRhi::create(QRhi::Null, &params));
        window.setGraphicsDevice(QQuickGraphicsDevice::fromRhi(rhi.get()));
        control.initialize();
        texture.reset(rhi->newTexture(QRhiTexture::RGBA8, QSize(300, 220), 1, QRhiTexture::RenderTarget));
        texture->create();
        target.reset(rhi->newTextureRenderTarget({texture.get()}));
        pass.reset(target->newCompatibleRenderPassDescriptor());
        target->setRenderPassDescriptor(pass.get());
        target->create();
        window.setRenderTarget(QQuickRenderTarget::fromRhiRenderTarget(target.get()));
        window.contentItem()->setSize(QSizeF(300, 220));
        QString error;
        home.setUp();
        home.addImports(engine);
        created = home.create(engine, "BorderStyleItem { x: 40; y: 40; width: 200; height: 120; " + properties + " }", &error);
        qobject_cast<QQuickItem *>(created.get())->setParentItem(window.contentItem());
    }

    void render()
    {
        control.polishItems();
        control.beginFrame();
        control.sync();
        control.render();
        control.endFrame();
    }

    Konveyor::Settings::Testing::SettingsHome home;
    QQmlEngine engine;
    HostedControl control;
    QQuickWindow window;
    std::unique_ptr<QRhi> rhi;
    std::unique_ptr<QRhiTexture> texture;
    std::unique_ptr<QRhiTextureRenderTarget> target;
    std::unique_ptr<QRhiRenderPassDescriptor> pass;
    std::unique_ptr<QObject> created;
};

}

using namespace Konveyor::Settings::Testing;

class TestSettingsBorderStyleRhiQml : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase() { QVERIFY(m_home.setUp()); }

    void theNullBackendIsRhiBased()
    {
        const Scene scene = open("layers: [{ kind: 'gradient' }]");
        QCOMPARE(scene.window->rendererInterface()->graphicsApi(), QSGRendererInterface::Null);
    }

    void everyLayerKindBuildsAndPaintsItsNode()
    {
        WarningLog log;
        const Scene scene = open("radii: Qt.vector4d(8, 8, 8, 8)");
        QVariantList layers;
        for (const auto &[name, kind] : Konveyor::Config::borderLayerKinds()) {
            layers.append(QVariantMap {{"kind", name}});
        }
        scene.item->setProperty("layers", layers);
        paints(scene);
        QCOMPARE(log.take(), QStringList());
    }

    void layersUseTheirOwnPaintsAndFallBackToTheAccent()
    {
        WarningLog log;
        const Scene own = open("layers: [{ kind: 'gradient', colors: [{ source: 'color', color: '#ff0000' }, { source: 'accent' }] }]; "
                               "theme: ({ accent: '#00ff00' })");
        const Scene plain = open("layers: [{ kind: 'sweep' }]; theme: ({ accent: '#00ff00' })");
        paints(own);
        paints(plain);
        QCOMPARE(log.take(), QStringList());
    }

    void layersFollowTheirDrivesAndSignals_data()
    {
        QTest::addColumn<QByteArray>("properties");
        QTest::newRow("stacked bands") << QByteArray("layers: [{ kind: 'gradient' }, { kind: 'sweep' }, { kind: 'comet' }]");
        QTest::newRow("blends") << QByteArray(
            "layers: [{ kind: 'gradient', blend: 'screen' }, { kind: 'gradient', blend: 'multiply' }, { kind: 'gradient', blend: 'add' }]");
        QTest::newRow("only one event end") << QByteArray("layers: [{ kind: 'wave' }]; eventTime: 0.2; eventFrom: Qt.rect(0, 0, 100, 100)");
        QTest::newRow("driven particles") << QByteArray(
            "layers: [{ kind: 'particles', drives: [{ setting: 'opacity', signal: 'audio.level', amount: 1, smooth: 0.1 }] }]");
        QTest::newRow("equalizer") << QByteArray("layers: [{ kind: 'equalizer' }]");
        QTest::newRow("drive") << QByteArray(
            "layers: [{ kind: 'gradient', drives: [{ setting: 'color-mix', signal: 'audio.level', amount: 1, smooth: 0.1 }] }]");
        QTest::newRow("posed") << QByteArray("layers: [{ kind: 'shimmer' }]; reduceMotion: true");
        QTest::newRow("dimmed") << QByteArray("layers: [{ kind: 'gradient' }, { kind: 'equalizer' }]; emphasis: 0; zoom: 2");
        QTest::newRow("bursts") << QByteArray("layers: [{ kind: 'particles', burst: 3 }]");
        QTest::newRow("fired bursts") << QByteArray("layers: [{ kind: 'particles', fireOn: ['focus'] }]");
        QTest::newRow("resting ripple") << QByteArray("layers: [{ kind: 'ripple' }]");
        QTest::newRow("fired ripple") << QByteArray("layers: [{ kind: 'ripple', fireOn: ['focus'] }]");
        QTest::newRow("still ripple") << QByteArray("layers: [{ kind: 'ripple' }]; reduceMotion: true");
        QTest::newRow("event") << QByteArray(
            "layers: [{ kind: 'wave' }, { kind: 'gradient' }]; eventTime: 0.2; eventFrame: Qt.rect(0, 0, 100, 100); "
            "eventSubject: Qt.rect(0, 0, 100, 100); eventFrom: Qt.rect(0, 0, 100, 100); eventTo: Qt.rect(500, 0, 100, 100)");
    }

    void layersFollowTheirDrivesAndSignals()
    {
        QFETCH(QByteArray, properties);
        WarningLog log;
        paints(open(properties));
        QCOMPARE(log.take(), QStringList());
    }

    void changingTheLayersRebuildsTheirNodes()
    {
        WarningLog log;
        const Scene scene = open("layers: [{ kind: 'gradient' }, { kind: 'sweep' }, { kind: 'comet' }]");
        paints(scene);
        scene.item->setProperty("layers", QVariantList {QVariantMap {{"kind", "particles"}}, QVariantMap {{"kind", "equalizer"}}});
        paints(scene);
        scene.item->setProperty("layers", QVariantList {QVariantMap {{"kind", "gradient"}}});
        paints(scene);
        QCOMPARE(log.take(), QStringList());
    }

    void onlyItemsInsideTheViewportKeepRequestingFrames()
    {
        const Scene scene = openBody(
            "Window { width: 300; height: 220; visible: true; Flickable { objectName: 'flick'; anchors.fill: parent; contentWidth: 3000; "
            "contentHeight: 3000; BorderStyleItem { objectName: 'style'; x: 2000; y: 2000; width: 100; height: 100; "
            "layers: [{ kind: 'sweep', speed: 2.0 }] } } }");
        QTest::qWait(300);
        QSignalSpy frames(scene.window.get(), &QQuickWindow::frameSwapped);
        QTest::qWait(400);
        QCOMPARE(frames.count(), 0);
        scene.window->findChild<QQuickItem *>("flick")->setProperty("contentX", 1900);
        scene.window->findChild<QQuickItem *>("flick")->setProperty("contentY", 1900);
        QTRY_VERIFY_WITH_TIMEOUT(frames.count() >= 3, 5000);
    }

    void anItemEmbeddedInAnotherWindowWaitsForTheHostToBeShown()
    {
        QWindow host;
        Embedded embedded(&host, "layers: [{ kind: 'sweep', speed: 2.0 }]");
        embedded.render();
        QSignalSpy requested(&embedded.control, &QQuickRenderControl::sceneChanged);
        QTest::qWait(300);
        QCOMPARE(requested.count(), 0);
        host.show();
        QTest::qWait(500);
        QVERIFY(requested.count() >= 1);
    }

private:
    using Scene = StyleScene;

    static void paints(const Scene &scene)
    {
        QSignalSpy frames(scene.window.get(), &QQuickWindow::frameSwapped);
        QMetaObject::invokeMethod(scene.item, "update");
        QTRY_VERIFY_WITH_TIMEOUT(frames.count() >= 1, 5000);
    }

    Scene open(const QByteArray &properties)
    {
        return openBody(
            QByteArray("Window { width: 300; height: 220; visible: true; BorderStyleItem { objectName: \"style\"; x: 40; y: 40; "
                       "width: 200; height: 120; ")
            + properties + " } }");
    }

    Scene openBody(const QByteArray &window) { return openStyleScene(m_home, m_engine, window); }

    SettingsHome m_home;
    QQmlEngine m_engine;
};

int main(int argc, char **argv)
{
    QQuickWindow::setSceneGraphBackend(QStringLiteral("rhi"));
    QQuickWindow::setGraphicsApi(QSGRendererInterface::Null);
    QGuiApplication app(argc, argv);
    TestSettingsBorderStyleRhiQml test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_settings_border_style_rhi_qml.moc"
