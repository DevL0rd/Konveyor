#include "launcherharness.h"

namespace
{

QString texts(const QString &entries)
{
    return entries + QStringLiteral(".map(e => e.separator ? '-' : e.text)");
}

QString game(const char *id)
{
    return QStringLiteral("launcherData.games.find(g => g.id === '%1')").arg(QLatin1String(id));
}

}

class TestLauncherGamesQml : public LauncherTest::TestCase
{
    Q_OBJECT

private:
    QStringList shownGames() { return page(QStringLiteral("shown.map(g => g.id)")).toStringList(); }

private Q_SLOTS:
    void gamesSortAndFilter_data()
    {
        QTest::addColumn<QString>("sort");
        QTest::addColumn<QString>("filter");
        QTest::addColumn<QStringList>("expected");
        const QString portal = QStringLiteral("steam_app_620");
        const QString halfLife = QStringLiteral("steam_app_70");
        const QString celeste = QStringLiteral("celeste");
        QTest::newRow("recent") << QStringLiteral("recent") << QStringLiteral("all") << QStringList {portal, halfLife, celeste};
        QTest::newRow("name") << QStringLiteral("name") << QStringLiteral("all") << QStringList {celeste, halfLife, portal};
        QTest::newRow("friends") << QStringLiteral("friends") << QStringLiteral("all") << QStringList {portal, halfLife, celeste};
        QTest::newRow("played") << QStringLiteral("name") << QStringLiteral("played") << QStringList {halfLife, portal};
        QTest::newRow("friends playing") << QStringLiteral("recent") << QStringLiteral("friends") << QStringList {portal};
    }

    void gamesSortAndFilter()
    {
        QFETCH(QString, sort);
        QFETCH(QString, filter);
        QFETCH(QStringList, expected);
        QVERIFY(openLibrary(false, {{QStringLiteral("gamesSort"), sort}}));
        QVERIFY(goTo(QStringLiteral("games")));
        page(QStringLiteral("filter = '%1'").arg(filter));
        QCOMPARE(shownGames(), expected);
    }

    void filtersCycleAndExplainEmptyResults()
    {
        QVERIFY(m_harness.openHost(false));
        QVERIFY(goTo(QStringLiteral("games")));
        QCOMPARE(page(QStringLiteral("shown.length")).toInt(), 0);
        page(QStringLiteral("cycle(false)"));
        QCOMPARE(page(QStringLiteral("filter")).toString(), QStringLiteral("friends"));
        page(QStringLiteral("cycle(true)"));
        QCOMPARE(page(QStringLiteral("filter")).toString(), QStringLiteral("all"));
    }

    void everyGamesViewLaunchesTheSelection_data()
    {
        QTest::addColumn<QString>("view");
        for (const char *view : {"grid", "banner", "list", "carousel", "coverflow"}) {
            QTest::newRow(view) << QString::fromLatin1(view);
        }
    }

    void everyGamesViewLaunchesTheSelection()
    {
        QFETCH(QString, view);
        QObject *host = openLibrary(true, {{QStringLiteral("gamesView"), view}});
        QVERIFY(host);
        QVERIFY(goTo(QStringLiteral("games")));
        TRY_VERIFY(page(QStringLiteral("sections[0].shownCount")).toInt() == 3);
        eval(QStringLiteral("launcher.resetSelection()"));
        TRY_VERIFY(
            eval(QStringLiteral(
                     "launcher.currentSection().itemAtIndex(0) !== null || launcher.page === 'games' && launcher.currentView().carousel"))
                .toBool());
        eval(QStringLiteral("launcher.activateCurrent()"));
        QCOMPARE(host->property("hideCount").toInt(), 1);
        QVERIFY(m_harness.commands().last().contains(QStringLiteral("$HOME/.local/bin/portal-games --track '")));
    }

    void cardSizeIsClamped()
    {
        QVERIFY(m_harness.openHost(false, {{QStringLiteral("gameCardSize"), 17}}));
        QVERIFY(goTo(QStringLiteral("games")));
        page(QStringLiteral("zoom(5)"));
        QCOMPARE(m_harness.config()->value(QStringLiteral("gameCardSize")).toInt(), 18);
        page(QStringLiteral("zoom(-50)"));
        QCOMPARE(m_harness.config()->value(QStringLiteral("gameCardSize")).toInt(), 6);
        page(QStringLiteral("setView('list')"));
        QCOMPARE(m_harness.config()->value(QStringLiteral("gamesView")).toString(), QStringLiteral("list"));
    }

