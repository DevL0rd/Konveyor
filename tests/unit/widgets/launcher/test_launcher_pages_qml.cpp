#include "launcherharness.h"

class TestLauncherPagesQml : public LauncherTest::TestCase
{
    Q_OBJECT

private:
    QStringList visibleSections()
    {
        return page(QStringLiteral("sections.filter(s => s && s.visible && s.shownCount > 0).map(s => s.shownCount)")).toStringList();
    }

private Q_SLOTS:
    void everyPageLoadsInBothModes_data()
    {
        QTest::addColumn<bool>("portal");
        QTest::addColumn<QString>("key");
        for (const bool portal : {false, true}) {
            for (const char *key : {"home", "apps", "games", "files", "friends", "system", "shortcuts", "settings"}) {
                QTest::addRow("%s %s", portal ? "App Portal" : "standalone", key) << portal << QString::fromLatin1(key);
            }
        }
    }

    void everyPageLoadsInBothModes()
    {
        QFETCH(bool, portal);
        QFETCH(QString, key);
        QObject *host = openLibrary(portal);
        QVERIFY(host);
        QVERIFY(goTo(key));
        QCOMPARE(eval(QStringLiteral("launcher.page")).toString(), key);
        eval(QStringLiteral("launcher.navigate(0, 1)"));
        eval(QStringLiteral("launcher.navigate(1, 0)"));
    }

    void homeShowsEverySectionWithContent()
    {
        QVERIFY(openLibrary(false));
        TRY_COMPARE(visibleSections(),
            QStringList({QStringLiteral("2"), QStringLiteral("1"), QStringLiteral("2"), QStringLiteral("2"), QStringLiteral("1")}));
        QVERIFY(page(QStringLiteral("greeting"))
                .toString()
                .contains(eval(QStringLiteral("launcherData.user.fullName || launcherData.user.loginName")).toString()));
    }

    void homeSectionsFollowTheSettings()
    {
        m_harness.respondWithLibrary();
        QVERIFY(m_harness.openHost(false,
            {{QStringLiteral("showRecentApps"), false}, {QStringLiteral("showRecentFiles"), false}, {QStringLiteral("showGames"), false},
                {QStringLiteral("showFriends"), false}}));
        TRY_COMPARE(visibleSections(), QStringList {QStringLiteral("2")});
        m_harness.config()->insert(QStringLiteral("showRecentFiles"), true);
        TRY_COMPARE(visibleSections(), QStringList({QStringLiteral("2"), QStringLiteral("1")}));
    }

    void homeWithoutPinsExplainsPinning()
    {
        QVERIFY(m_harness.openHost(true));
        eval(QStringLiteral("launcherData.favorites.clear()"));
        TRY_COMPARE(page(QStringLiteral("sections[0].count")).toInt(), 0);
        QCOMPARE(page(QStringLiteral("sections[0].shownCount")).toInt(), 0);
    }

    void homeOpensAFolderPanel()
    {
        QVERIFY(m_harness.openHost(false));
        eval(QStringLiteral(
            "launcherData.folders = [{ id: 'work', name: 'Work', apps: ['org.kde.konsole.desktop', 'org.kde.dolphin.desktop'] }]"));
        TRY_COMPARE(page(QStringLiteral("sections[0].count")).toInt(), 1);
        eval(QStringLiteral("launcher.toggleFolder('work')"));
        TRY_COMPARE(page(QStringLiteral("sections[1].count")).toInt(), 2);
        QVERIFY(page(QStringLiteral("sections[1].visible")).toBool());
        eval(QStringLiteral("launcher.toggleFolder('work')"));
        TRY_VERIFY(!page(QStringLiteral("sections[1].visible")).toBool());
    }

    void appsListTheCategoryAndRememberIt()
    {
        QVERIFY(m_harness.openHost(false, {{QStringLiteral("appsCategory"), QStringLiteral("Utilities")}}));
        QVERIFY(goTo(QStringLiteral("apps")));
        TRY_COMPARE(page(QStringLiteral("categoryRow")).toInt(), 1);
        TRY_COMPARE(page(QStringLiteral("activeGroup.count")).toInt(), 1);
        page(QStringLiteral("cycle(true)"));
        QCOMPARE(page(QStringLiteral("categoryRow")).toInt(), 0);
        QCOMPARE(m_harness.config()->value(QStringLiteral("appsCategory")).toString(), QStringLiteral("All Applications"));
        TRY_COMPARE(page(QStringLiteral("activeGroup.count")).toInt(), 4);
    }

