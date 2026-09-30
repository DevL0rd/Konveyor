#include "launcherharness.h"

namespace
{

const QString konsole = QStringLiteral("org.kde.konsole.desktop");

QString entriesFor(const QString &favoriteId, const QString &actions = QStringLiteral("[]"))
{
    return QStringLiteral("launcher.kickerEntries(launcherData.favorites, 0, %1, '%2')").arg(actions, favoriteId);
}

QString texts(const QString &entries)
{
    return entries + QStringLiteral(".map(e => e.separator ? '-' : e.text)");
}

QString run(const QString &entries, const QString &text)
{
    return entries + QStringLiteral(".find(e => e.text === '%1').run()").arg(text);
}

}

class TestLauncherMenusQml : public LauncherTest::TestCase
{
    Q_OBJECT

private Q_SLOTS:
    void desktopAppsOfferPanelAndDesktopOnlyStandalone_data()
    {
        QTest::addColumn<bool>("portal");
        QTest::addColumn<QStringList>("expected");
        const QStringList common {QStringLiteral("Open"), QStringLiteral("Pin to sidebar"), QStringLiteral("Unpin from Home"),
            QStringLiteral("Hide from launcher")};
        QTest::newRow("standalone") << false
                                    << common + QStringList {QStringLiteral("Add to Panel (Widget)"), QStringLiteral("Add to Desktop")};
        QTest::newRow("App Portal") << true << common;
    }

    void desktopAppsOfferPanelAndDesktopOnlyStandalone()
    {
        QFETCH(bool, portal);
        QFETCH(QStringList, expected);
        QVERIFY(m_harness.host(portal));
        QCOMPARE(eval(texts(entriesFor(konsole))).toStringList(), expected);
    }

    void unpinnedAppsOfferPinning()
    {
        QVERIFY(m_harness.host(false));
        const QStringList entries = eval(texts(entriesFor(QStringLiteral("firefox.desktop")))).toStringList();
        QVERIFY(entries.contains(QStringLiteral("Pin to Home")));
        QVERIFY(!entries.contains(QStringLiteral("Unpin from Home")));
    }

    void filesAreNeitherHiddenNorAddedToPanels()
    {
        QVERIFY(m_harness.host(false));
        QCOMPARE(eval(texts(entriesFor(QStringLiteral("file:///home/user/notes.txt")))).toStringList(),
            QStringList({QStringLiteral("Open"), QStringLiteral("Pin to sidebar"), QStringLiteral("Pin to Home")}));
    }

    void entriesWithoutAnIdOnlyOpen()
    {
        QVERIFY(m_harness.host(false));
        QCOMPARE(eval(texts(entriesFor(QString()))).toStringList(), QStringList {QStringLiteral("Open")});
    }

    void actionListSeparatorsCollapse()
    {
        QVERIFY(m_harness.host(true));
        const QString actions
            = QStringLiteral("[{ text: 'New Window', actionId: 'new', icon: 'window-new' }, { type: 'separator' }, { type: 'separator' },"
                             " null, { text: 'Private', actionId: 'private', actionArgument: 7 }, { type: 'separator' }]");
        const QStringList entries = eval(texts(entriesFor(QString(), actions))).toStringList();
        QCOMPARE(entries,
            QStringList({QStringLiteral("Open"), QStringLiteral("-"), QStringLiteral("New Window"), QStringLiteral("-"),
                QStringLiteral("Private")}));
        QObject *host = m_harness.view()->parent();
        eval(run(entriesFor(QString(), actions), QStringLiteral("Private")));
        QCOMPARE(host->property("hideCount").toInt(), 1);
        const QVariantList triggered = eval(QStringLiteral("launcherData.favorites.triggered")).toList();
        QCOMPARE(triggered.size(), 1);
        QCOMPARE(triggered.first().toMap().value(QStringLiteral("actionId")).toString(), QStringLiteral("private"));
        QCOMPARE(triggered.first().toMap().value(QStringLiteral("argument")).toInt(), 7);
    }

    void addingToThePanelAsksThePortalLauncher_data()
    {
        QTest::addColumn<QString>("text");
        QTest::addColumn<QString>("place");
        QTest::newRow("panel") << QStringLiteral("Add to Panel (Widget)") << QStringLiteral("panel");
        QTest::newRow("desktop") << QStringLiteral("Add to Desktop") << QStringLiteral("desktop");
    }

    void addingToThePanelAsksThePortalLauncher()
    {
        QFETCH(QString, text);
        QFETCH(QString, place);
        QObject *host = m_harness.openHost(false);
        QVERIFY(host);
        eval(run(entriesFor(konsole), text));
        QCOMPARE(host->property("hideCount").toInt(), 1);
        QVERIFY(m_harness.commands().contains(
            QStringLiteral("$HOME/.local/bin/portal-launcher add-to %1 'org.kde.konsole.desktop'").arg(place)));
    }

