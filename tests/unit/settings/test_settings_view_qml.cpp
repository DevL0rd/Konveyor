#include <QDir>
#include <QFile>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QTemporaryDir>
#include <QTest>

#include <memory>

class TestSettingsViewQml : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        QVERIFY(m_home.isValid());
        const QDir home(m_home.path());
        QVERIFY(home.mkpath(QStringLiteral("config")));
        QVERIFY(home.mkpath(QStringLiteral("data/konveyor")));
        QVERIFY(home.mkpath(QStringLiteral("qml/org/kde/konveyor")));
        QVERIFY(QFile::copy(QStringLiteral(KONVEYOR_SOURCE_DIR "/data/default-config.kdl"),
            home.filePath(QStringLiteral("data/konveyor/default-config.kdl"))));
        QVERIFY(QFile::link(QStringLiteral(KONVEYOR_BINARY_DIR "/bin/org/kde/konveyor/settings"),
            home.filePath(QStringLiteral("qml/org/kde/konveyor/settings"))));
        QVERIFY(QFile::link(
            QStringLiteral(KONVEYOR_SOURCE_DIR "/src/components"), home.filePath(QStringLiteral("qml/org/kde/konveyor/components"))));
        qputenv("XDG_CONFIG_HOME", home.filePath(QStringLiteral("config")).toUtf8());
        qputenv("XDG_DATA_DIRS", home.filePath(QStringLiteral("data")).toUtf8());
    }

    void appliesChangesLive()
    {
        QQmlEngine engine;
        engine.addImportPath(QDir(m_home.path()).filePath(QStringLiteral("qml")));
        QQmlComponent component(&engine);
        component.setData("import QtQuick\nimport org.kde.konveyor.settings\nSettingsView { active: false; width: 900; height: 600 }",
            QUrl(QStringLiteral("inline:view.qml")));
        QTRY_VERIFY_WITH_TIMEOUT(component.isReady() || component.isError(), 5000);
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        std::unique_ptr<QObject> view(component.create());
        QVERIFY2(view, qPrintable(component.errorString()));

        auto *store = engine.singletonInstance<QObject *>(QStringLiteral("org.kde.konveyor.settings"), QStringLiteral("SettingsStore"));
        QVERIFY(store);
        QCOMPARE(store->property("autoSave").toBool(), true);

        bool changed = false;
        QVERIFY(QMetaObject::invokeMethod(store, "setValue", Q_RETURN_ARG(bool, changed), Q_ARG(QString, QStringLiteral("layout/gaps")),
            Q_ARG(QVariantList, QVariantList {27}), Q_ARG(QVariantMap, QVariantMap {})));
        QVERIFY(changed);

        const QString config = QDir(m_home.path()).filePath(QStringLiteral("config/konveyor/config.kdl"));
        QTRY_VERIFY_WITH_TIMEOUT(readText(config).contains(QStringLiteral("gaps 27")), 5000);
    }

private:
    static QString readText(const QString &path)
    {
        QFile file(path);
        return file.open(QIODevice::ReadOnly) ? QString::fromUtf8(file.readAll()) : QString();
    }

    QTemporaryDir m_home;
};

QTEST_MAIN(TestSettingsViewQml)
#include "test_settings_view_qml.moc"