    void appsSortOrders_data()
    {
        QTest::addColumn<QString>("sort");
        QTest::addColumn<QString>("first");
        QTest::newRow("name") << QStringLiteral("name") << QStringLiteral("Dolphin");
        QTest::newRow("recent") << QStringLiteral("recent") << QStringLiteral("Firefox");
        QTest::newRow("installed") << QStringLiteral("installed") << QStringLiteral("Firefox");
        QTest::newRow("popular") << QStringLiteral("popular") << QStringLiteral("Firefox");
    }

    void appsSortOrders()
    {
        QFETCH(QString, sort);
        QFETCH(QString, first);
        QVERIFY(m_harness.openHost(false, {{QStringLiteral("appsSort"), sort}}));
        QVERIFY(goTo(QStringLiteral("apps")));
        TRY_COMPARE(page(QStringLiteral("activeGroup.count")).toInt(), 4);
        TRY_COMPARE(page(QStringLiteral("activeGroup.get(0).model.display")).toString(), first);
        TRY_COMPARE(page(QStringLiteral("letters.length")).toInt(), sort == QLatin1String("name") ? 27 : 0);
    }

    void appsSortSwitchesBackToNameOrder()
    {
        QVERIFY(m_harness.openHost(false));
        QVERIFY(goTo(QStringLiteral("apps")));
        const QString labels
            = QStringLiteral("(g => { const out = []; for (let i = 0; i < g.count; ++i) out.push(g.get(i).model.display); return out })"
                             "(launcher.currentView().activeGroup)");
        const QStringList byName
            = {QStringLiteral("Dolphin"), QStringLiteral("Firefox"), QStringLiteral("Konsole"), QStringLiteral("Portal 2")};
        const QStringList newFirst
            = {QStringLiteral("Firefox"), QStringLiteral("Dolphin"), QStringLiteral("Konsole"), QStringLiteral("Portal 2")};
        TRY_COMPARE(eval(labels).toStringList(), byName);
        for (const char *sort : {"installed", "name", "recent", "name", "popular", "installed", "name"}) {
            m_harness.config()->insert(QStringLiteral("appsSort"), QString::fromLatin1(sort));
            TRY_COMPARE(eval(labels).toStringList(), qstrcmp(sort, "name") == 0 ? byName : newFirst);
            TRY_COMPARE(page(QStringLiteral("letters.filter(l => l.row >= 0).map(l => l.key + l.row)")).toStringList(),
                qstrcmp(sort, "name") == 0
                    ? QStringList({QStringLiteral("D0"), QStringLiteral("F1"), QStringLiteral("K2"), QStringLiteral("P3")})
                    : QStringList());
        }
    }

    void appsHideHiddenAppsAndSwitchViews()
    {
        QVERIFY(m_harness.openHost(false));
        QVERIFY(goTo(QStringLiteral("apps")));
        TRY_COMPARE(page(QStringLiteral("activeGroup.count")).toInt(), 4);
        eval(QStringLiteral("launcherData.setHidden('org.kde.konsole.desktop', true)"));
        TRY_COMPARE(page(QStringLiteral("activeGroup.count")).toInt(), 3);
        QCOMPARE(page(QStringLiteral("hiddenShown")).toInt(), 1);
        m_harness.config()->insert(QStringLiteral("appsView"), QStringLiteral("list"));
        TRY_VERIFY(page(QStringLiteral("listView")).toBool());
        TRY_COMPARE(page(QStringLiteral("activeGroup.count")).toInt(), 3);
        eval(QStringLiteral("launcherData.setHidden('org.kde.konsole.desktop', false)"));
        TRY_COMPARE(page(QStringLiteral("activeGroup.count")).toInt(), 4);
    }

    void appsZoomIsClamped()
    {
        QVERIFY(m_harness.openHost(false, {{QStringLiteral("tileSize"), 104}}));
        QVERIFY(goTo(QStringLiteral("apps")));
        page(QStringLiteral("zoom(3)"));
        QCOMPARE(m_harness.config()->value(QStringLiteral("tileSize")).toInt(), 112);
        page(QStringLiteral("zoom(-20)"));
        QCOMPARE(m_harness.config()->value(QStringLiteral("tileSize")).toInt(), 32);
    }

    void appsLaunchFromTheGrid()
    {
        QObject *host = m_harness.openHost(false);
        QVERIFY(host);
        QVERIFY(goTo(QStringLiteral("apps")));
        TRY_VERIFY(page(QStringLiteral("sections[0].shownCount")).toInt() > 0);
        eval(QStringLiteral("launcher.resetSelection()"));
        eval(QStringLiteral("launcher.activateCurrent()"));
        QCOMPARE(host->property("hideCount").toInt(), 1);
        QCOMPARE(page(QStringLiteral("categoryModel.triggered.length")).toInt(), 1);
    }

