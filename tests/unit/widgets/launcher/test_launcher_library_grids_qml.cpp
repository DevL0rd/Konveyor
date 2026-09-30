#include "appsharness.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

using LauncherTest::fixture;
using LauncherTest::response;

namespace
{

QByteArray gamesWith(const QJsonArray &games)
{
    return QJsonDocument(QJsonObject {{QStringLiteral("games"), games}}).toJson();
}

QJsonObject gameNamed(const QString &id, const QString &name, int last)
{
    return {{QStringLiteral("id"), id}, {QStringLiteral("name"), name}, {QStringLiteral("icon"), id}, {QStringLiteral("appid"), QString()},
        {QStringLiteral("launch"), id}, {QStringLiteral("last"), last}};
}

QByteArray friendsWith(const QJsonArray &friends)
{
    return QJsonDocument(QJsonObject {{QStringLiteral("ok"), true}, {QStringLiteral("friends"), friends}}).toJson();
}

QJsonObject friendNamed(const QString &name, int state, const QString &game = {})
{
    return {{QStringLiteral("name"), name}, {QStringLiteral("state"), state}, {QStringLiteral("ingame"), !game.isEmpty()},
        {QStringLiteral("appid"), QString()}, {QStringLiteral("game"), game},
        {QStringLiteral("chat"), QStringLiteral("steam://friends/message/") + name},
        {QStringLiteral("profile"), QStringLiteral("steam://url/SteamIDPage/") + name}};
}

}

#define EXPECT_TILES(...)                                                                                                                  \
    do {                                                                                                                                   \
        expectTiles(__VA_ARGS__);                                                                                                          \
        if (QTest::currentTestFailed()) {                                                                                                  \
            return;                                                                                                                        \
        }                                                                                                                                  \
    } while (false)

class TestLauncherLibraryGridsQml : public AppsTest::TestCase
{
    Q_OBJECT

private:
    const QString m_games = QStringLiteral("launcher.currentView().sections[0]");

    void expectTiles(const QString &grid, const QString &truth, const QStringList &expected)
    {
        TRY_COMPARE(tilesOf(grid, truth), expected);
        TRY_VERIFY(eval(QStringLiteral("launcher.liveSections().some(s => s.sectionActive && s.currentIndex >= 0)")).toBool());
        QCOMPARE(hitsOtherTiles(grid), QString());
        for (int position = 0; position < expected.size(); ++position) {
            QVERIFY2(hoverLightsOnlyThat(grid, position), qPrintable(expected.at(position)));
        }
    }

    void reloadGames(const QByteArray &json)
    {
        m_harness.respondWithLibrary({response(QStringLiteral("^\\$HOME/\\.local/bin/portal-games # "), json)});
        eval(QStringLiteral("launcherData.refreshGames(true)"));
    }

private Q_SLOTS:
    void gameTilesFollowTheLibrary_data()
    {
        QTest::addColumn<QString>("view");
        for (const char *view : {"grid", "banner", "list"}) {
            QTest::newRow(view) << QString::fromLatin1(view);
        }
    }

