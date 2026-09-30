#include "launcherharness.h"

using LauncherTest::response;

class TestLauncherDataQml : public LauncherTest::TestCase
{
    Q_OBJECT

private:
    int count(const QString &prefix)
    {
        const QStringList all = m_harness.commands();
        return int(std::count_if(all.cbegin(), all.cend(), [&prefix](const QString &command) { return command.startsWith(prefix); }));
    }

    QString statePath(const char *name) const { return m_harness.path(QStringLiteral("data/Plasma-App-Portal/") + QLatin1String(name)); }

private Q_SLOTS:
    void goingLiveLoadsTheLibrary()
    {
        m_harness.respondWithLibrary();
        QVERIFY(m_harness.openHost(false));
        QCOMPARE(count(QStringLiteral("$HOME/.local/bin/portal-games # ")), 1);
        QCOMPARE(count(QStringLiteral("$HOME/.local/bin/portal-games --sidebar # ")), 1);
        TRY_COMPARE(eval(QStringLiteral("launcherData.games.length")).toInt(), 3);
        QCOMPARE(eval(QStringLiteral("launcherData.recentGames.map(g => g.id)")).toStringList(),
            QStringList({QStringLiteral("steam_app_620"), QStringLiteral("steam_app_70")}));
        QCOMPARE(eval(QStringLiteral("launcherData.gameForApp('applications:steam_app_620.desktop').name")).toString(),
            QStringLiteral("Portal 2"));
        QVERIFY(eval(QStringLiteral("launcherData.steamGameForApp('celeste.desktop') === null")).toBool());
        QVERIFY(eval(QStringLiteral("launcherData.gameForApp('') === null")).toBool());
    }

    void theLibraryIsReloadedAtMostEveryHalfMinute()
    {
        m_harness.respondWithLibrary();
        QObject *host = m_harness.openHost(false);
        QVERIFY(host);
        TRY_COMPARE(eval(QStringLiteral("launcherData.games.length")).toInt(), 3);
        host->setProperty("open", false);
        TRY_VERIFY(!eval(QStringLiteral("launcher.shown")).toBool());
        eval(QStringLiteral("launcherData.gamesLoadedAt = Date.now() - 29000"));
        host->setProperty("open", true);
        QCOMPARE(count(QStringLiteral("$HOME/.local/bin/portal-games # ")), 1);
        eval(QStringLiteral("launcherData.refreshGames(true)"));
        QCOMPARE(count(QStringLiteral("$HOME/.local/bin/portal-games # ")), 2);
        eval(QStringLiteral("launcherData.gamesLoadedAt = Date.now() - 31000"));
        eval(QStringLiteral("launcherData.refreshGames()"));
        QCOMPARE(count(QStringLiteral("$HOME/.local/bin/portal-games # ")), 3);
    }

    void gamesOffSkipsTheLibrary()
    {
        m_harness.respondWithLibrary();
        QVERIFY(m_harness.openHost(false, {{QStringLiteral("showGames"), false}}));
        eval(QStringLiteral("launcherData.refreshGames(true)"));
        QCOMPARE(count(QStringLiteral("$HOME/.local/bin/portal-games # ")), 0);
    }

    void aBrokenLibraryKeepsTheLastGames_data()
    {
        QTest::addColumn<QByteArray>("output");
        QTest::addColumn<int>("exitCode");
        QTest::newRow("output that is not json") << QByteArray("not json") << 0;
        QTest::newRow("a crash with no output") << QByteArray() << 1;
        QTest::newRow("a missing portal-games") << QByteArray() << 127;
    }

    void aBrokenLibraryKeepsTheLastGames()
    {
        QFETCH(QByteArray, output);
        QFETCH(int, exitCode);
        m_harness.respondWithLibrary();
        QVERIFY(m_harness.openHost(false));
        TRY_COMPARE(eval(QStringLiteral("launcherData.games.length")).toInt(), 3);
        m_harness.respond({response(QStringLiteral("portal-games # "), output, exitCode, QStringLiteral("Traceback"))});
        eval(QStringLiteral("launcherData.refreshGames(true)"));
        QVERIFY(m_harness.waitHandled(QStringLiteral("$HOME/.local/bin/portal-games # "), 2));
        QCOMPARE(eval(QStringLiteral("launcherData.games.length")).toInt(), 3);
    }

