#include "kontrolpanelservice.h"

#include <QDBusConnection>
#include <QSignalSpy>
#include <QTest>

using Konveyor::KontrolPanelService;

class TestKontrolPanelService : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void dbusMethodsAskTheInterface()
    {
        KontrolPanelService service;
        QSignalSpy toggled(&service, &KontrolPanelService::toggleRequested);
        QSignalSpy opened(&service, &KontrolPanelService::openRequested);
        QSignalSpy hidden(&service, &KontrolPanelService::hideRequested);
        QSignalSpy pinned(&service, &KontrolPanelService::pinRequested);
        QSignalSpy configured(&service, &KontrolPanelService::configureRequested);

        service.Toggle();
        service.Open(QStringLiteral("games"));
        service.Hide();
        service.Pin({QStringLiteral("/usr/share/applications/org.kde.konsole.desktop")});
        service.Configure();

        QCOMPARE(toggled.count(), 1);
        QCOMPARE(opened.count(), 1);
        QCOMPARE(opened.first().first().toString(), QStringLiteral("games"));
        QCOMPARE(hidden.count(), 1);
        QCOMPARE(pinned.first().first().toStringList(), QStringList {QStringLiteral("/usr/share/applications/org.kde.konsole.desktop")});
        QCOMPARE(configured.count(), 1);
    }

    void reportsWhetherItIsOpen()
    {
        KontrolPanelService service;
        QSignalSpy changed(&service, &KontrolPanelService::openChanged);
        QVERIFY(!service.IsOpen());
        service.setOpen(true);
        service.setOpen(true);
        QVERIFY(service.IsOpen());
        QVERIFY(service.isOpen());
        QCOMPARE(changed.count(), 1);
        service.setOpen(false);
        QVERIFY(!service.IsOpen());
        QCOMPARE(changed.count(), 2);
    }

    void registrationFailsWithoutABus()
    {
        KontrolPanelService service;
        QVERIFY(!service.registerOn(QDBusConnection(QStringLiteral("konveyor-kontrol-panel-test-no-bus"))));
    }

    void konveyorIsNotRunningWithoutABus()
    {
        const KontrolPanelService service;
        QVERIFY(!service.konveyorRunning());
    }

    void backgroundEffectsIgnoreAMissingWindow()
    {
        KontrolPanelService service;
        service.applyBackgroundEffects(nullptr, QRegion(0, 0, 10, 10));
    }
};

QTEST_MAIN(TestKontrolPanelService)
#include "test_kontrolpanel_service.moc"