    void hidingAnAppTellsPortalGames()
    {
        QVERIFY(m_harness.host(false));
        eval(run(entriesFor(konsole), QStringLiteral("Hide from launcher")));
        QVERIFY(eval(QStringLiteral("launcherData.isHidden('%1')").arg(konsole)).toBool());
        QVERIFY(m_harness.commands().contains(QStringLiteral("$HOME/.local/bin/portal-games --hide 'org.kde.konsole'")));
        eval(QStringLiteral("launcherData.setHidden('%1', false)").arg(konsole));
        QVERIFY(!eval(QStringLiteral("launcherData.isHidden('%1')").arg(konsole)).toBool());
        QVERIFY(m_harness.commands().contains(QStringLiteral("$HOME/.local/bin/portal-games --unhide 'org.kde.konsole'")));
    }

    void pinningTogglesTheFavorite()
    {
        QVERIFY(m_harness.host(false));
        eval(run(entriesFor(konsole), QStringLiteral("Unpin from Home")));
        QVERIFY(!eval(QStringLiteral("launcherData.favorites.isFavorite('%1')").arg(konsole)).toBool());
        eval(run(entriesFor(konsole), QStringLiteral("Pin to Home")));
        QVERIFY(eval(QStringLiteral("launcherData.favorites.isFavorite('%1')").arg(konsole)).toBool());
    }

    void pinnedAppsMoveBetweenFolders()
    {
        QVERIFY(m_harness.host(false));
        eval(QStringLiteral("launcherData.folders = [{ id: 'work', name: 'Work', apps: ['%1'] }].concat("
                            "[1, 2, 3, 4, 5, 6, 7].map(n => ({ id: 'f' + n, name: 'Folder ' + n, apps: ['x' + n] })))")
                .arg(konsole));
        const QStringList entries = eval(texts(entriesFor(konsole))).toStringList();
        QVERIFY(entries.contains(QStringLiteral("Remove from “Work”")));
        QVERIFY(!entries.contains(QStringLiteral("Move to “Work”")));
        QCOMPARE(entries.filter(QStringLiteral("Move to")).size(), 6);
        eval(run(entriesFor(konsole), QStringLiteral("Move to “Folder 2”")));
        QCOMPARE(eval(QStringLiteral("launcherData.folderFor('%1').id").arg(konsole)).toString(), QStringLiteral("f2"));
        QVERIFY(eval(QStringLiteral("launcherData.folderById('work') === null")).toBool());
        QVERIFY(m_harness.commands().contains(QStringLiteral("$HOME/.local/bin/portal-games --folder-add 'f2' '%1'").arg(konsole)));
    }

    void sidebarEntriesToggle()
    {
        QVERIFY(m_harness.host(false));
        eval(run(entriesFor(konsole), QStringLiteral("Pin to sidebar")));
        QCOMPARE(eval(QStringLiteral("launcherData.sidebarPins.map(p => p.kind + ':' + p.id)")).toStringList(),
            QStringList {QStringLiteral("app:org.kde.konsole")});
        QVERIFY(m_harness.commands().last().contains(QStringLiteral("--sidebar-set")));
        QVERIFY(eval(texts(entriesFor(konsole))).toStringList().contains(QStringLiteral("Unpin from sidebar")));
    }

    void folderMenuRenamesAndUngroups()
    {
        QVERIFY(m_harness.host(false));
        eval(QStringLiteral("launcherData.folders = [{ id: 'work', name: 'Work', apps: ['%1'] }]").arg(konsole));
        const QString folder = QStringLiteral("launcher.folderEntries({ id: 'work', name: 'Work' })");
        QCOMPARE(eval(texts(folder)).toStringList(),
            QStringList({QStringLiteral("Open folder"), QStringLiteral("Rename…"), QStringLiteral("-"), QStringLiteral("Ungroup")}));
        eval(run(folder, QStringLiteral("Open folder")));
        QCOMPARE(eval(QStringLiteral("launcher.openFolder")).toString(), QStringLiteral("work"));
        QCOMPARE(eval(texts(folder)).toStringList().first(), QStringLiteral("Close folder"));
        eval(run(folder, QStringLiteral("Ungroup")));
        QCOMPARE(eval(QStringLiteral("launcher.openFolder")).toString(), QString());
        QCOMPARE(eval(QStringLiteral("launcherData.folders.length")).toInt(), 0);
        QVERIFY(m_harness.commands().contains(QStringLiteral("$HOME/.local/bin/portal-games --folder-delete 'work'")));
    }

