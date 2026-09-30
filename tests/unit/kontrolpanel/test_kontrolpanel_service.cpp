#include "kontrolpanelservice.h"

#include <QDBusConnection>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QRegularExpression>
#include <QTemporaryDir>

#include <QSignalSpy>
#include <QTest>
#include <memory>

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

    void formsOnlyGetTheValuesTheyDeclare()
    {
        QTemporaryDir directory;
        QFile form(directory.filePath(QStringLiteral("form.qml")));
        QVERIFY(form.open(QIODevice::WriteOnly));
        form.write("import QtQuick\nItem { property int cfg_size: 1; property string seen: \"\"; Component.onCompleted: seen = \"size \" + "
                   "cfg_size }\n");
        form.close();
        QQmlEngine engine;
        QQmlComponent parentComponent(&engine);
        parentComponent.setData("import QtQuick\nItem {}\n", QUrl());
        std::unique_ptr<QObject> parent(parentComponent.create());
        const KontrolPanelService service;
        QQuickItem *item = service.createForm(QUrl::fromLocalFile(form.fileName()),
            {{QStringLiteral("cfg_size"), 5}, {QStringLiteral("cfg_unknown"), true}}, qobject_cast<QQuickItem *>(parent.get()));
        QVERIFY(item);
        QCOMPARE(item->property("seen").toString(), QStringLiteral("size 5"));
        QCOMPARE(item->parentItem(), parent.get());
        QCOMPARE(item->parent(), parent.get());

        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("could not create .*missing.qml")));
        QVERIFY(!service.createForm(
            QUrl::fromLocalFile(directory.filePath(QStringLiteral("missing.qml"))), {}, qobject_cast<QQuickItem *>(parent.get())));
    }

    void backgroundEffectsIgnoreAMissingWindow()
    {
        KontrolPanelService service;
        service.applyBackgroundEffects(nullptr, QRegion(0, 0, 10, 10));
    }
};

QTEST_MAIN(TestKontrolPanelService)
#include "test_kontrolpanel_service.moc"