    void steamGameMenusLinkToSteamAndFriends()
    {
        QVERIFY(openLibrary(false));
        const QString entries = QStringLiteral("launcher.gameEntries(%1)").arg(game("steam_app_620"));
        QCOMPARE(eval(texts(entries)).toStringList(),
            QStringList({QStringLiteral("Play"), QStringLiteral("Pin to sidebar"), QStringLiteral("-"), QStringLiteral("Store page"),
                QStringLiteral("Properties"), QStringLiteral("Browse local files"), QStringLiteral("-"), QStringLiteral("Set custom art…"),
                QStringLiteral("-"), QStringLiteral("Playing now"), QStringLiteral("erin"), QStringLiteral("Alice")}));
        eval(entries + QStringLiteral(".find(e => e.text === 'Properties').run()"));
        QCOMPARE(LauncherTest::urls().opened, QList<QUrl> {QUrl(QStringLiteral("steam://gameproperties/620"))});
    }

    void otherGamesOfferArtReset()
    {
        QVERIFY(openLibrary(false));
        const QString entries = QStringLiteral("launcher.gameEntries(%1)").arg(game("celeste"));
        QCOMPARE(eval(texts(entries)).toStringList(),
            QStringList({QStringLiteral("Play"), QStringLiteral("Pin to sidebar"), QStringLiteral("-"), QStringLiteral("Set custom art…"),
                QStringLiteral("Reset art")}));
        eval(entries + QStringLiteral(".find(e => e.text === 'Reset art').run()"));
        QCOMPARE(m_harness.commands().last(), QStringLiteral("$HOME/.local/bin/portal-games --reset-art 'celeste'"));
    }

    void pickedArtIsSavedForTheGame()
    {
        QObject *host = openLibrary(false);
        QVERIFY(host);
        QVERIFY(m_harness.writeFile(m_harness.path(QStringLiteral("home/art one.png")), "png"));
        eval(QStringLiteral("launcherData.pickArt(%1)").arg(game("celeste")));
        QCOMPARE(host->property("hideCount").toInt(), 1);
        TRY_VERIFY(m_harness.evalData(QStringLiteral("artDialogLoader.item !== null")).toBool());
        const QUrl art = QUrl::fromLocalFile(m_harness.path(QStringLiteral("home/art one.png")));
        m_harness.evalData(QStringLiteral("artDialogLoader.item.selectedFile = '%1'").arg(art.toString(QUrl::FullyEncoded)));
        QCOMPARE(m_harness.evalData(QStringLiteral("artDialogLoader.item.selectedFile")).toUrl(), art);
        m_harness.evalData(QStringLiteral("artDialogLoader.item.accepted()"));
        QCOMPARE(
            m_harness.commands().last(), QStringLiteral("$HOME/.local/bin/portal-games --set-art 'celeste' '%1'").arg(art.toLocalFile()));
        TRY_VERIFY(!m_harness.evalData(QStringLiteral("artDialogLoader.active")).toBool());
    }

    void playingNowKnowsGamesThatLoadAfterFriends()
    {
        m_harness.respond({LauncherTest::response(QStringLiteral("^printf "), m_harness.friendsPath().toUtf8())});
        QVERIFY(m_harness.writeFile(m_harness.friendsPath(), LauncherTest::fixture("friends.json")));
        QVERIFY(m_harness.openHost(false));
        TRY_COMPARE(eval(QStringLiteral("launcherData.playingNow.length")).toInt(), 2);
        QVERIFY(eval(QStringLiteral("launcherData.playingNow[0].game === null")).toBool());
        m_harness.respondWithLibrary();
        eval(QStringLiteral("launcherData.refreshGames(true)"));
        TRY_COMPARE(eval(QStringLiteral("launcherData.games.length")).toInt(), 3);
        QCOMPARE(eval(QStringLiteral("launcherData.playingNow[0].game ? launcherData.playingNow[0].game.id : ''")).toString(),
            QStringLiteral("steam_app_620"));
    }