    void gameTilesFollowTheLibrary()
    {
        QFETCH(QString, view);
        QVERIFY(openLibrary(false, {{QStringLiteral("gamesView"), view}}));
        QVERIFY(goTo(QStringLiteral("games")));
        const QString portal = QStringLiteral("steam_app_620");
        const QString halfLife = QStringLiteral("steam_app_70");
        const QString celeste = QStringLiteral("celeste");
        const QString id = QStringLiteral("modelData.id");
        EXPECT_TILES(m_games, id, {portal, halfLife, celeste});
        m_harness.config()->insert(QStringLiteral("gamesSort"), QStringLiteral("name"));
        EXPECT_TILES(m_games, id, {celeste, halfLife, portal});
        page(QStringLiteral("filter = 'played'"));
        EXPECT_TILES(m_games, id, {halfLife, portal});
        page(QStringLiteral("filter = 'friends'"));
        EXPECT_TILES(m_games, id, {portal});
        page(QStringLiteral("filter = 'all'"));
        EXPECT_TILES(m_games, id, {celeste, halfLife, portal});
        QJsonArray changed = QJsonDocument::fromJson(fixture("games.json")).object().value(QStringLiteral("games")).toArray();
        changed.removeAt(2);
        changed.append(gameNamed(QStringLiteral("hades"), QStringLiteral("Hades"), 3000));
        changed.append(gameNamed(QStringLiteral("braid"), QStringLiteral("Braid"), 0));
        reloadGames(gamesWith(changed));
        EXPECT_TILES(m_games, id, {QStringLiteral("braid"), celeste, QStringLiteral("hades"), portal});
        m_harness.config()->insert(QStringLiteral("gamesSort"), QStringLiteral("recent"));
        EXPECT_TILES(m_games, id, {QStringLiteral("hades"), portal, QStringLiteral("braid"), celeste});
        QVERIFY(hoverLightsOnlyThat(m_games, 2));
        QTest::keyClick(m_harness.window(), Qt::Key_Return);
        TRY_VERIFY(m_harness.commands().constLast().contains(QStringLiteral("--track 'braid'")));
    }

    void theKeyboardSelectionStaysOnAShownGame()
    {
        QVERIFY(openLibrary(false, {{QStringLiteral("gamesSort"), QStringLiteral("name")}}));
        QVERIFY(goTo(QStringLiteral("games")));
        TRY_COMPARE(tilesOf(m_games, QStringLiteral("modelData.id")).size(), 3);
        TRY_VERIFY(eval(m_games + QStringLiteral(".sectionActive && ") + m_games + QStringLiteral(".currentIndex === 0")).toBool());
        QVERIFY(hoverLightsOnlyThat(m_games, 2));
        page(QStringLiteral("filter = 'friends'"));
        TRY_COMPARE(tilesOf(m_games, QStringLiteral("modelData.id")), QStringList {QStringLiteral("steam_app_620")});
        TRY_COMPARE(eval(m_games + QStringLiteral(".currentIndex")).toInt(), 0);
        QTest::keyClick(m_harness.window(), Qt::Key_Return);
        TRY_VERIFY(m_harness.commands().constLast().contains(QStringLiteral("--track 'steam_app_620'")));
    }

    void friendRowsFollowTheSnapshot()
    {
        QVERIFY(openLibrary(false));
        QVERIFY(goTo(QStringLiteral("friends")));
        const QString inGame = QStringLiteral("launcher.currentView().sections[1]");
        const QString online = QStringLiteral("launcher.currentView().sections[2]");
        const QString offline = QStringLiteral("launcher.currentView().sections[3]");
        const QString label = QStringLiteral("label");
        EXPECT_TILES(inGame, label, {QStringLiteral("Alice"), QStringLiteral("dave"), QStringLiteral("erin")});
        EXPECT_TILES(online, label, {QStringLiteral("bob")});
        EXPECT_TILES(offline, label, {QStringLiteral("carol")});
        QVERIFY(m_harness.writeFile(m_harness.friendsPath(),
            friendsWith({friendNamed(QStringLiteral("bob"), 1, QStringLiteral("Celeste")), friendNamed(QStringLiteral("frank"), 1),
                friendNamed(QStringLiteral("gina"), 1), friendNamed(QStringLiteral("Alice"), 0), friendNamed(QStringLiteral("erin"), 0),
                friendNamed(QStringLiteral("carol"), 0)})));
        EXPECT_TILES(inGame, label, {QStringLiteral("bob")});
        EXPECT_TILES(online, label, {QStringLiteral("frank"), QStringLiteral("gina")});
        EXPECT_TILES(offline, label, {QStringLiteral("Alice"), QStringLiteral("carol"), QStringLiteral("erin")});
        QCOMPARE(tilesOf(offline, QStringLiteral("subtitle")),
            QStringList({QStringLiteral("Offline"), QStringLiteral("Offline"), QStringLiteral("Offline")}));
        QCOMPARE(tilesOf(inGame, QStringLiteral("subtitle")), QStringList {QStringLiteral("Playing Celeste")});
        QVERIFY(hoverLightsOnlyThat(online, 1));
        QTest::keyClick(m_harness.window(), Qt::Key_Return);
        TRY_COMPARE(LauncherTest::urls().opened.constLast(), QUrl(QStringLiteral("steam://friends/message/gina")));
    }