    void launchingAGameTracksItAndMovesItFirst()
    {
        m_harness.respondWithLibrary();
        QVERIFY(m_harness.openHost(false));
        TRY_COMPARE(eval(QStringLiteral("launcherData.games.length")).toInt(), 3);
        eval(QStringLiteral("launcher.launchGame(launcherData.games.find(g => g.id === 'celeste'))"));
        QVERIFY(m_harness.commands().last().startsWith(
            QStringLiteral("celeste </dev/null >/dev/null 2>&1 & $HOME/.local/bin/portal-games --track 'celeste' # ")));
        QCOMPARE(eval(QStringLiteral("launcherData.recentGames[0].id")).toString(), QStringLiteral("celeste"));
        const int before = m_harness.commands().size();
        eval(QStringLiteral("launcher.launchGame({ id: 'x', launch: '' })"));
        eval(QStringLiteral("launcher.launchGame(null)"));
        QCOMPARE(m_harness.commands().size(), before);
    }

    void friendsAreSortedAndGroupedByGame()
    {
        m_harness.respondWithLibrary();
        QVERIFY(m_harness.writeFile(m_harness.friendsPath(), LauncherTest::fixture("friends.json")));
        QVERIFY(m_harness.openHost(false));
        TRY_COMPARE(eval(QStringLiteral("launcherData.friends.length")).toInt(), 5);
        QCOMPARE(eval(QStringLiteral("launcherData.friends.map(f => f.name)")).toStringList(),
            QStringList(
                {QStringLiteral("Alice"), QStringLiteral("dave"), QStringLiteral("erin"), QStringLiteral("bob"), QStringLiteral("carol")}));
        QCOMPARE(eval(QStringLiteral("launcherData.friendsOnline")).toInt(), 4);
        QCOMPARE(eval(QStringLiteral("launcherData.friendsInGame")).toInt(), 3);
        QCOMPARE(eval(QStringLiteral("launcherData.playingNow.map(p => p.name + ':' + p.friends.length)")).toStringList(),
            QStringList({QStringLiteral("Portal 2:2"), QStringLiteral("Minecraft:1")}));
        TRY_VERIFY(eval(QStringLiteral("launcherData.games.length === 3")).toBool());
        QCOMPARE(eval(QStringLiteral("launcherData.friendsFor({ appid: '620' }).length")).toInt(), 2);
        QCOMPARE(eval(QStringLiteral("launcherData.friendsFor({ appid: '' }).length")).toInt(), 0);
        QVERIFY(!eval(QStringLiteral("launcherData.friendsNeedsApiKey")).toBool());
    }

    void friendsFollowTheSnapshotFile()
    {
        m_harness.respondWithLibrary();
        QVERIFY(m_harness.writeFile(m_harness.friendsPath(), LauncherTest::fixture("friends.json")));
        QVERIFY(m_harness.openHost(false));
        TRY_COMPARE(eval(QStringLiteral("launcherData.friends.length")).toInt(), 5);
        QVERIFY(
            m_harness.writeFile(m_harness.friendsPath(), R"({"ok": false, "needs_api_key": true, "error": "no steam_api_key in config"})"));
        TRY_VERIFY(eval(QStringLiteral("launcherData.friendsNeedsApiKey")).toBool());
        QCOMPARE(eval(QStringLiteral("launcherData.friends.length")).toInt(), 0);
        QCOMPARE(eval(QStringLiteral("launcherData.friendsError")).toString(), QStringLiteral("no steam_api_key in config"));
        QVERIFY(m_harness.writeFile(m_harness.friendsPath(), R"({"ok": false, "error": "HTTP 403: steam_api_key rejected"})"));
        TRY_COMPARE(eval(QStringLiteral("launcherData.friendsError")).toString(), QStringLiteral("HTTP 403: steam_api_key rejected"));
        QVERIFY(eval(QStringLiteral("launcherData.friendsNeedsApiKey")).toBool());
    }

    void friendsOffReadsNothing()
    {
        m_harness.respondWithLibrary();
        QVERIFY(m_harness.writeFile(m_harness.friendsPath(), LauncherTest::fixture("friends.json")));
        QVERIFY(m_harness.openHost(false, {{QStringLiteral("showFriends"), false}}));
        QVERIFY(m_harness.waitHandled(QStringLiteral("printf ")));
        QCOMPARE(eval(QStringLiteral("launcherData.friendsPath")).toString(), m_harness.friendsPath());
        QCOMPARE(eval(QStringLiteral("launcherData.friends.length")).toInt(), 0);
    }

