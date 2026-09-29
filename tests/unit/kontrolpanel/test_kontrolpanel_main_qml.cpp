#include "kontrolpanelservice.h"

#include <QDir>

#include <QFile>
#include <QGuiApplication>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQmlPropertyMap>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QWindow>
#include <algorithm>

#include <memory>

using Konveyor::KontrolPanelService;

namespace
{

bool write(const QString &path, const QByteArray &contents)
{
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(contents) == contents.size();
}

}

class TestKontrolPanelMainQml : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init()
    {
        m_directory = std::make_unique<QTemporaryDir>();
        QVERIFY(m_directory->isValid());
        const QDir directory(m_directory->path());
        QVERIFY(QFile::copy(
            QStringLiteral(KONVEYOR_SOURCE_DIR "/widgets/portals/kontrol-panel/Main.qml"), directory.filePath(QStringLiteral("Main.qml"))));
        QVERIFY(write(directory.filePath(QStringLiteral("Overlay.qml")), "import QtQuick\nItem {}\n"));
        QVERIFY(write(directory.filePath(QStringLiteral("ConfigWindow.qml")),
            "import QtQuick\nimport QtQuick.Window\nWindow { title: \"config-stub\" }\n"));
        m_config = std::make_unique<QQmlPropertyMap>();
        m_config->insert(QStringLiteral("favoritesClient"), QStringLiteral("org.devl0rd.kontrolpanel.favorites"));
        m_config->insert(QStringLiteral("openPageOnStart"), QString());
    }

    void cleanup()
    {
        m_root.reset();
        m_engine.reset();
        m_service.reset();
        m_config.reset();
        m_directory.reset();
    }

    void toggleOpensAndCloses()
    {
        QVERIFY(load());
        m_service->Toggle();
        QVERIFY(m_root->property("open").toBool());
        QVERIFY(m_service->IsOpen());
        m_service->Toggle();
        QVERIFY(!m_root->property("open").toBool());
        QVERIFY(!m_service->IsOpen());
    }

    void openingAPageRemembersItUntilTheViewOpens()
    {
        QVERIFY(load());
        m_service->Open(QStringLiteral("games"));
        QVERIFY(m_root->property("open").toBool());
        QCOMPARE(m_root->property("requestedPage").toString(), QStringLiteral("games"));
    }

    void openingTheShownPageClosesIt()
    {
        QVERIFY(load());
        m_service->Toggle();
        m_root->setProperty("currentPage", QStringLiteral("shortcuts"));
        m_service->Open(QStringLiteral("shortcuts"));
        QVERIFY(!m_root->property("open").toBool());
    }

    void openingAnotherPageSwitchesToIt()
    {
        QVERIFY(load());
        QSignalSpy switched(m_root.get(), SIGNAL(pageRequested(QString)));
        m_service->Toggle();
        m_root->setProperty("currentPage", QStringLiteral("home"));
        m_service->Open(QStringLiteral("games"));
        QVERIFY(m_root->property("open").toBool());
        QCOMPARE(switched.count(), 1);
        QCOMPARE(switched.first().first().toString(), QStringLiteral("games"));
    }

    void hideCloses()
    {
        QVERIFY(load());
        m_service->Toggle();
        m_service->Hide();
        QVERIFY(!m_root->property("open").toBool());
    }

    void pinsOnlyTakeDesktopFiles()
    {
        QVERIFY(load());
        QSignalSpy requested(m_root.get(), SIGNAL(pinsRequested()));
        m_service->Pin({QStringLiteral("/usr/share/applications/org.kde.konsole.desktop"), QStringLiteral("/home/user/notes.txt")});
        QCOMPARE(m_root->property("pendingPins").toStringList(),
            QStringList {QStringLiteral("/usr/share/applications/org.kde.konsole.desktop")});
        QCOMPARE(requested.count(), 1);
    }

    void configureClosesThePanelAndShowsTheSettings()
    {
        QVERIFY(load());
        m_service->Toggle();
        m_service->Configure();
        QVERIFY(!m_root->property("open").toBool());
        QVERIFY(configWindowShown());
    }

    void hostsTheSharedViewWithTheConfig()
    {
        QVERIFY(load());
        QCOMPARE(m_root->property("launcherConfig").value<QObject *>(), m_config.get());
        QCOMPARE(m_root->property("favoritesClient").toString(), QStringLiteral("org.devl0rd.kontrolpanel.favorites"));
        QVERIFY(m_root->property("kickerApplet").isNull() || !m_root->property("kickerApplet").value<QObject *>());
    }

    void openPageOnStartWaitsForKonveyor()
    {
        m_config->insert(QStringLiteral("openPageOnStart"), QStringLiteral("shortcuts"));
        QVERIFY(load());
        QVERIFY(!m_root->property("open").toBool());
        QCOMPARE(m_config->value(QStringLiteral("openPageOnStart")).toString(), QStringLiteral("shortcuts"));
    }

private:
    bool load()
    {
        m_service = std::make_unique<KontrolPanelService>();
        m_engine = std::make_unique<QQmlEngine>();
        QQmlComponent component(m_engine.get(), QUrl::fromLocalFile(QDir(m_directory->path()).filePath(QStringLiteral("Main.qml"))));
        m_root.reset(component.createWithInitialProperties({{QStringLiteral("service"), QVariant::fromValue<QObject *>(m_service.get())},
            {QStringLiteral("config"), QVariant::fromValue<QObject *>(m_config.get())}}));
        if (!m_root) {
            qWarning() << component.errorString();
        }
        return m_root != nullptr;
    }

    static bool configWindowShown()
    {
        const QWindowList windows = QGuiApplication::topLevelWindows();
        return std::any_of(
            windows.cbegin(), windows.cend(), [](const QWindow *window) { return window->title() == QLatin1String("config-stub"); });
    }

    std::unique_ptr<QTemporaryDir> m_directory;
    std::unique_ptr<QQmlPropertyMap> m_config;
    std::unique_ptr<KontrolPanelService> m_service;
    std::unique_ptr<QQmlEngine> m_engine;
    std::unique_ptr<QObject> m_root;
};

QTEST_MAIN(TestKontrolPanelMainQml)
#include "test_kontrolpanel_main_qml.moc"
