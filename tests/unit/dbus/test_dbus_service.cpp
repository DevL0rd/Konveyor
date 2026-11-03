#include "../dbus/privatebus.h"
#include "dbus/dbusservice.h"
#include "ipc/dbusnames.h"

#include <QDBusConnectionInterface>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QEventLoop>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTest>
#include <QTimer>

#include <memory>

using Konveyor::DBusHandlers;
using Konveyor::DBusService;
using Konveyor::Test::FakeService;
using Konveyor::Test::PrivateSession;

namespace
{

const QString serviceName = QString::fromLatin1(Konveyor::Ipc::dbusService);
const QString objectPath = QString::fromLatin1(Konveyor::Ipc::dbusPath);

QJsonDocument marker(const QString &name)
{
    return QJsonDocument(QJsonObject {{QStringLiteral("name"), name}});
}

DBusHandlers handlers()
{
    DBusHandlers result;
    result.windows = [] { return marker(QStringLiteral("windows")); };
    result.workspaces = [] { return marker(QStringLiteral("workspaces")); };
    result.outputs = [] { return marker(QStringLiteral("outputs")); };
    result.focusedWindow = [] { return QJsonDocument(); };
    result.focusedOutput = [] { return marker(QStringLiteral("focusedOutput")); };
    result.binds = [] { return marker(QStringLiteral("binds")); };
    result.action = [](const QString &json) { return QStringLiteral("action:") + json; };
    result.loadConfigFile = [](const QString &path) { return QStringLiteral("load:") + path; };
    result.overviewOpen = [] { return true; };
    result.lastBind = [] { return marker(QStringLiteral("lastBind")); };
    result.gestures = [] { return marker(QStringLiteral("gestures")); };
    result.appRules = [](const QString &json) { return marker(QStringLiteral("rules:") + json); };
    result.setAppRule = [](const QString &json) { return QStringLiteral("set:") + json; };
    result.signalFrames = [] { return marker(QStringLiteral("frames")); };
    return result;
}

QString callText(const QDBusConnection &client, const QString &method, const QVariantList &arguments = {})
{
    QDBusMessage message = QDBusMessage::createMethodCall(serviceName, objectPath, serviceName, method);
    message.setArguments(arguments);
    QDBusPendingCallWatcher watcher(client.asyncCall(message));
    QEventLoop loop;
    QObject::connect(&watcher, &QDBusPendingCallWatcher::finished, &loop, &QEventLoop::quit);
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);
    if (!watcher.isFinished()) {
        loop.exec();
    }
    return QDBusPendingReply<QString>(watcher).value();
}

FakeService::Handler acknowledge()
{
    return [](const QDBusMessage &message) { return message.createReply(); };
}

}