    void savingTheSteamKeyReportsTheResult_data()
    {
        QTest::addColumn<int>("exitCode");
        QTest::addColumn<QString>("expected");
        QTest::newRow("saved") << 0 << QStringLiteral("Steam API key saved");
        QTest::newRow("rejected") << 1 << QStringLiteral("the key was rejected");
    }

    void savingTheSteamKeyReportsTheResult()
    {
        QFETCH(int, exitCode);
        QFETCH(QString, expected);
        m_harness.respondWithLibrary({response(QStringLiteral("--set-key"), "", exitCode, QStringLiteral("the key was rejected\n"))});
        QVERIFY(m_harness.openHost(false));
        eval(QStringLiteral("launcherData.setSteamApiKey('   ')"));
        QCOMPARE(count(QStringLiteral("$HOME/.local/bin/portal-friends")), 0);
        eval(QStringLiteral("launcherData.setSteamApiKey(\" ab'c \")"));
        QVERIFY(eval(QStringLiteral("launcherData.steamKeyBusy")).toBool());
        QCOMPARE(count(QStringLiteral("$HOME/.local/bin/portal-friends --set-key 'ab'\\''c' # ")), 1);
        TRY_VERIFY(!eval(QStringLiteral("launcherData.steamKeyBusy")).toBool());
        QCOMPARE(eval(QStringLiteral("launcherData.steamKeyResult")).toString(), expected);
        QCOMPARE(eval(QStringLiteral("launcherData.steamKeyError")).toBool(), exitCode != 0);
    }

    void packageSearchNeedsLettersAndLength_data()
    {
        QTest::addColumn<QString>("query");
        QTest::addColumn<bool>("searches");
        QTest::newRow("three letters") << QStringLiteral("vim") << true;
        QTest::newRow("two letters") << QStringLiteral("vi") << false;
        QTest::newRow("packages mode two letters") << QStringLiteral("s vi") << true;
        QTest::newRow("packages mode one letter") << QStringLiteral("s v") << false;
        QTest::newRow("digits only") << QStringLiteral("1234") << false;
        QTest::newRow("apps mode") << QStringLiteral("a vim") << false;
    }

    void packageSearchNeedsLettersAndLength()
    {
        QFETCH(QString, query);
        QFETCH(bool, searches);
        QVERIFY(m_harness.openHost(false));
        eval(QStringLiteral("field.text = '%1'").arg(query));
        QCOMPARE(eval(QStringLiteral("launcherData.packageTerm !== ''")).toBool(), searches);
        QCOMPARE(eval(QStringLiteral("launcherData.packagesBusy")).toBool(), searches);
        if (searches) {
            TRY_COMPARE(count(QStringLiteral("$HOME/.local/bin/portal-packages ")), 1);
        }
    }

    void packageSearchIsDebouncedAndIgnoresStaleAnswers()
    {
        m_harness.respond({response(QStringLiteral("portal-packages 'vim' 6 # "), R"({"query": "vim", "packages": []})"),
            response(QStringLiteral("portal-packages 'vimb' 6 # "), LauncherTest::fixture("packages.json"))});
        QVERIFY(m_harness.openHost(false));
        eval(QStringLiteral("field.text = 'vi'"));
        eval(QStringLiteral("field.text = 'vim'"));
        eval(QStringLiteral("field.text = 'vimb'"));
        TRY_COMPARE(eval(QStringLiteral("launcherData.packagesQuery")).toString(), QStringLiteral("vimb"));
        QCOMPARE(count(QStringLiteral("$HOME/.local/bin/portal-packages ")), 1);
        QCOMPARE(eval(QStringLiteral("launcherData.packages.length")).toInt(), 2);
        QVERIFY(!eval(QStringLiteral("launcherData.packagesBusy")).toBool());
        eval(QStringLiteral("field.text = ''"));
        QCOMPARE(eval(QStringLiteral("launcherData.packages.length")).toInt(), 0);
    }

    void packageSearchOffAndBrokenAnswers()
    {
        m_harness.respond({response(QStringLiteral("portal-packages"), "oops")});
        QVERIFY(m_harness.openHost(false));
        eval(QStringLiteral("field.text = 's vim'"));
        TRY_VERIFY(!eval(QStringLiteral("launcherData.packagesBusy")).toBool());
        QCOMPARE(eval(QStringLiteral("launcherData.packages.length")).toInt(), 0);
        QCOMPARE(eval(QStringLiteral("launcherData.packageTerm")).toString(), QStringLiteral("vim"));
        m_harness.config()->insert(QStringLiteral("searchPackages"), false);
        QCOMPARE(eval(QStringLiteral("launcherData.packageTerm")).toString(), QString());
    }

