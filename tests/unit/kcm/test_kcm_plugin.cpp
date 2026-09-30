#include "../settings/qml/settingsqmlharness.h"

#include <KPluginMetaData>
#include <KQuickConfigModule>
#include <KQuickConfigModuleLoader>

#include <QQuickItem>
#include <QQuickWindow>
#include <QRegularExpression>

using namespace Konveyor::Settings::Testing;

namespace
{

QObject *findView(QObject *root)
{
    for (QObject *child : root->findChildren<QObject *>()) {
        if (QString::fromLatin1(child->metaObject()->className()).startsWith(QLatin1String("SettingsView"))) {
            return child;
        }
    }
    return nullptr;
}

QString firstPage()
{
    const QString pages = SettingsHome::read(QStringLiteral(KONVEYOR_SOURCE_DIR "/src/settings/qml/catalog/Pages.js"));
    return QRegularExpression(QStringLiteral("id: \"([a-z]+)\"")).match(pages).captured(1);
}

}

class TestKcmPlugin : public QObject
{
    Q_OBJECT

public:
    static void initMain()
    {
        qputenv("QT_QPA_PLATFORM", "offscreen");
        qputenv("DBUS_SESSION_BUS_ADDRESS", "unix:path=/nonexistent/konveyor-test-bus");
    }

private:
    SettingsHome m_home;
    std::shared_ptr<QQmlEngine> m_engine;

    std::unique_ptr<KQuickConfigModule> load(const QVariantList &args)
    {
        const auto result
            = KQuickConfigModuleLoader::loadModule(KPluginMetaData(QStringLiteral(KONVEYOR_KCM_PLUGIN)), nullptr, args, m_engine);
        if (!result) {
            qWarning() << result.errorString;
        }
        return std::unique_ptr<KQuickConfigModule>(result.plugin);
    }

    static QObject *viewOf(KQuickConfigModule &module)
    {
        QQuickItem *ui = module.mainUi();
        return ui ? findView(ui) : nullptr;
    }

private Q_SLOTS:
    void initTestCase()
    {
        QVERIFY(m_home.setUp());
        m_engine = std::make_shared<QQmlEngine>();
        m_home.addImports(*m_engine);
    }

    void cleanupTestCase() { m_engine.reset(); }

    void describesItselfToSystemSettings()
    {
        const KPluginMetaData metaData(QStringLiteral(KONVEYOR_KCM_PLUGIN));
        QVERIFY(metaData.isValid());
        QCOMPARE(metaData.pluginId(), QStringLiteral("kcm_konveyor"));
        QCOMPARE(metaData.name(), QStringLiteral("Konveyor"));
        QCOMPARE(metaData.iconName(), QStringLiteral("view-split-left-right"));
        QCOMPARE(metaData.value(QStringLiteral("X-KDE-System-Settings-Parent-Category")), QStringLiteral("windowmanagement"));
        QVERIFY(metaData.value(QStringLiteral("X-KDE-Keywords")).contains(QLatin1String("window rules")));
    }

    void opensOnTheFirstPageByDefault()
    {
        const auto module = load({});
        QVERIFY(module);
        QCOMPARE(module->buttons(), KAbstractConfigModule::NoAdditionalButton);
        QCOMPARE(module->property("initialPage").toString(), QString());
        QObject *view = viewOf(*module);
        QVERIFY2(view, qPrintable(module->errorString()));
        QCOMPARE(view->property("pageId").toString(), firstPage());
    }

    void opensOnThePageItIsGiven()
    {
        const auto module = load({QStringLiteral("rules")});
        QVERIFY(module);
        QCOMPARE(module->property("initialPage").toString(), QStringLiteral("rules"));
        QObject *view = viewOf(*module);
        QVERIFY2(view, qPrintable(module->errorString()));
        QCOMPARE(view->property("pageId").toString(), QStringLiteral("rules"));
    }

    void anotherActivationSwitchesThePage()
    {
        const auto module = load({QStringLiteral("rules")});
        QVERIFY(module);
        QObject *view = viewOf(*module);
        QVERIFY(view);
        QSignalSpy changed(module.get(), SIGNAL(initialPageChanged()));
        Q_EMIT module->activationRequested({QStringLiteral("monitors")});
        QCOMPARE(changed.count(), 1);
        QCOMPARE(module->property("initialPage").toString(), QStringLiteral("monitors"));
        QCOMPARE(view->property("pageId").toString(), QStringLiteral("monitors"));
        Q_EMIT module->activationRequested({});
        QCOMPARE(changed.count(), 2);
        QCOMPARE(view->property("pageId").toString(), QStringLiteral("monitors"));
        QMetaObject::invokeMethod(view, "pageChosen", Q_ARG(QString, QStringLiteral("look")));
        QCOMPARE(view->property("pageId").toString(), QStringLiteral("look"));
    }

    void undoTakesBackTheLastChange()
    {
        m_home.resetConfig(QStringLiteral("layout {\n    gaps 16\n}\n"));
        const auto module = load({});
        QVERIFY(module);
        QQuickItem *ui = module->mainUi();
        QVERIFY(ui);
        QQuickWindow window;
        ui->setParentItem(window.contentItem());
        window.resize(1200, 800);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QObject *store = SettingsHome::store(*m_engine);
        QVERIFY(store);
        QVERIFY(call<bool>(store, "setValue", QStringLiteral("layout/gaps"), QVariantList {27}, QVariantMap {}));
        QVERIFY(store->property("canUndo").toBool());
        window.requestActivate();
        QTest::keySequence(&window, QKeySequence::Undo);
        QTRY_VERIFY(!store->property("canUndo").toBool());
        QCOMPARE(call<QVariantMap>(store, "scope", QStringLiteral("layout")).value(QStringLiteral("gaps")).toInt(), 16);
        ui->setParentItem(nullptr);
    }
};

QTEST_MAIN(TestKcmPlugin)

#include "test_kcm_plugin.moc"