    void friendsPageGroupsFriends()
    {
        QVERIFY(openLibrary(false));
        QVERIFY(goTo(QStringLiteral("friends")));
        TRY_COMPARE(page(QStringLiteral("sections.map(s => s.count)")).toStringList(),
            QStringList({QStringLiteral("2"), QStringLiteral("3"), QStringLiteral("1"), QStringLiteral("1")}));
        QVERIFY(!page(QStringLiteral("needsApiKey")).toBool());
        TRY_VERIFY(page(QStringLiteral("sections[1].itemAtIndex(0) !== null")).toBool());
        eval(QStringLiteral("launcher.select(launcher.currentView().sections[1], 0)"));
        eval(QStringLiteral("launcher.activateCurrent()"));
        QCOMPARE(LauncherTest::urls().opened, QList<QUrl> {QUrl(QStringLiteral("steam://friends/message/1"))});
    }

    void friendMenusJoinWatchAndOpenProfiles()
    {
        QVERIFY(openLibrary(false));
        QCOMPARE(eval(texts(QStringLiteral("launcher.friendEntries(launcherData.friends.find(f => f.name === 'erin'))"))).toStringList(),
            QStringList({QStringLiteral("Open chat"), QStringLiteral("Join game"), QStringLiteral("Watch game"), QStringLiteral("-"),
                QStringLiteral("View profile")}));
        QCOMPARE(eval(texts(QStringLiteral("launcher.friendEntries(launcherData.friends.find(f => f.name === 'Alice'))"))).toStringList(),
            QStringList({QStringLiteral("Open chat"), QStringLiteral("-"), QStringLiteral("View profile"),
                QStringLiteral("Open profile in browser")}));
        QCOMPARE(eval(texts(QStringLiteral("launcher.friendsQuickEntries(null)"))).toStringList(),
            QStringList({QStringLiteral("In game"), QStringLiteral("Alice · Portal 2"), QStringLiteral("dave · Minecraft"),
                QStringLiteral("erin · Portal 2"), QStringLiteral("-"), QStringLiteral("Online"), QStringLiteral("bob"),
                QStringLiteral("-"), QStringLiteral("All friends")}));
    }

    void playingNowCardsOfferPlayJoinAndChat()
    {
        QVERIFY(openLibrary(false));
        QVERIFY(goTo(QStringLiteral("friends")));
        const QString card = QStringLiteral("launcher.currentView().sections[0].itemAtIndex(0)");
        TRY_VERIFY(eval(card + QStringLiteral(" !== null")).toBool());
        QCOMPARE(eval(texts(card + QStringLiteral(".entries()"))).toStringList(),
            QStringList({QStringLiteral("Play Portal 2"), QStringLiteral("-"), QStringLiteral("Alice"), QStringLiteral("Chat with Alice"),
                QStringLiteral("-"), QStringLiteral("erin"), QStringLiteral("Join erin"), QStringLiteral("Chat with erin"),
                QStringLiteral("-"), QStringLiteral("Store page")}));
    }

    void missingApiKeyShowsTheSetupCard()
    {
        m_harness.respondWithLibrary({LauncherTest::response(QStringLiteral("--set-key"), "")});
        QVERIFY(
            m_harness.writeFile(m_harness.friendsPath(), R"({"ok": false, "needs_api_key": true, "error": "no steam_api_key in config"})"));
        QVERIFY(m_harness.openHost(false));
        QVERIFY(goTo(QStringLiteral("friends")));
        TRY_VERIFY(page(QStringLiteral("needsApiKey")).toBool());
        eval(QStringLiteral("launcher.currentView().saveApiKey()"));
        QVERIFY(!m_harness.commands().join(QLatin1Char('\n')).contains(QStringLiteral("--set-key")));
    }

    void friendsPillSummarisesAndOpensTheFriendsPage()
    {
        QVERIFY(openLibrary(true));
        QCOMPARE(eval(QStringLiteral("launcherData.friendsInGame")).toInt(), 3);
        eval(QStringLiteral("launcher.openMenu(launcher.friendsQuickEntries(null), null)"));
        TRY_VERIFY(m_harness.view()->property("menuOpen").toBool());
        eval(QStringLiteral("menu.entries[menu.entries.length - 1].run()"));
        QCOMPARE(eval(QStringLiteral("launcher.page")).toString(), QStringLiteral("friends"));
    }
};

LAUNCHER_TEST_MAIN(TestLauncherGamesQml)
#include "test_launcher_games_qml.moc"
