#include <QDir>
#include <QFile>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QTest>

#include <memory>

class TestSceneAnimationQml : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void switchingScenesWarnsNothing();
};

void TestSceneAnimationQml::switchingScenesWarnsNothing()
{
    QTemporaryDir imports;
    QVERIFY(imports.isValid());
    const QDir source(QStringLiteral(KONVEYOR_SOURCE_DIR "/src/components"));
    const QString module = imports.filePath(QStringLiteral("org/kde/konveyor/components"));
    QVERIFY(QDir().mkpath(module));
    const QStringList files = source.entryList({QStringLiteral("qmldir"), QStringLiteral("*.qml"), QStringLiteral("*.js")}, QDir::Files);
    for (const QString &name : files) {
        QVERIFY(QFile::copy(source.filePath(name), module + QLatin1Char('/') + name));
    }

    QTest::failOnWarning(QRegularExpression(QStringLiteral("org/kde/konveyor/components/")));
    QQmlEngine engine;
    engine.addImportPath(imports.path());
    QQmlComponent component(&engine);
    component.setData("import QtQuick\nimport org.kde.konveyor.components\nSceneAnimation { width: 480; height: 300; showKeys: true }",
        QUrl(QStringLiteral("inline:scene.qml")));
    QTRY_VERIFY_WITH_TIMEOUT(component.isReady() || component.isError(), 5000);
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    std::unique_ptr<QObject> demo(component.create());
    QVERIFY2(demo, qPrintable(component.errorString()));

    const QStringList actions {
        QStringLiteral("focus-column-left"),
        QStringLiteral("focus-column-first"),
        QStringLiteral("focus-column-right"),
        QStringLiteral("consume-window-into-column"),
        QStringLiteral("toggle-column-tabbed-display"),
        QStringLiteral("focus-monitor-left"),
        QStringLiteral("focus-workspace-down"),
        QString(),
        QStringLiteral("not-an-action"),
        QStringLiteral("fullscreen-window"),
        QStringLiteral("close-window"),
        QStringLiteral("focus-column-last"),
        QStringLiteral("focus-monitor-right"),
        QStringLiteral("focus-column-left"),
    };
    for (const bool animated : {true, false, true}) {
        demo->setProperty("animated", animated);
        for (const QString &action : actions) {
            demo->setProperty("actionId", action);
            QTest::qWait(150);
        }
    }
    QVERIFY(demo->property("hasScene").toBool());
}

QTEST_MAIN(TestSceneAnimationQml)

#include "test_scene_animation_qml.moc"
