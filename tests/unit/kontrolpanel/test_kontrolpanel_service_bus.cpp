#include "../dbus/privatebus.h"
#include "kontrolpanelservice.h"

#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusReply>
#include <QSignalSpy>
#include <QTest>

#include <memory>

using Konveyor::KontrolPanelService;
using Konveyor::Test::FakeService;
using Konveyor::Test::PrivateSession;

namespace
{

const QString konveyorName = QStringLiteral("org.kde.Konveyor");
const QString panelName = QString::fromLatin1(KontrolPanelService::serviceName);

FakeService::Handler acknowledge()
{
    return [](const QDBusMessage &message) { return message.createReply(); };
}

}

class TestKontrolPanelServiceBus : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        m_session = std::make_unique<PrivateSession>();
        QVERIFY(m_session->start());
        m_connection = QDBusConnection::connectToBus(m_session->address(), QStringLiteral("konveyor-kontrol-panel-test-bus"));
        QVERIFY(m_connection.isConnected());
    }

    void cleanupTestCase()
    {
        m_connection = QDBusConnection(QString());
        QDBusConnection::disconnectFromBus(QStringLiteral("konveyor-kontrol-panel-test-bus"));
        m_session.reset();
    }

    void registersItsObjectAndName()
    {
        KontrolPanelService service;
        QVERIFY(service.registerOn(m_connection));
        QVERIFY(m_connection.interface()->isServiceRegistered(panelName));
        QDBusInterface panel(panelName, QLatin1String(KontrolPanelService::objectPath), panelName, m_connection);
        service.setOpen(true);
        QCOMPARE(QDBusReply<bool>(panel.call(QStringLiteral("IsOpen"))).value(), true);
        m_connection.unregisterObject(QLatin1String(KontrolPanelService::objectPath));
        QVERIFY(m_connection.unregisterService(panelName));
    }

    void failsWhenAnotherProcessOwnsTheName()
    {
        FakeService other(panelName, QStringLiteral("/Other"), acknowledge());
        QVERIFY(other.start(m_session->address()));
        KontrolPanelService service;
        QVERIFY(!service.registerOn(m_connection));
        m_connection.unregisterObject(QLatin1String(KontrolPanelService::objectPath));
    }

    void followsKonveyorOnTheSessionBus()
    {
        const KontrolPanelService service;
        QVERIFY(!service.konveyorRunning());
        QSignalSpy started(&service, &KontrolPanelService::konveyorStarted);
        FakeService konveyor(konveyorName, QStringLiteral("/Konveyor"), acknowledge());
        QVERIFY(konveyor.start(m_session->address()));
        QTRY_COMPARE(started.count(), 1);
        QVERIFY(service.konveyorRunning());
    }

private:
    std::unique_ptr<PrivateSession> m_session;
    QDBusConnection m_connection {QString()};
};

QTEST_MAIN(TestKontrolPanelServiceBus)
#include "test_kontrolpanel_service_bus.moc"