    void powerMenuListsTheSessionActions()
    {
        QObject *host = m_harness.host(false);
        QVERIFY(host);
        const QString entries = QStringLiteral("launcher.powerEntries()");
        QCOMPARE(eval(texts(entries)).toStringList(),
            QStringList({QStringLiteral("Lock"), QStringLiteral("Log Out"), QStringLiteral("Sleep"), QStringLiteral("Restart"),
                QStringLiteral("Shut Down"), QStringLiteral("-"), QStringLiteral("Session page")}));
        QCOMPARE(eval(entries + QStringLiteral(".map(e => e.icon || '')")).toStringList().mid(0, 5),
            QStringList({QStringLiteral("system-lock-screen-symbolic"), QStringLiteral("system-log-out-symbolic"),
                QStringLiteral("system-suspend-symbolic"), QStringLiteral("system-reboot-symbolic"),
                QStringLiteral("system-shutdown-symbolic")}));
        eval(run(entries, QStringLiteral("Restart")));
        QCOMPARE(eval(QStringLiteral("launcherData.system.triggered[0].favoriteId")).toString(), QStringLiteral("reboot"));
        QCOMPARE(host->property("hideCount").toInt(), 1);
    }

    void packageMenuInstallsWithShelly()
    {
        QObject *host = m_harness.host(false);
        QVERIFY(host);
        const QString entries = QStringLiteral("launcher.packageEntries({ name: \"it's\", source: 'aur', page: 'https://aur', url: '' })");
        QCOMPARE(eval(entries + QStringLiteral("[2].text")).toString(), QStringLiteral("Open AUR page"));
        QVERIFY(eval(entries + QStringLiteral("[3].disabled")).toBool());
        eval(entries + QStringLiteral("[0].run()"));
        QVERIFY(m_harness.commands().contains(QStringLiteral("konsole --hold -e shelly install aur 'it'\\''s'")));
        QCOMPARE(host->property("hideCount").toInt(), 1);
    }

    void sidebarPinMenus_data()
    {
        QTest::addColumn<QString>("pins");
        QTest::addColumn<int>("index");
        QTest::addColumn<QStringList>("expected");
        const QString pins = QStringLiteral(
            "[{ kind: 'app', id: 'firefox', name: 'Firefox' }, { kind: 'path', id: 'file:///home/user/notes.txt', name: 'notes' },"
            " { kind: 'path', id: 'file:///gone', name: 'gone', missing: true }]");
        QTest::newRow("app") << pins << 0
                             << QStringList({QStringLiteral("Open"), QStringLiteral("Unpin from sidebar"), QStringLiteral("-"),
                                    QStringLiteral("Move up"), QStringLiteral("Move down")});
        QTest::newRow("file") << pins << 1
                              << QStringList({QStringLiteral("Open"), QStringLiteral("Open containing folder"),
                                     QStringLiteral("Unpin from sidebar"), QStringLiteral("-"), QStringLiteral("Move up"),
                                     QStringLiteral("Move down")});
        QTest::newRow("missing") << pins << 2
                                 << QStringList({QStringLiteral("“gone” no longer exists"), QStringLiteral("Remove from sidebar"),
                                        QStringLiteral("-"), QStringLiteral("Move up"), QStringLiteral("Move down")});
    }

    void sidebarPinMenus()
    {
        QFETCH(QString, pins);
        QFETCH(int, index);
        QFETCH(QStringList, expected);
        QVERIFY(m_harness.host(false));
        eval(QStringLiteral("launcherData.sidebarPins = %1").arg(pins));
        const QString entries = QStringLiteral("launcher.sidebarEntries(launcherData.sidebarPins[%1], %1)").arg(index);
        QCOMPARE(eval(texts(entries)).toStringList(), expected);
        QCOMPARE(eval(entries + QStringLiteral(".find(e => e.text === 'Move up').disabled")).toBool(), index == 0);
        QCOMPARE(eval(entries + QStringLiteral(".find(e => e.text === 'Move down').disabled")).toBool(), index == 2);
    }

    void sidebarPinsOpenAndShowTheirFolder()
    {
        QObject *host = m_harness.host(false);
        QVERIFY(host);
        eval(QStringLiteral("launcherData.sidebarPins = [{ kind: 'path', id: 'file:///home/user/notes.txt', name: 'notes' }]"));
        eval(run(QStringLiteral("launcher.sidebarEntries(launcherData.sidebarPins[0], 0)"), QStringLiteral("Open containing folder")));
        QCOMPARE(host->property("hideCount").toInt(), 1);
        QVERIFY(m_harness.commands().last().contains(
            QStringLiteral("org.freedesktop.FileManager1.ShowItems array:string:'file:///home/user/notes.txt'")));
        eval(QStringLiteral("launcher.openPin(launcherData.sidebarPins[0])"));
        QCOMPARE(LauncherTest::urls().opened, QList<QUrl> {QUrl(QStringLiteral("file:///home/user/notes.txt"))});
        eval(QStringLiteral("launcher.openPin({ kind: 'path', id: 'file:///gone', missing: true })"));
        QCOMPARE(host->property("hideCount").toInt(), 2);
    }
};

LAUNCHER_TEST_MAIN(TestLauncherMenusQml)
#include "test_launcher_menus_qml.moc"
