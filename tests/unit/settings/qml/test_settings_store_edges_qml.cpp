#include "settingsqmlharness.h"

#include <QDesktopServices>
#include <QKeySequence>

using namespace Konveyor::Settings::Testing;

namespace
{

class UrlRecorder : public QObject
{
    Q_OBJECT

public Q_SLOTS:
    void opened(const QUrl &url) { urls.append(url); }

public:
    QList<QUrl> urls;
};

QVariant gaps(QObject *store, const QString &path)
{
    return call<QVariantMap>(store, "scope", path).value(QStringLiteral("gaps"));
}

}

class TestSettingsStoreEdgesQml : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase() { QVERIFY(m_home.setUp()); }
    void init() { QVERIFY(m_home.installDefaults()); }

    void theConfigFileOpensInTheDesktopsEditor()
    {
        QQmlEngine engine;
        QObject *store = m_home.openStore(engine, QStringLiteral("layout {\n}\n"));
        UrlRecorder recorder;
        QDesktopServices::setUrlHandler(QStringLiteral("file"), &recorder, "opened");
        QVERIFY(QMetaObject::invokeMethod(store, "openConfigFile"));
        QDesktopServices::unsetUrlHandler(QStringLiteral("file"));
        QCOMPARE(recorder.urls, QList<QUrl> {QUrl::fromLocalFile(m_home.configPath())});
    }

    void aToggleOnABrokenConfigIsRefused()
    {
        QQmlEngine engine;
        QObject *store = m_home.openStore(engine, QStringLiteral("layout {\n"));
        QSignalSpy failed(store, SIGNAL(editFailed(QString)));
        QVERIFY(!call<bool>(store, "setToggle", QStringLiteral("layout/border"), true));
        QCOMPARE(failed.count(), 1);
        QCOMPARE(SettingsHome::read(m_home.configPath()), QStringLiteral("layout {\n"));
    }

    void settingTheSameValueSavesNothing()
    {
        QQmlEngine engine;
        QObject *store = m_home.openStore(engine, QStringLiteral("layout {\n    gaps 4\n}\n"));
        store->setProperty("autoSave", true);
        QSignalSpy saved(store, SIGNAL(saved()));
        QVERIFY(call<bool>(store, "setValue", QStringLiteral("layout/gaps"), QVariantList {4}, QVariantMap {}));
        QTest::qWait(SaveSettleMs);
        QCOMPARE(saved.count(), 0);
        QCOMPARE(store->property("needsSave").toBool(), false);
    }

    void aMissingConfigFolderIsCreatedOnTheFirstSave()
    {
        QQmlEngine engine;
        m_home.resetConfig(std::nullopt);
        QVERIFY(QDir(m_home.configDir()).removeRecursively());
        m_home.create(engine, "QtObject {}");
        QObject *store = SettingsHome::store(engine);
        QVERIFY(call<bool>(store, "setValue", QStringLiteral("layout/gaps"), QVariantList {5}, QVariantMap {}));
        QVERIFY(saveAndWait(store));
        QVERIFY(SettingsHome::read(m_home.configPath()).contains(QStringLiteral("gaps 5")));
    }

    void aDeletedConfigFileKeepsTheOpenDocument()
    {
        QQmlEngine engine;
        QObject *store = m_home.openStore(engine, QStringLiteral("layout {\n    gaps 4\n}\n"));
        QVERIFY(call<bool>(store, "setValue", QStringLiteral("layout/gaps"), QVariantList {6}, QVariantMap {}));
        QVERIFY(saveAndWait(store));
        QSignalSpy changed(store, SIGNAL(documentChanged()));
        QVERIFY(QFile::remove(m_home.configPath()));
        QTRY_VERIFY_WITH_TIMEOUT(changed.count() >= 1, SignalTimeoutMs);
        QCOMPARE(gaps(store, QStringLiteral("layout")).toInt(), 6);
        QVERIFY(saveAndWait(store));
        QVERIFY(SettingsHome::read(m_home.configPath()).contains(QStringLiteral("gaps 6")));
    }

    void anExternalEditDuringAPendingAutoSaveIsAdopted()
    {
        QQmlEngine engine;
        QObject *store = m_home.openStore(engine, QStringLiteral("layout {\n    gaps 4\n}\n"));
        store->setProperty("autoSave", true);
        QSignalSpy failed(store, SIGNAL(editFailed(QString)));
        QVERIFY(call<bool>(store, "setValue", QStringLiteral("layout/gaps"), QVariantList {6}, QVariantMap {}));
        QVERIFY(SettingsHome::write(m_home.configPath(), QStringLiteral("layout {\n    gaps 11\n}\n")));
        QTRY_VERIFY_WITH_TIMEOUT(failed.count() >= 1, SignalTimeoutMs);
        QTest::qWait(SaveSettleMs);
        QCOMPARE(gaps(store, QStringLiteral("layout")).toInt(), 11);
        QCOMPARE(SettingsHome::read(m_home.configPath()), QStringLiteral("layout {\n    gaps 11\n}\n"));
    }

    void keysAreNamedLikeTheKeybindSyntax()
    {
        QQmlEngine engine;
        QObject *store = m_home.openStore(engine, std::nullopt);
        QCOMPARE(call<QString>(store, "keyName", QKeySequence(Qt::Key_A)), QStringLiteral("A"));
        QCOMPARE(call<QString>(store, "keyName", QKeySequence(QKeyCombination(Qt::AltModifier, Qt::Key_F4))), QStringLiteral("Alt+F4"));
    }

    void scopesOfNodesWithoutAStringNameFallBackToTheDefaults()
    {
        for (const QString &head : {QStringLiteral("output"), QStringLiteral("output 5")}) {
            QQmlEngine engine;
            QObject *store = m_home.openStore(engine, QStringLiteral("%1 {\n    layout {\n        gaps 9\n    }\n}\n").arg(head));
            QVERIFY(!store->property("configError").toString().isEmpty());
            QCOMPARE(gaps(store, QStringLiteral("output/layout")).toInt(), 16);
        }
    }

private:
    static constexpr int SaveSettleMs = 700;

    SettingsHome m_home;
};

QTEST_MAIN(TestSettingsStoreEdgesQml)
#include "test_settings_store_edges_qml.moc"
