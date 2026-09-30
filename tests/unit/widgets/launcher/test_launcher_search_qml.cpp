#include "launcherharness.h"

using LauncherTest::response;

namespace
{

const QStringList everyRunner {QStringLiteral("krunner_services"), QStringLiteral("krunner_systemsettings"), QStringLiteral("calculator"),
    QStringLiteral("unitconverter"), QStringLiteral("krunner_shell"), QStringLiteral("krunner_placesrunner"),
    QStringLiteral("krunner_recentdocuments"), QStringLiteral("baloosearch"), QStringLiteral("locations"),
    QStringLiteral("krunner_sessions"), QStringLiteral("krunner_powerdevil"), QStringLiteral("windows")};

}

class TestLauncherSearchQml : public LauncherTest::TestCase
{
    Q_OBJECT

private:
    bool search(const QString &text)
    {
        eval(QStringLiteral("field.text = '%1'; launcher.settleSearch()").arg(text));
        return QTest::qWaitFor(
            [this] { return eval(QStringLiteral("launcher.searchSettled && searchLoader.item !== null")).toBool(); }, 30000);
    }

    QVariant results(const QString &expression) { return eval(QStringLiteral("searchLoader.item.") + expression); }

private Q_SLOTS:
    void eachModeAsksItsRunners_data()
    {
        QTest::addColumn<QString>("text");
        QTest::addColumn<QVariantMap>("settings");
        QTest::addColumn<QStringList>("runners");
        const QStringList files {QStringLiteral("krunner_placesrunner"), QStringLiteral("krunner_recentdocuments"),
            QStringLiteral("baloosearch"), QStringLiteral("locations")};
        QTest::newRow("everything") << QStringLiteral("kon") << QVariantMap() << everyRunner;
        QTest::newRow("web on") << QStringLiteral("kon") << QVariantMap {{QStringLiteral("searchWeb"), true}}
                                << everyRunner + QStringList {QStringLiteral("krunner_webshortcuts")};
        QTest::newRow("all extras off") << QStringLiteral("kon")
                                        << QVariantMap {{QStringLiteral("searchFiles"), false}, {QStringLiteral("searchSettings"), false},
                                               {QStringLiteral("searchCalculator"), false}, {QStringLiteral("searchCommands"), false},
                                               {QStringLiteral("searchWindows"), false}}
                                        << QStringList {QStringLiteral("krunner_services"), QStringLiteral("krunner_sessions"),
                                               QStringLiteral("krunner_powerdevil")};
        QTest::newRow("apps") << QStringLiteral("a kon") << QVariantMap() << QStringList {QStringLiteral("krunner_services")};
        QTest::newRow("files") << QStringLiteral("f notes") << QVariantMap() << files;
        QTest::newRow("calc") << QStringLiteral("=2+2") << QVariantMap()
                              << QStringList {QStringLiteral("calculator"), QStringLiteral("unitconverter")};
        QTest::newRow("command") << QStringLiteral(">ls") << QVariantMap() << QStringList {QStringLiteral("krunner_shell")};
        QTest::newRow("games ask no runner") << QStringLiteral("g portal") << QVariantMap() << QStringList();
        QTest::newRow("friends ask no runner") << QStringLiteral("@bob") << QVariantMap() << QStringList();
    }

    void eachModeAsksItsRunners()
    {
        QFETCH(QString, text);
        QFETCH(QVariantMap, settings);
        QFETCH(QStringList, runners);
        QVERIFY(m_harness.openHost(false, settings));
        QVERIFY(search(text));
        const QVariantList queries = eval(QStringLiteral("launcherData.runner.queries")).toList();
        QCOMPARE(queries.isEmpty(), runners.isEmpty());
        if (!runners.isEmpty()) {
            QCOMPARE(queries.last().toMap().value(QStringLiteral("runners")).toStringList(), runners);
            QCOMPARE(queries.last().toMap().value(QStringLiteral("query")).toString(), eval(QStringLiteral("launcher.term")).toString());
        }
    }

    void theBestMatchIsTheFirstShownApp()
    {
        QVERIFY(openLibrary(false));
        QVERIFY(search(QStringLiteral("o")));
        TRY_COMPARE(results(QStringLiteral("hero.kind")).toString(), QStringLiteral("app"));
        QCOMPARE(results(QStringLiteral("appIds[searchLoader.item.hero.row]")).toString(), QStringLiteral("org.kde.konsole.desktop"));
        eval(QStringLiteral("launcherData.setHidden('org.kde.konsole.desktop', true)"));
        TRY_COMPARE(results(QStringLiteral("appIds[searchLoader.item.hero.row]")).toString(), QStringLiteral("org.kde.dolphin.desktop"));
        eval(QStringLiteral("launcherData.learn('o', 'firefox.desktop')"));
        TRY_COMPARE(results(QStringLiteral("appIds[searchLoader.item.hero.row]")).toString(), QStringLiteral("firefox.desktop"));
    }

    void steamAppsBecomeGameHeroes()
    {
        QVERIFY(openLibrary(false));
        QVERIFY(search(QStringLiteral("portal")));
        TRY_COMPARE(results(QStringLiteral("hero.kind")).toString(), QStringLiteral("game"));
        QCOMPARE(results(QStringLiteral("hero.game.id")).toString(), QStringLiteral("steam_app_620"));
        QVERIFY(results(QStringLiteral("gameRows.every(g => g.id !== 'steam_app_620')")).toBool());
    }

