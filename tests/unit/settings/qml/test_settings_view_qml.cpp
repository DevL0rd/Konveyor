#include "settingsqmlharness.h"

#include <QQmlIncubationController>
#include <QQuickWindow>

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
            "    function wrapAround(on) { return LayoutKeys.writeFlag(SettingsStore, \"layout\", false, "
            "\"wrap-columns\", on, true) }\n"
            "}",
            &error);
        QVERIFY2(view, qPrintable(error));
        QObject *store = SettingsHome::store(engine);
        QVERIFY(store);
        QSignalSpy saved(store, SIGNAL(saved()));

        QVERIFY(QMetaObject::invokeMethod(view.get(), "wrapAround", Q_ARG(QVariant, false)));
        QVERIFY(saved.wait(SignalTimeoutMs));
        QVERIFY(SettingsHome::read(m_home.configPath()).contains(QStringLiteral("wrap-columns false")));
        QCOMPARE(call<QVariantMap>(store, "scope", QStringLiteral("layout")).value(QStringLiteral("wrap-columns")).toBool(), false);

        QVERIFY(QMetaObject::invokeMethod(view.get(), "wrapAround", Q_ARG(QVariant, true)));
        QVERIFY(saved.wait(SignalTimeoutMs));
        QVERIFY(!SettingsHome::read(m_home.configPath()).contains(QStringLiteral("wrap-columns false")));
        QCOMPARE(call<QVariantMap>(store, "scope", QStringLiteral("layout")).value(QStringLiteral("wrap-columns")).toBool(), true);
    }

    void incubatesSectionsInTheBackgroundOnce()
    {
        QQmlEngine engine;
        QString error;
        const std::unique_ptr<QObject> host
            = m_home.create(engine, "QtObject { function load() { SettingsNavigation.loadSectionsInBackground() } }", &error);
        QVERIFY2(host, qPrintable(error));
        QVERIFY(engine.incubationController() == nullptr);
        QVERIFY(QMetaObject::invokeMethod(host.get(), "load"));
        QQmlIncubationController *controller = engine.incubationController();
        QVERIFY(controller != nullptr);
        QVERIFY(QMetaObject::invokeMethod(host.get(), "load"));
        QCOMPARE(engine.incubationController(), controller);
    }

    void survivesSwitchingPagesWhileTheyLoad()
    {
        WarningLog log;
        QQmlEngine engine;
        QString error;
        const std::unique_ptr<QObject> root = m_home.create(engine,
            "import QtQuick.Window\n"
            "Window {\n"
            "    width: 1100; height: 800; visible: true\n"
            "    property alias view: settings\n"
            "    SettingsView { id: settings; anchors.fill: parent }\n"
            "}\n",
            &error);
        QVERIFY2(root, qPrintable(error));
        QVERIFY(QTest::qWaitForWindowExposed(qobject_cast<QQuickWindow *>(root.get())));
        QObject *view = root->property("view").value<QObject *>();
        QVERIFY(view);
        QQmlIncubationController *controller = engine.incubationController();
        QVERIFY(controller);
        const QStringList pages {QStringLiteral("appearance"), QStringLiteral("taskbar"), QStringLiteral("shortcuts")};
        int interrupted = 0;
        for (int step = 0; step < 200; ++step) {
            view->setProperty("pageId", pages.at(step % pages.size()));
            view->setProperty("active", step % 5 != 4);
            interrupted += controller->incubatingObjectCount() > 0 ? 1 : 0;
            controller->incubateFor(step % 4);
            QTest::qWait(0);
        }
        QVERIFY(interrupted > 0);
        QTRY_COMPARE_WITH_TIMEOUT(controller->incubatingObjectCount(), 0, SignalTimeoutMs);
        QCOMPARE(log.take(), QStringList {});
    }

private:
    SettingsHome m_home;
};

QTEST_MAIN(TestSettingsViewQml)
#include "test_settings_view_qml.moc"