    void theSelectedFriendStaysSelectedWhenTheListRefreshes()
    {
        QVERIFY(openLibrary(false));
        QVERIFY(goTo(QStringLiteral("friends")));
        const QString inGame = QStringLiteral("launcher.currentView().sections[1]");
        TRY_COMPARE(tilesOf(inGame, QStringLiteral("label")),
            QStringList({QStringLiteral("Alice"), QStringLiteral("dave"), QStringLiteral("erin")}));
        TRY_VERIFY(eval(QStringLiteral("launcher.liveSections().some(s => s.sectionActive && s.currentIndex >= 0)")).toBool());
        QVERIFY(hoverLightsOnlyThat(inGame, 1));
        QTest::mouseMove(m_harness.window(), QPoint(2, 2));
        QJsonArray friends = QJsonDocument::fromJson(fixture("friends.json")).object().value(QStringLiteral("friends")).toArray();
        friends.append(friendNamed(QStringLiteral("abe"), 1, QStringLiteral("Braid")));
        QVERIFY(m_harness.writeFile(m_harness.friendsPath(), friendsWith(friends)));
        TRY_COMPARE(tilesOf(inGame, QStringLiteral("label")),
            QStringList({QStringLiteral("abe"), QStringLiteral("Alice"), QStringLiteral("dave"), QStringLiteral("erin")}));
        TRY_COMPARE(eval(inGame + QStringLiteral(".currentIndex")).toInt(), 2);
        QCOMPARE(selectedIn(inGame), QList<int> {2});
        QTest::keyClick(m_harness.window(), Qt::Key_Return);
        TRY_COMPARE(LauncherTest::urls().opened.constLast(), QUrl(QStringLiteral("steam://friends/message/4")));
    }

    void homeLibraryCardsFollowPlayAndFriends()
    {
        QVERIFY(openLibrary(false));
        const QString friendsPlaying = QStringLiteral("launcher.currentView().sections[3]");
        const QString playing = QStringLiteral("launcher.currentView().sections[4]");
        const QString name = QStringLiteral("modelData.name");
        EXPECT_TILES(friendsPlaying, name, {QStringLiteral("Portal 2"), QStringLiteral("Minecraft")});
        EXPECT_TILES(playing, name, {QStringLiteral("Portal 2"), QStringLiteral("Half-Life")});
        eval(QStringLiteral("launcherData.launchGame(launcherData.games.find(g => g.id === 'steam_app_70'))"));
        EXPECT_TILES(playing, name, {QStringLiteral("Half-Life"), QStringLiteral("Portal 2")});
        eval(QStringLiteral("launcherData.launchGame(launcherData.games.find(g => g.id === 'celeste'))"));
        EXPECT_TILES(playing, name, {QStringLiteral("Celeste"), QStringLiteral("Half-Life"), QStringLiteral("Portal 2")});
        QVERIFY(m_harness.writeFile(m_harness.friendsPath(),
            friendsWith({friendNamed(QStringLiteral("bob"), 1, QStringLiteral("Celeste")),
                friendNamed(QStringLiteral("frank"), 1, QStringLiteral("Braid")),
                friendNamed(QStringLiteral("gina"), 1, QStringLiteral("Braid"))})));
        EXPECT_TILES(friendsPlaying, name, {QStringLiteral("Braid"), QStringLiteral("Celeste")});
        QVERIFY(hoverLightsOnlyThat(playing, 1));
        QTest::keyClick(m_harness.window(), Qt::Key_Return);
        TRY_VERIFY(m_harness.commands().constLast().contains(QStringLiteral("--track 'steam_app_70'")));
    }
};

LAUNCHER_TEST_MAIN(TestLauncherLibraryGridsQml)
#include "test_launcher_library_grids_qml.moc"