    void filesListPlacesWithUrlsAndRecentItems()
    {
        QVERIFY(m_harness.openHost(false));
        QVERIFY(goTo(QStringLiteral("files")));
        TRY_COMPARE(page(QStringLiteral("sections.map(s => s.count)")).toStringList(),
            QStringList({QStringLiteral("2"), QStringLiteral("1"), QStringLiteral("1")}));
        page(QStringLiteral("sections[0].currentIndex = 0"));
        eval(QStringLiteral("launcher.select(launcher.currentView().sections[0], 0)"));
        eval(QStringLiteral("launcher.activateCurrent()"));
        QCOMPARE(eval(QStringLiteral("launcherData.places.triggered[0].row")).toInt(), 0);
    }

    void filesSearchButtonPrefillsTheQuery()
    {
        QVERIFY(m_harness.openHost(false));
        QVERIFY(goTo(QStringLiteral("files")));
        eval(QStringLiteral("launcher.setQuery('f ')"));
        QCOMPARE(eval(QStringLiteral("launcher.mode")).toString(), QStringLiteral("files"));
    }

    void systemAsksTwiceBeforeLeavingTheSession()
    {
        QObject *host = m_harness.openHost(false);
        QVERIFY(host);
        QVERIFY(goTo(QStringLiteral("system")));
        const QString shutdown = QStringLiteral("launcher.currentView().sections[0].itemAtIndex(4)");
        TRY_VERIFY(eval(shutdown + QStringLiteral(" !== null")).toBool());
        eval(shutdown + QStringLiteral(".activate()"));
        QCOMPARE(page(QStringLiteral("armed")).toString(), QStringLiteral("shutdown"));
        QCOMPARE(host->property("hideCount").toInt(), 0);
        QVERIFY(eval(shutdown + QStringLiteral(".label")).toString().startsWith(QStringLiteral("Press again")));
        eval(shutdown + QStringLiteral(".activate()"));
        QCOMPARE(host->property("hideCount").toInt(), 1);
        QCOMPARE(eval(QStringLiteral("launcherData.system.triggered[0].favoriteId")).toString(), QStringLiteral("shutdown"));
        eval(QStringLiteral("launcher.currentView().sections[0].itemAtIndex(0).activate()"));
        QCOMPARE(eval(QStringLiteral("launcherData.system.triggered.length")).toInt(), 2);
    }

    void systemDisarmsAfterAWhile()
    {
        QVERIFY(m_harness.openHost(false));
        QVERIFY(goTo(QStringLiteral("system")));
        TRY_VERIFY(eval(QStringLiteral("launcher.currentView().sections[0].itemAtIndex(1) !== null")).toBool());
        eval(QStringLiteral("launcher.currentView().sections[0].itemAtIndex(1).activate()"));
        QCOMPARE(page(QStringLiteral("armed")).toString(), QStringLiteral("logout"));
        TRY_COMPARE(page(QStringLiteral("armed")).toString(), QString());
        QVERIFY(!page(QStringLiteral("kernel")).toString().isEmpty());
    }

    void systemLinksRunCommandsOrOpenSettings_data()
    {
        QTest::addColumn<int>("index");
        QTest::addColumn<QString>("command");
        QTest::addColumn<int>("configured");
        QTest::addColumn<QString>("pageAfter");
        QTest::newRow("System Settings") << 0 << QStringLiteral("systemsettings") << 0 << QStringLiteral("system");
        QTest::newRow("Konveyor") << 1 << QString() << 0 << QStringLiteral("settings");
        QTest::newRow("Launcher settings") << 2 << QString() << 1 << QStringLiteral("system");
        QTest::newRow("Displays") << 3 << QStringLiteral("kcmshell6 kcm_kscreen") << 0 << QStringLiteral("system");
    }

    void systemLinksRunCommandsOrOpenSettings()
    {
        QFETCH(int, index);
        QFETCH(QString, command);
        QFETCH(int, configured);
        QFETCH(QString, pageAfter);
        QObject *host = m_harness.openHost(false);
        QVERIFY(host);
        QVERIFY(goTo(QStringLiteral("system")));
        const QString link = QStringLiteral("launcher.currentView().sections[1].itemAtIndex(%1)").arg(index);
        TRY_VERIFY(eval(link + QStringLiteral(" !== null")).toBool());
        eval(link + QStringLiteral(".activate()"));
        QCOMPARE(host->property("configureCount").toInt(), configured);
        QCOMPARE(m_harness.commands().contains(command), !command.isEmpty());
        QCOMPARE(eval(QStringLiteral("launcher.page")).toString(), pageAfter);
    }
};

LAUNCHER_TEST_MAIN(TestLauncherPagesQml)
#include "test_launcher_pages_qml.moc"