    void hiddenAppsFollowTheStateFile()
    {
        QVERIFY(m_harness.writeFile(statePath("hidden.json"), "broken"));
        QVERIFY(m_harness.host(false));
        QVERIFY(m_harness.writeFile(statePath("hidden.json"), R"(["org.kde.konsole"])"));
        TRY_VERIFY(eval(QStringLiteral("launcherData.isHidden('applications:org.kde.konsole.desktop')")).toBool());
        QVERIFY(m_harness.writeFile(statePath("hidden.json"), R"({"not": "a list"})"));
        TRY_VERIFY(!eval(QStringLiteral("launcherData.isHidden('org.kde.konsole.desktop')")).toBool());
        QVERIFY(m_harness.writeFile(statePath("hidden.json"), R"(["firefox"])"));
        TRY_VERIFY(eval(QStringLiteral("launcherData.isHidden('firefox.desktop')")).toBool());
        QVERIFY(!eval(QStringLiteral("launcherData.isHidden('')")).toBool());
    }

    void foldersFollowTheStateFileAndGroupPins()
    {
        QVERIFY(m_harness.writeFile(statePath("folders.json"),
            R"([{"id": "work", "name": "Work", "apps": ["org.kde.konsole.desktop", "org.kde.dolphin.desktop"]}, {"id": "bad"}, null])"));
        QVERIFY(m_harness.host(false));
        TRY_COMPARE(eval(QStringLiteral("launcherData.folders.length")).toInt(), 1);
        TRY_COMPARE(eval(QStringLiteral("launcherData.pinnedEntries.map(e => e.kind + ':' + (e.id || e.favoriteId))")).toStringList(),
            QStringList {QStringLiteral("folder:work")});
        QCOMPARE(eval(QStringLiteral("launcherData.pinnedEntries[0].apps")).toList().size(), 2);
        eval(QStringLiteral("launcherData.removeFromFolder('org.kde.dolphin.desktop')"));
        TRY_COMPARE(eval(QStringLiteral("launcherData.pinnedEntries.map(e => e.kind)")).toStringList(),
            QStringList({QStringLiteral("folder"), QStringLiteral("app")}));
    }

    void creatingAFolderTakesAppsOutOfOthers()
    {
        QVERIFY(m_harness.host(false));
        eval(QStringLiteral("launcherData.folders = [{ id: 'old', name: 'Old', apps: ['org.kde.konsole.desktop'] }]"));
        const QString id
            = eval(QStringLiteral("launcherData.createFolder(['org.kde.konsole.desktop', 'org.kde.dolphin.desktop'])")).toString();
        QVERIFY(id.startsWith(QStringLiteral("folder-")));
        QCOMPARE(eval(QStringLiteral("launcherData.folders.map(f => f.name + ':' + f.apps.length)")).toStringList(),
            QStringList {QStringLiteral("Folder:2")});
        QVERIFY(
            m_harness.commands().last().startsWith(QStringLiteral("$HOME/.local/bin/portal-games --folder-create '%1' 'Folder' ").arg(id)));
        eval(QStringLiteral("launcherData.renameFolder('%1', '   ')").arg(id));
        QCOMPARE(eval(QStringLiteral("launcherData.folderById('%1').name").arg(id)).toString(), QStringLiteral("Folder"));
        eval(QStringLiteral("launcherData.renameFolder('%1', ' Tools ')").arg(id));
        QCOMPARE(eval(QStringLiteral("launcherData.folderById('%1').name").arg(id)).toString(), QStringLiteral("Tools"));
        QCOMPARE(m_harness.commands().last(), QStringLiteral("$HOME/.local/bin/portal-games --folder-rename '%1' 'Tools'").arg(id));
    }