class TestDBusService : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        m_session = std::make_unique<PrivateSession>();
        QVERIFY(m_session->start());
        m_client = QDBusConnection::connectToBus(m_session->address(), QStringLiteral("konveyor-dbus-service-client"));
        QVERIFY(m_client.isConnected());
    }

    void cleanupTestCase()
    {
        m_client = QDBusConnection(QString());
        QDBusConnection::disconnectFromBus(QStringLiteral("konveyor-dbus-service-client"));
        m_session.reset();
    }

    void leavesTheBusAloneWhenItNeverRegistered()
    {
        {
            const DBusService unregistered(handlers());
        }
        QVERIFY(!m_client.interface()->isServiceRegistered(serviceName));
    }

    void answersEveryCallFromItsHandlers()
    {
        DBusService service(handlers());
        QVERIFY(service.registerService());
        const auto text = [&](const QString &method, const QVariantList &arguments = {}) { return callText(m_client, method, arguments); };
        QCOMPARE(text(QStringLiteral("Version")), QStringLiteral("{\"version\":\"%1\"}").arg(QStringLiteral(KONVEYOR_VERSION)));
        QCOMPARE(text(QStringLiteral("Windows")), QStringLiteral("{\"name\":\"windows\"}"));
        QCOMPARE(text(QStringLiteral("Workspaces")), QStringLiteral("{\"name\":\"workspaces\"}"));
        QCOMPARE(text(QStringLiteral("Outputs")), QStringLiteral("{\"name\":\"outputs\"}"));
        QCOMPARE(text(QStringLiteral("FocusedWindow")), QStringLiteral("null"));
        QCOMPARE(text(QStringLiteral("FocusedOutput")), QStringLiteral("{\"name\":\"focusedOutput\"}"));
        QCOMPARE(text(QStringLiteral("Binds")), QStringLiteral("{\"name\":\"binds\"}"));
        QCOMPARE(text(QStringLiteral("OverviewState")), QStringLiteral("{\"is_open\":true}"));
        QCOMPARE(text(QStringLiteral("Action"), {QStringLiteral("a")}), QStringLiteral("action:a"));
        QCOMPARE(text(QStringLiteral("LoadConfigFile"), {QStringLiteral("/p")}), QStringLiteral("load:/p"));
        QCOMPARE(text(QStringLiteral("LastBind")), QStringLiteral("{\"name\":\"lastBind\"}"));
        QCOMPARE(text(QStringLiteral("SignalFrames")), QStringLiteral("{\"name\":\"frames\"}"));
        QCOMPARE(text(QStringLiteral("Gestures")), QStringLiteral("{\"name\":\"gestures\"}"));
        QCOMPARE(text(QStringLiteral("AppRules"), {QStringLiteral("q")}), QStringLiteral("{\"name\":\"rules:q\"}"));
        QCOMPARE(text(QStringLiteral("SetAppRule"), {QStringLiteral("r")}), QStringLiteral("set:r"));
    }

    void reportsMultiTouchAndModifiersAndEmitsOnlyOnChange()
    {
        DBusService service(handlers());
        QSignalSpy multiTouch(&service, &DBusService::MultiTouchChanged);
        QSignalSpy modifiers(&service, &DBusService::ModifiersHeldChanged);
        QSignalSpy layout(&service, &DBusService::LayoutChanged);
        QSignalSpy taskbar(&service, &DBusService::TaskbarItemRequested);

        QVERIFY(!service.MultiTouchActive());
        service.setMultiTouchActive(false);
        QCOMPARE(multiTouch.count(), 0);
        service.setMultiTouchActive(true);
        service.setMultiTouchActive(true);
        QCOMPARE(multiTouch.count(), 1);
        QVERIFY(service.MultiTouchActive());

        QCOMPARE(service.ModifiersHeld(), QStringLiteral("{\"alt\":false,\"super\":false}"));
        service.setModifiersHeld(false, false);
        QCOMPARE(modifiers.count(), 0);
        service.setModifiersHeld(true, false);
        service.setModifiersHeld(true, false);
        service.setModifiersHeld(true, true);
        QCOMPARE(modifiers.count(), 2);
        QCOMPARE(service.ModifiersHeld(), QStringLiteral("{\"alt\":true,\"super\":true}"));

        service.announceLayoutChange();
        QCOMPARE(layout.count(), 1);
        service.requestTaskbarItem(4);
        QCOMPARE(taskbar.takeFirst().first().toInt(), 4);
    }

    void unregistersItselfWhenDestroyed()
    {
        {
            DBusService service(handlers());
            QVERIFY(service.registerService());
            QVERIFY(m_client.interface()->isServiceRegistered(serviceName));
        }
        QTRY_VERIFY(!m_client.interface()->isServiceRegistered(serviceName));
    }

    void waitsForTheCurrentOwnerAndTakesTheNameWhenItLeaves()
    {
        auto owner = std::make_unique<FakeService>(serviceName, QStringLiteral("/Other"), acknowledge());
        QVERIFY(owner->start(m_session->address()));
        DBusService service(handlers());
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("could not register .*org.kde.Konveyor.* yet")));
        QVERIFY(!service.registerService());
        owner.reset();
        QTRY_VERIFY(m_client.interface()->isServiceRegistered(serviceName));
        QCOMPARE(callText(m_client, QStringLiteral("Windows")), QStringLiteral("{\"name\":\"windows\"}"));
    }

    void keepsWaitingWhenAnotherWaiterTookTheNameFirst()
    {
        auto owner = std::make_unique<FakeService>(serviceName, QStringLiteral("/Other"), acknowledge());
        QVERIFY(owner->start(m_session->address()));
        DBusService first(handlers());
        DBusService second(handlers());
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("could not register .*org.kde.Konveyor.* yet")));
        QVERIFY(!first.registerService());
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("could not register .*org.kde.Konveyor.* yet")));
        QVERIFY(!second.registerService());
        owner.reset();
        QTRY_VERIFY(m_client.interface()->isServiceRegistered(serviceName));
        QCOMPARE(callText(m_client, QStringLiteral("Outputs")), QStringLiteral("{\"name\":\"outputs\"}"));
    }

private:
    std::unique_ptr<PrivateSession> m_session;
    QDBusConnection m_client {QString()};
};

QTEST_MAIN(TestDBusService)
#include "test_dbus_service.moc"
