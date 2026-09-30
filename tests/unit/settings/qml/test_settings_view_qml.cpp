#include "settingsqmlharness.h"

using namespace Konveyor::Settings::Testing;

class TestSettingsViewQml : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase() { QVERIFY(m_home.setUp()); }
    void init() { m_home.resetConfig(std::nullopt); }

    void appliesChangesLive()
    {
        QQmlEngine engine;
        QString error;
        const std::unique_ptr<QObject> view = m_home.create(engine, "SettingsView { active: false; width: 900; height: 600 }", &error);
        QVERIFY2(view, qPrintable(error));
        QObject *store = SettingsHome::store(engine);
        QVERIFY(store);
        QCOMPARE(store->property("autoSave").toBool(), true);

        QSignalSpy saved(store, SIGNAL(saved()));
        QVERIFY(call<bool>(store, "setValue", QStringLiteral("layout/gaps"), QVariantList {27}, QVariantMap {}));
        QVERIFY(saved.wait(SignalTimeoutMs));
        QVERIFY(SettingsHome::read(m_home.configPath()).contains(QStringLiteral("gaps 27")));
    }

    void switchesOffAFlagThatDefaultsToOn()
    {
        QQmlEngine engine;
        QString error;
        const std::unique_ptr<QObject> view = m_home.create(engine,
            "import \"sections/LayoutKeys.js\" as LayoutKeys\n"
            "SettingsView {\n"
            "    active: false\n"
            "    function expandAlone(on) { return LayoutKeys.writeFlag(SettingsStore, \"layout\", false, "
            "\"always-expand-single-column\", on, true) }\n"
            "}",
            &error);
        QVERIFY2(view, qPrintable(error));
        QObject *store = SettingsHome::store(engine);
        QVERIFY(store);
        QSignalSpy saved(store, SIGNAL(saved()));

        QVERIFY(QMetaObject::invokeMethod(view.get(), "expandAlone", Q_ARG(QVariant, false)));
        QVERIFY(saved.wait(SignalTimeoutMs));
        QVERIFY(SettingsHome::read(m_home.configPath()).contains(QStringLiteral("always-expand-single-column false")));
        QCOMPARE(call<QVariantMap>(store, "scope", QStringLiteral("layout")).value(QStringLiteral("always-expand-single-column")).toBool(),
            false);

        QVERIFY(QMetaObject::invokeMethod(view.get(), "expandAlone", Q_ARG(QVariant, true)));
        QVERIFY(saved.wait(SignalTimeoutMs));
        QVERIFY(!SettingsHome::read(m_home.configPath()).contains(QStringLiteral("always-expand-single-column false")));
        QCOMPARE(call<QVariantMap>(store, "scope", QStringLiteral("layout")).value(QStringLiteral("always-expand-single-column")).toBool(),
            true);
    }

private:
    SettingsHome m_home;
};

QTEST_MAIN(TestSettingsViewQml)
#include "test_settings_view_qml.moc"
