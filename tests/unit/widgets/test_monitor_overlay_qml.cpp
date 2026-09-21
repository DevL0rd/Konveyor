#include <QFile>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QTemporaryDir>
#include <QTest>

#include <memory>

class TestMonitorOverlayQml : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void loads()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString source = QStringLiteral(KONVEYOR_SOURCE_DIR "/widgets/shared/MonitorOverlay.qml");
        const QString watcher = QStringLiteral(KONVEYOR_SOURCE_DIR "/widgets/shared/common/FileWatcher.qml");
        const QString componentPath = directory.filePath(QStringLiteral("MonitorOverlay.qml"));
        QVERIFY(QFile::copy(source, componentPath));
        QVERIFY(QFile::copy(watcher, directory.filePath(QStringLiteral("FileWatcher.qml"))));

        QQmlEngine engine;
        QQmlComponent content(&engine);
        content.setData("import QtQuick\nItem {}", QUrl(QStringLiteral("inline:compact.qml")));
        QQmlComponent popup(&engine);
        popup.setData("import QtQuick\nItem {}", QUrl(QStringLiteral("inline:popup.qml")));
        QTRY_VERIFY_WITH_TIMEOUT(content.isReady() || content.isError(), 5000);
        QTRY_VERIFY_WITH_TIMEOUT(popup.isReady() || popup.isError(), 5000);
        QVERIFY2(content.isReady(), qPrintable(content.errorString()));
        QVERIFY2(popup.isReady(), qPrintable(popup.errorString()));

        QQmlComponent component(&engine, QUrl::fromLocalFile(componentPath));
        QTRY_VERIFY_WITH_TIMEOUT(component.isReady() || component.isError(), 5000);
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        const QVariantMap properties {
            {QStringLiteral("slot"), 0},
            {QStringLiteral("content"), QVariant::fromValue(&content)},
            {QStringLiteral("popupContent"), QVariant::fromValue(&popup)},
            {QStringLiteral("active"), false},
        };
        std::unique_ptr<QObject> object(component.createWithInitialProperties(properties));
        QVERIFY2(object, qPrintable(component.errorString()));
    }

    void settlesWidthBeforeMapping()
    {
        QFile file(QStringLiteral(KONVEYOR_SOURCE_DIR "/widgets/shared/MonitorOverlay.qml"));
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QByteArray source = file.readAll();
        QVERIFY(source.contains("property bool readyToShow: false"));
        QVERIFY(source.contains("visible: readyToShow"));
        QVERIFY(source.contains("onDesiredWidthChanged: prepareToShow()"));
        QVERIFY(source.contains("card.prepareToShow()"));
        QVERIFY(source.contains("interval: 50"));
    }

    void detachedSurfacesUseWindowColors()
    {
        QFile file(QStringLiteral(KONVEYOR_SOURCE_DIR "/widgets/shared/MonitorOverlay.qml"));
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QByteArray source = file.readAll();
        QCOMPARE(source.count("Kirigami.Theme.inherit: false"), 2);
        QCOMPARE(source.count("Kirigami.Theme.colorSet: Kirigami.Theme.Window"), 2);
    }

    void detachedProcessColorsUseTheSurfaceTheme()
    {
        QFile rootFile(
            QStringLiteral(KONVEYOR_SOURCE_DIR "/widgets/process-monitor/plasmoids/org.devl0rd.procmon.panel/contents/ui/main.qml"));
        QVERIFY(rootFile.open(QIODevice::ReadOnly));
        const QByteArray rootSource = rootFile.readAll();
        QVERIFY(rootSource.contains("function heatColor(value, theme)"));
        QVERIFY(rootSource.contains("function colColor(p, column, theme)"));
        QVERIFY(rootSource.contains("function fpsColor(fps, theme)"));

        const QStringList viewNames {QStringLiteral("CompactView.qml"), QStringLiteral("FocusCard.qml"), QStringLiteral("ProcessRow.qml")};
        for (const QString &viewName : viewNames) {
            QFile viewFile(
                QStringLiteral(KONVEYOR_SOURCE_DIR "/widgets/process-monitor/plasmoids/org.devl0rd.procmon.panel/contents/ui/") + viewName);
            QVERIFY2(viewFile.open(QIODevice::ReadOnly), qPrintable(viewName));
            const QByteArray viewSource = viewFile.readAll();
            QVERIFY2(viewSource.contains("Kirigami.Theme"), qPrintable(viewName));
            QVERIFY2(!viewSource.contains("root.fpsColor(compact.proc ? compact.proc.fps : 0)"), qPrintable(viewName));
            QVERIFY2(!viewSource.contains("root.colColor(row.proc, modelData)"), qPrintable(viewName));
        }
    }

    void detachedSystemMetricColorsUseTheSurfaceTheme()
    {
        QFile file(QStringLiteral(KONVEYOR_SOURCE_DIR "/widgets/shared/common/PopMetricTabs.qml"));
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QByteArray source = file.readAll();
        QVERIFY(source.contains("tab.modelData.color(tab.value, Kirigami.Theme)"));
    }

    void offlineRouterPanelDoesNotBleedThroughText()
    {
        QFile file(QStringLiteral(KONVEYOR_SOURCE_DIR "/widgets/router-monitor/shared/lib/StatusOverlay.qml"));
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QByteArray source = file.readAll();
        QVERIFY(source.contains("color: Kirigami.Theme.backgroundColor"));
        QVERIFY(!source.contains("color: Qt.alpha(Kirigami.Theme.backgroundColor"));
    }
};

QTEST_MAIN(TestMonitorOverlayQml)

#include "test_monitor_overlay_qml.moc"