    void sidebarWritesAreQueuedAndReloaded()
    {
        m_harness.respond({response(QStringLiteral("--sidebar # "), R"([{"kind": "app", "id": "firefox", "name": "Firefox"}])")});
        QVERIFY(m_harness.openHost(false));
        TRY_COMPARE(eval(QStringLiteral("launcherData.sidebarPins.length")).toInt(), 1);
        m_harness.respond({});
        eval(QStringLiteral("launcherData.addSidebar({ kind: 'app', id: 'a' }, 0)"));
        eval(QStringLiteral("launcherData.addSidebar({ kind: 'path', id: 'file:///tmp' })"));
        QCOMPARE(eval(QStringLiteral("launcherData.sidebarPins.map(p => p.id)")).toStringList(),
            QStringList({QStringLiteral("a"), QStringLiteral("firefox"), QStringLiteral("file:///tmp")}));
        QCOMPARE(count(QStringLiteral("$HOME/.local/bin/portal-games --sidebar-set ")), 1);
        QCOMPARE(eval(QStringLiteral("launcherData.sidebarWrites")).toInt(), 1);
        QVERIFY(!eval(QStringLiteral("launcherData.sidebarQueued")).toString().isEmpty());
        eval(QStringLiteral("launcherData.moveSidebar(0, 99)"));
        QCOMPARE(eval(QStringLiteral("launcherData.sidebarPins[2].id")).toString(), QStringLiteral("a"));
        eval(QStringLiteral("launcherData.moveSidebar(-1, 0)"));
        QCOMPARE(eval(QStringLiteral("launcherData.sidebarPins[2].id")).toString(), QStringLiteral("a"));
        QVERIFY(!eval(QStringLiteral("launcherData.openSidebarPin({ kind: 'app', id: 'gone', missing: true })")).toBool());
        QVERIFY(eval(QStringLiteral("launcherData.openSidebarPin({ kind: 'app', id: 'org.kde.kate' })")).toBool());
        QVERIFY(m_harness.commands().contains(QStringLiteral("kstart --application 'org.kde.kate'")));
    }

    void searchLearnsWhatYouOpen()
    {
        QVERIFY(m_harness.host(false));
        eval(QStringLiteral("launcherData.learn('Fi', 'firefox.desktop')"));
        eval(QStringLiteral("launcherData.learn('fire', 'firefox.desktop')"));
        eval(QStringLiteral("launcherData.learn('fi', 'files.desktop')"));
        eval(QStringLiteral("launcherData.learn('', 'nothing.desktop')"));
        QCOMPARE(eval(QStringLiteral("launcherData.learnedFor('FI')")).toStringList(),
            QStringList({QStringLiteral("firefox.desktop"), QStringLiteral("files.desktop")}));
        QCOMPARE(eval(QStringLiteral("launcherData.learnedFor('zzz')")).toStringList(), QStringList());
        const QString stored = m_harness.config()->value(QStringLiteral("learnedRanking")).toString();
        QVERIFY(stored.contains(QStringLiteral("\"fir\":{\"firefox.desktop\":1}")));
        QVERIFY(!stored.contains(QStringLiteral("nothing")));
    }

    void relativeTimesAndKeyNames()
    {
        QVERIFY(m_harness.host(false));
        const qint64 now = QDateTime::currentSecsSinceEpoch();
        QCOMPARE(eval(QStringLiteral("launcherData.relativeTime(0)")).toString(), QString());
        QCOMPARE(eval(QStringLiteral("launcherData.relativeTime(%1)").arg(now - 60)).toString(), QStringLiteral("just now"));
        QCOMPARE(eval(QStringLiteral("launcherData.relativeTime(%1)").arg(now - 3 * 3600)).toString(), QStringLiteral("3 hours ago"));
        QCOMPARE(eval(QStringLiteral("launcherData.relativeTime(%1)").arg(now - 86400)).toString(), QStringLiteral("yesterday"));
        QCOMPARE(eval(QStringLiteral("launcherData.relativeTime(%1)").arg(now - 5 * 86400)).toString(), QStringLiteral("5 days ago"));
        QCOMPARE(eval(QStringLiteral("launcherData.relativeTime(%1)").arg(now - 21 * 86400)).toString(), QStringLiteral("3 weeks ago"));
        QCOMPARE(eval(QStringLiteral("launcherData.keyText('Mod+Shift+page_down')")).toString(), QStringLiteral("Meta + Shift + PgDn"));
        QCOMPARE(eval(QStringLiteral("launcherData.keyText('Meta++')")).toString(), QStringLiteral("Meta + +"));
        QCOMPARE(eval(QStringLiteral("launcherData.keyText('Ctrl+Shift++')")).toString(), QStringLiteral("Ctrl + Shift + +"));
        QCOMPARE(eval(QStringLiteral("launcherData.desktopKey('applications:org.kde.kate.desktop')")).toString(),
            QStringLiteral("org.kde.kate"));
    }
};

LAUNCHER_TEST_MAIN(TestLauncherDataQml)
#include "test_launcher_data_qml.moc"
