#include <QDir>
#include <QElapsedTimer>
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
        const std::unique_ptr<QObject> view = createView(engine, "SettingsView { active: false; width: 900; height: 600 }");
        QVERIFY2(view, qPrintable(m_error));
        QObject *store = settingsStore(engine);
        QVERIFY(store);
        QCOMPARE(store->property("autoSave").toBool(), true);

        bool changed = false;
        QVERIFY(QMetaObject::invokeMethod(store, "setValue", Q_RETURN_ARG(bool, changed), Q_ARG(QString, QStringLiteral("layout/gaps")),
            Q_ARG(QVariantList, QVariantList {27}), Q_ARG(QVariantMap, QVariantMap {})));
        QVERIFY(changed);

        QTRY_VERIFY_WITH_TIMEOUT(readText(configPath()).contains(QStringLiteral("gaps 27")), 5000);
    }

    void switchesOffAFlagThatDefaultsToOn()
    {
        QQmlEngine engine;
        const std::unique_ptr<QObject> view = createView(engine,
            "import \"file://" KONVEYOR_SOURCE_DIR "/src/settings/qml/sections/LayoutKeys.js\" as LayoutKeys\n"
            "SettingsView {\n"
            "    active: false\n"
            "    function expandAlone(on) { return LayoutKeys.writeFlag(SettingsStore, \"layout\", false, "
            "\"always-expand-single-column\", on, true) }\n"
            "}");
        QVERIFY2(view, qPrintable(m_error));
        QObject *store = settingsStore(engine);
        QVERIFY(store);
        const QString config = configPath();

        QVERIFY(QMetaObject::invokeMethod(view.get(), "expandAlone", Q_ARG(QVariant, false)));
        QTRY_VERIFY_WITH_TIMEOUT(readText(config).contains(QStringLiteral("always-expand-single-column false")), 5000);
        QVariantMap layout;
        QVERIFY(QMetaObject::invokeMethod(store, "scope", Q_RETURN_ARG(QVariantMap, layout), Q_ARG(QString, QStringLiteral("layout"))));
        QCOMPARE(layout.value(QStringLiteral("always-expand-single-column")).toBool(), false);

        QVERIFY(QMetaObject::invokeMethod(view.get(), "expandAlone", Q_ARG(QVariant, true)));
        QTRY_VERIFY_WITH_TIMEOUT(!readText(config).contains(QStringLiteral("always-expand-single-column false")), 5000);
        QVERIFY(QMetaObject::invokeMethod(store, "scope", Q_RETURN_ARG(QVariantMap, layout), Q_ARG(QString, QStringLiteral("layout"))));
        QCOMPARE(layout.value(QStringLiteral("always-expand-single-column")).toBool(), true);
    }

private:
    std::unique_ptr<QObject> createView(QQmlEngine &engine, const QByteArray &body)
    {
        engine.addImportPath(QDir(m_home.path()).filePath(QStringLiteral("qml")));
        QQmlComponent component(&engine);
        component.setData("import QtQuick\nimport org.kde.konveyor.settings\n" + body, QUrl(QStringLiteral("inline:view.qml")));
        QElapsedTimer waited;
        waited.start();
        while (component.isLoading() && waited.elapsed() < 5000) {
            QTest::qWait(10);
        }
        std::unique_ptr<QObject> view(component.isReady() ? component.create() : nullptr);
        m_error = component.errorString();
        return view;
    }

    static QObject *settingsStore(QQmlEngine &engine)
    {
        return engine.singletonInstance<QObject *>(QStringLiteral("org.kde.konveyor.settings"), QStringLiteral("SettingsStore"));
    }

    QString configPath() const { return QDir(m_home.path()).filePath(QStringLiteral("config/konveyor/config.kdl")); }

    static QString readText(const QString &path)
    {
        QFile file(path);
        return file.open(QIODevice::ReadOnly) ? QString::fromUtf8(file.readAll()) : QString();
    }

    QTemporaryDir m_home;
    QString m_error;
};

QTEST_MAIN(TestSettingsViewQml)
#include "test_settings_view_qml.moc"