    void learnedGamesWinTheBestMatch()
    {
        QVERIFY(openLibrary(false));
        eval(QStringLiteral("launcherData.learn('l', 'game:steam_app_70')"));
        QVERIFY(search(QStringLiteral("l")));
        TRY_COMPARE(results(QStringLiteral("hero.kind")).toString(), QStringLiteral("game"));
        QCOMPARE(results(QStringLiteral("hero.game.id")).toString(), QStringLiteral("steam_app_70"));
    }

    void gamesAndFriendsModesOnlyListTheirKind()
    {
        QVERIFY(openLibrary(false));
        QVERIFY(search(QStringLiteral("g e")));
        TRY_COMPARE(results(QStringLiteral("gameMatches.map(g => g.id)")).toStringList(),
            QStringList({QStringLiteral("celeste"), QStringLiteral("steam_app_70")}));
        QCOMPARE(results(QStringLiteral("friendMatches.length")).toInt(), 0);
        QVERIFY(search(QStringLiteral("@minecraft")));
        TRY_COMPARE(results(QStringLiteral("friendMatches.map(f => f.name)")).toStringList(), QStringList {QStringLiteral("dave")});
        QCOMPARE(results(QStringLiteral("gameMatches.length")).toInt(), 0);
    }

    void gamesAndFriendsStayOutWhenTurnedOff()
    {
        QVERIFY(openLibrary(false));
        m_harness.config()->insert(QStringLiteral("showGames"), false);
        m_harness.config()->insert(QStringLiteral("showFriends"), false);
        QVERIFY(search(QStringLiteral("a")));
        QCOMPARE(results(QStringLiteral("gameMatches.length")).toInt(), 0);
        QCOMPARE(results(QStringLiteral("friendMatches.length")).toInt(), 0);
    }

    void packagesComeLastAndPackagesModeAsksForMore()
    {
        m_harness.respond({response(QStringLiteral("portal-packages 'vimb' "), LauncherTest::fixture("packages.json"))});
        QVERIFY(m_harness.openHost(false));
        QVERIFY(search(QStringLiteral("s vimb")));
        TRY_COMPARE(eval(QStringLiteral("launcherData.packages.length")).toInt(), 2);
        QVERIFY(m_harness.commands().last().startsWith(QStringLiteral("$HOME/.local/bin/portal-packages 'vimb' 30 # ")));
        TRY_COMPARE(results(QStringLiteral("totalResults")).toInt(), 2);
        QCOMPARE(results(QStringLiteral("sections.slice(-1)[0].count")).toInt(), 2);
    }

    void nothingFoundCountsNothing()
    {
        QVERIFY(openLibrary(false));
        QVERIFY(search(QStringLiteral("zzzz")));
        TRY_COMPARE(results(QStringLiteral("hero.kind")).toString(), QStringLiteral("none"));
        QCOMPARE(results(QStringLiteral("totalResults")).toInt(), 0);
    }

    void enterRunsTheBestMatch()
    {
        QObject *host = openLibrary(false);
        QVERIFY(host);
        QVERIFY(search(QStringLiteral("fire")));
        TRY_COMPARE(results(QStringLiteral("hero.kind")).toString(), QStringLiteral("app"));
        TRY_VERIFY(eval(QStringLiteral("launcher.currentSection() !== null && launcher.currentSection().currentIndex === 0")).toBool());
        eval(QStringLiteral("launcher.activateCurrent()"));
        QCOMPARE(host->property("hideCount").toInt(), 1);
        QCOMPARE(eval(QStringLiteral("launcherData.runner.modelForRow(0).triggered[0].favoriteId")).toString(),
            QStringLiteral("firefox.desktop"));
        QVERIFY(eval(QStringLiteral("launcherData.learnedFor('fire')[0] === 'firefox.desktop'")).toBool());
    }

    void resultOrderFollowsTheSetting()
    {
        QVERIFY(m_harness.openHost(false, {{QStringLiteral("searchOrder"), QStringLiteral("games, nonsense ,apps")}}));
        QVERIFY(search(QStringLiteral("kon")));
        QCOMPARE(results(QStringLiteral("order")).toStringList(),
            QStringList({QStringLiteral("games"), QStringLiteral("apps"), QStringLiteral("answer"), QStringLiteral("windows"),
                QStringLiteral("settings"), QStringLiteral("files"), QStringLiteral("friends"), QStringLiteral("commands"),
                QStringLiteral("other")}));
        QCOMPARE(results(QStringLiteral("classify('Calculator')")).toString(), QStringLiteral("answer"));
        QCOMPARE(results(QStringLiteral("classify('Command Line')")).toString(), QStringLiteral("commands"));
        QCOMPARE(results(QStringLiteral("classify('Desktop Search')")).toString(), QStringLiteral("files"));
    }

    void settingsAndShortcutsAreSearchable()
    {
        m_harness.respond({response(QStringLiteral("konveyor-cheatsheet --json"), LauncherTest::fixture("shortcuts.json"))});
        QVERIFY(m_harness.openHost(false));
        QVERIFY(search(QStringLiteral("show desk")));
        TRY_COMPARE(
            results(QStringLiteral("shortcutMatches.map(m => m.action)")).toStringList(), QStringList {QStringLiteral("Show Desktop")});
        QVERIFY(search(QStringLiteral("gaps")));
        TRY_VERIFY(results(QStringLiteral("settingMatches.length")).toInt() > 0);
        QVERIFY(search(QStringLiteral("g")));
        QCOMPARE(results(QStringLiteral("settingMatches.length")).toInt(), 0);
        QCOMPARE(results(QStringLiteral("shortcutMatches.length")).toInt(), 0);
    }
};

LAUNCHER_TEST_MAIN(TestLauncherSearchQml)
#include "test_launcher_search_qml.moc"
