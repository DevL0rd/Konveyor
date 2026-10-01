#include "../dbus/fakekglobalaccel.h"

#include <KConfig>
#include <KConfigGroup>

#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusMetaType>
#include <QDBusReply>
#include <QDBusServiceWatcher>
#include <QSignalSpy>
#include <QTest>

#include <csignal>
#include <memory>

using Konveyor::Test::FakeKGlobalAccel;
using Konveyor::Test::FakeService;
using Konveyor::Test::PrivateSession;
using Konveyor::Test::ProgramResult;
using Konveyor::Test::writeFile;

namespace
{

const QString busName = QStringLiteral("org.devl0rd.KontrolPanel");

const QByteArray workingMain = R"(import QtQuick
QtObject {
    required property QtObject service
    required property var config
    property Connections toggles: Connections {
        target: service
        function onToggleRequested() { service.setOpen(!service.isOpen) }
    }
    Component.onCompleted: config.tileSize = 64
}
)";

const QByteArray pageRecordingOverlay = R"(import QtQuick
Item {
    Connections {
        target: root
        function onOpenChanged() { root.config.appsCategory = root.requestedPage }
    }
}
)";

}

class TestKontrolPanelMain : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        m_session = std::make_unique<PrivateSession>();
        QVERIFY(m_session->start());
        m_directory = m_session->dir(QStringLiteral("data")) + QStringLiteral("/kontrol-panel");
    }

    void cleanupTestCase() { m_session.reset(); }

    void init()
    {
        QVERIFY(QDir(m_directory).removeRecursively());
        QVERIFY(QDir().mkpath(m_directory + QStringLiteral("/config")));
        QVERIFY(QFile::copy(QStringLiteral(KONVEYOR_SOURCE_DIR "/widgets/portals/kontrol-panel/config/main.xml"),
            m_directory + QStringLiteral("/config/main.xml")));
        QVERIFY(writeFile(m_directory + QStringLiteral("/Main.qml"), workingMain));
    }

    void cleanup()
    {
        if (m_process) {
            m_process->kill();
            m_process->waitForFinished();
        }
        m_process.reset();
    }

    void needsExactlyOneDirectory_data()
    {
        QTest::addColumn<QStringList>("arguments");
        QTest::newRow("none") << QStringList {};
        QTest::newRow("two") << QStringList {QStringLiteral("a"), QStringLiteral("b")};
    }

    void needsExactlyOneDirectory()
    {
        QFETCH(QStringList, arguments);
        const ProgramResult result = m_session->run(QStringLiteral(KONVEYOR_KONTROL_PANEL), arguments);
        QCOMPARE(result.exitCode, 2);
        QVERIFY(result.err.contains(QStringLiteral("usage: konveyor-kontrol-panel DIRECTORY")));
    }

    void needsTheConfigSchema()
    {
        QVERIFY(QFile::remove(m_directory + QStringLiteral("/config/main.xml")));
        const ProgramResult result = m_session->run(QStringLiteral(KONVEYOR_KONTROL_PANEL), {m_directory});
        QCOMPARE(result.exitCode, 1);
        QVERIFY(result.err.contains(QStringLiteral("cannot read %1/config/main.xml").arg(m_directory)));
    }

    void refusesToRunTwice()
    {
        FakeService other(busName, QStringLiteral("/KontrolPanel"), [](const QDBusMessage &message) { return message.createReply(); });
        QVERIFY(other.start(m_session->address()));
        const ProgramResult result = m_session->run(QStringLiteral(KONVEYOR_KONTROL_PANEL), {m_directory});
        QCOMPARE(result.exitCode, 1);
        QVERIFY2(
            result.err.contains(QStringLiteral("could not register org.devl0rd.KontrolPanel on the session bus; is it already running?")),
            qPrintable(result.err));
    }

    void brokenMainQmlExitsWithoutClaimingTheBusName()
    {
        QVERIFY(writeFile(m_directory + QStringLiteral("/Main.qml"), "import QtQuick\nItem {\n"));
        QDBusServiceWatcher watcher(busName, QDBusConnection::sessionBus(), QDBusServiceWatcher::WatchForRegistration);
        QSignalSpy registered(&watcher, &QDBusServiceWatcher::serviceRegistered);
        FakeKGlobalAccel kglobalaccel;
        QVERIFY(kglobalaccel.start(m_session->address()));
        const ProgramResult result = m_session->run(QStringLiteral(KONVEYOR_KONTROL_PANEL), {m_directory});
        QCOMPARE(result.exitCode, 1);
        QVERIFY2(result.err.contains(QStringLiteral("Main.qml")), qPrintable(result.err));
        QVERIFY(QDBusConnection::sessionBus().interface()->isServiceRegistered(QStringLiteral("org.freedesktop.DBus")));
        QCoreApplication::processEvents();
        QCOMPARE(registered.count(), 0);
        FakeKGlobalAccel::settle();
        QVERIFY(kglobalaccel.calls(QStringLiteral("doRegister")).isEmpty());
    }

    void startsRegistersItsShortcutAndAnswersOnTheBus()
    {
        FakeKGlobalAccel kglobalaccel;
        QVERIFY(kglobalaccel.start(m_session->address()));
        start();
        const QStringList actionId {QStringLiteral("konveyor-kontrol-panel"), QStringLiteral("toggle"), QStringLiteral("Kontrol Panel"),
            QStringLiteral("Open or close the Kontrol Panel")};
        QDBusInterface panel(busName, QStringLiteral("/KontrolPanel"), busName, QDBusConnection::sessionBus());
        QCOMPARE(QDBusReply<bool>(panel.call(QStringLiteral("IsOpen"))).value(), false);
        FakeKGlobalAccel::settle();
        QVERIFY(!kglobalaccel.calls(QStringLiteral("setShortcutKeys")).isEmpty());
        QCOMPARE(kglobalaccel.calls(QStringLiteral("doRegister")).first().arguments().value(0).toStringList(), actionId);
        const QDBusMessage set = kglobalaccel.calls(QStringLiteral("setShortcutKeys")).first();
        QCOMPARE(set.arguments().value(0).toStringList(), actionId);
        QCOMPARE(FakeKGlobalAccel::keysOf(set.arguments().value(1)),
            QList<QKeySequence>({QKeySequence(Qt::Key_Meta), QKeySequence(Qt::ALT | Qt::Key_F1)}));

        QVERIFY(panel.call(QStringLiteral("Toggle")).type() == QDBusMessage::ReplyMessage);
        QCOMPARE(QDBusReply<bool>(panel.call(QStringLiteral("IsOpen"))).value(), true);
        QCOMPARE(panel.call(QStringLiteral("Open"), QStringLiteral("games")).type(), QDBusMessage::ReplyMessage);
        QCOMPARE(panel.call(QStringLiteral("Hide")).type(), QDBusMessage::ReplyMessage);
        QCOMPARE(panel.call(QStringLiteral("Pin"), QStringList {QStringLiteral("/a.desktop")}).type(), QDBusMessage::ReplyMessage);
        QCOMPARE(panel.call(QStringLiteral("Configure")).type(), QDBusMessage::ReplyMessage);
        QCOMPARE(panel.call(QStringLiteral("setOpen"), false).type(), QDBusMessage::ErrorMessage);
    }

    void keepsAShortcutTheUserChanged()
    {
        FakeKGlobalAccel kglobalaccel;
        QVERIFY(kglobalaccel.start(m_session->address()));
        const QKeySequence metaSpace(QStringLiteral("Meta+Space"));
        kglobalaccel.add({QStringLiteral("konveyor-kontrol-panel"), QStringLiteral("toggle"), QStringLiteral("Kontrol Panel"),
                             QStringLiteral("Open or close the Kontrol Panel")},
            {metaSpace});
        start();
        QDBusInterface panel(busName, QStringLiteral("/KontrolPanel"), busName, QDBusConnection::sessionBus());
        QVERIFY(panel.call(QStringLiteral("IsOpen")).type() == QDBusMessage::ReplyMessage);
        FakeKGlobalAccel::settle();
        QVERIFY(!kglobalaccel.calls(QStringLiteral("setShortcutKeys")).isEmpty());
        QCOMPARE(kglobalaccel.keys(QStringLiteral("konveyor-kontrol-panel"), QStringLiteral("toggle")), QList<QKeySequence> {metaSpace});
    }

    void savesSettingsChangesToKontrolpanelrc()
    {
        start();
        const QString path = m_session->dir(QStringLiteral("config")) + QStringLiteral("/konveyor/kontrolpanelrc");
        QVERIFY(QFile::exists(path));
        const KConfig config(path, KConfig::SimpleConfig);
        QCOMPARE(config.group(QStringLiteral("General")).readEntry("tileSize", 0), 64);
    }

    void panelUsesPlasmaColors_data()
    {
        QTest::addColumn<QString>("plasmaTheme");
        QTest::addColumn<QString>("applicationScheme");
        QTest::addColumn<QString>("expectedColors");
        QTest::newRow("Breeze Twilight") << QStringLiteral("breeze-dark") << QStringLiteral("BreezeLight")
                                         << QStringLiteral("#fcfcfc;#202326;#fcfcfc");
        QTest::newRow("light Plasma with dark applications")
            << QStringLiteral("breeze-light") << QStringLiteral("BreezeDark") << QStringLiteral("#232629;#eff0f1;#232629");
    }

    void panelUsesPlasmaColors()
    {
        QFETCH(QString, plasmaTheme);
        QFETCH(QString, applicationScheme);
        QFETCH(QString, expectedColors);
        const QString configDirectory = m_session->dir(QStringLiteral("config"));
        QVERIFY(writeFile(configDirectory + QStringLiteral("/plasmarc"), QStringLiteral("[Theme]\nname=%1\n").arg(plasmaTheme).toUtf8()));
        QFile applicationColors(QStringLiteral("/usr/share/color-schemes/%1.colors").arg(applicationScheme));
        QVERIFY(applicationColors.open(QIODevice::ReadOnly));
        QVERIFY(writeFile(configDirectory + QStringLiteral("/kdeglobals"), applicationColors.readAll()));
        QVERIFY(writeFile(configPath(), "[General]\nappsCategory=\n"));
        QVERIFY(writeFile(m_directory + QStringLiteral("/Main.qml"), R"(import QtQuick
import QtQuick.Window
import org.kde.ksvg as KSvg
import org.kde.plasma.components as PlasmaComponents
import org.kde.kirigami as Kirigami
Window {
    visible: true
    required property QtObject service
    required property var config
    readonly property color ink: Kirigami.Theme.textColor
    KSvg.FrameSvgItem { anchors.fill: parent; imagePath: "dialogs/background" }
    PlasmaComponents.Label { id: label; text: "Kontrol Panel" }
    Timer {
        interval: 100
        running: true
        onTriggered: config.appsCategory = ink + ";" + Kirigami.Theme.backgroundColor + ";" + label.color
    }
}
)"));
        start();
        KConfig config(configPath(), KConfig::SimpleConfig);
        QString colors;
        QTRY_VERIFY(([&] {
            config.reparseConfiguration();
            colors = config.group(QStringLiteral("General")).readEntry("appsCategory");
            return colors.contains(QLatin1Char(';'));
        })());
        qInfo().noquote() << "Plasma text;background;label:" << colors;
        QCOMPARE(colors, expectedColors);
        QVERIFY(QFile::remove(configDirectory + QStringLiteral("/plasmarc")));
        QVERIFY(QFile::remove(configDirectory + QStringLiteral("/kdeglobals")));
    }

    void runsWithoutKGlobalAccel()
    {
        start();
        QVERIFY(m_process->state() == QProcess::Running);
    }

    void theFirstStartAfterInstallOpensTheRequestedPageOnceKonveyorRuns()
    {
        useRealMainWithPageRecordingOverlay();
        FakeService konveyor(QStringLiteral("org.kde.Konveyor"), QStringLiteral("/Konveyor"),
            [](const QDBusMessage &message) { return message.createReply(); });
        QVERIFY(konveyor.start(m_session->address()));
        QVERIFY(writeFile(configPath(), "[General]\nopenPageOnStart=shortcuts\n"));
        start();
        QDBusInterface panel(busName, QStringLiteral("/KontrolPanel"), busName, QDBusConnection::sessionBus());
        QCOMPARE(QDBusReply<bool>(panel.call(QStringLiteral("IsOpen"))).value(), true);
        const KConfig config(configPath(), KConfig::SimpleConfig);
        QCOMPARE(config.group(QStringLiteral("General")).readEntry("openPageOnStart"), QString());
        QCOMPARE(config.group(QStringLiteral("General")).readEntry("appsCategory"), QStringLiteral("shortcuts"));
    }

    void theRequestedPageOpensWhenKonveyorStartsAfterThePanelAndOnlyOnce()
    {
        useRealMainWithPageRecordingOverlay();
        QVERIFY(writeFile(configPath(), "[General]\nopenPageOnStart=shortcuts\n"));
        start();
        QDBusInterface panel(busName, QStringLiteral("/KontrolPanel"), busName, QDBusConnection::sessionBus());
        QCOMPARE(QDBusReply<bool>(panel.call(QStringLiteral("IsOpen"))).value(), false);
        const auto reply = [](const QDBusMessage &message) { return message.createReply(); };
        auto konveyor = std::make_unique<FakeService>(QStringLiteral("org.kde.Konveyor"), QStringLiteral("/Konveyor"), reply);
        QVERIFY(konveyor->start(m_session->address()));
        QCOMPARE(QDBusReply<bool>(panel.call(QStringLiteral("IsOpen"))).value(), true);
        const KConfig config(configPath(), KConfig::SimpleConfig);
        QCOMPARE(config.group(QStringLiteral("General")).readEntry("openPageOnStart"), QString());
        QCOMPARE(config.group(QStringLiteral("General")).readEntry("appsCategory"), QStringLiteral("shortcuts"));

        QCOMPARE(panel.call(QStringLiteral("Hide")).type(), QDBusMessage::ReplyMessage);
        konveyor.reset();
        konveyor = std::make_unique<FakeService>(QStringLiteral("org.kde.Konveyor"), QStringLiteral("/Konveyor"), reply);
        QVERIFY(konveyor->start(m_session->address()));
        QCOMPARE(QDBusReply<bool>(panel.call(QStringLiteral("IsOpen"))).value(), false);
    }

    void portalLauncherStartsTheServiceAndItOpensOnThePage()
    {
        useRealMainWithPageRecordingOverlay();
        QVERIFY(writeFile(m_session->dir(QStringLiteral("services")) + QStringLiteral("/org.devl0rd.KontrolPanel.service"),
            QStringLiteral("[D-BUS Service]\nName=%1\nExec=%2 %3\n")
                .arg(busName, QStringLiteral(KONVEYOR_KONTROL_PANEL), m_directory)
                .toUtf8()));
        QVERIFY(useTheSessionEnvironmentForActivation());
        QDBusServiceWatcher watcher(busName, QDBusConnection::sessionBus(), QDBusServiceWatcher::WatchForUnregistration);
        QSignalSpy gone(&watcher, &QDBusServiceWatcher::serviceUnregistered);

        const ProgramResult result = m_session->run(QStringLiteral("python3"),
            {QStringLiteral(KONVEYOR_SOURCE_DIR "/widgets/portals/bin/portal-launcher"), QStringLiteral("games")});
        QVERIFY2(result.exitCode == 0, qPrintable(result.err));
        const uint pid = QDBusConnection::sessionBus().interface()->servicePid(busName);
        QVERIFY(pid > 0);
        QDBusInterface panel(busName, QStringLiteral("/KontrolPanel"), busName, QDBusConnection::sessionBus());
        QCOMPARE(QDBusReply<bool>(panel.call(QStringLiteral("IsOpen"))).value(), true);
        const KConfig config(configPath(), KConfig::SimpleConfig);
        QCOMPARE(config.group(QStringLiteral("General")).readEntry("appsCategory"), QStringLiteral("games"));
        QCOMPARE(::kill(static_cast<pid_t>(pid), SIGTERM), 0);
        QVERIFY(gone.wait(15000));
    }

private:
    QString configPath() const { return m_session->dir(QStringLiteral("config")) + QStringLiteral("/konveyor/kontrolpanelrc"); }

    void useRealMainWithPageRecordingOverlay()
    {
        QVERIFY(QFile::remove(m_directory + QStringLiteral("/Main.qml")));
        QVERIFY(QFile::copy(
            QStringLiteral(KONVEYOR_SOURCE_DIR "/widgets/portals/kontrol-panel/Main.qml"), m_directory + QStringLiteral("/Main.qml")));
        QVERIFY(writeFile(m_directory + QStringLiteral("/Overlay.qml"), pageRecordingOverlay));
        QVERIFY(writeFile(m_directory + QStringLiteral("/ConfigWindow.qml"), "import QtQuick\nimport QtQuick.Window\nWindow {}\n"));
    }

    bool useTheSessionEnvironmentForActivation() const
    {
        qDBusRegisterMetaType<QMap<QString, QString>>();
        const QProcessEnvironment environment = m_session->environment();
        QMap<QString, QString> variables;
        const QStringList keys = environment.keys();
        for (const QString &key : keys) {
            variables.insert(key, environment.value(key));
        }
        QDBusMessage update = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.DBus"),
            QStringLiteral("/org/freedesktop/DBus"), QStringLiteral("org.freedesktop.DBus"), QStringLiteral("UpdateActivationEnvironment"));
        update << QVariant::fromValue(variables);
        return QDBusConnection::sessionBus().call(update).type() == QDBusMessage::ReplyMessage;
    }

    void start()
    {
        m_process = std::make_unique<QProcess>();
        m_process->setProcessEnvironment(m_session->environment());
        QDBusServiceWatcher watcher(busName, QDBusConnection::sessionBus(), QDBusServiceWatcher::WatchForRegistration);
        QSignalSpy registered(&watcher, &QDBusServiceWatcher::serviceRegistered);
        m_process->start(QStringLiteral(KONVEYOR_KONTROL_PANEL), {m_directory});
        QVERIFY(m_process->waitForStarted());
        QVERIFY2(registered.wait(15000), m_process->readAllStandardError().constData());
    }

    std::unique_ptr<PrivateSession> m_session;
    std::unique_ptr<QProcess> m_process;
    QString m_directory;
};

QTEST_GUILESS_MAIN(TestKontrolPanelMain)
#include "test_kontrolpanel_main.moc"
