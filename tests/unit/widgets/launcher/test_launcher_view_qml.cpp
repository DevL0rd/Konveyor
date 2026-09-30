#include "launcherharness.h"

#include <QSignalSpy>

class TestLauncherViewQml : public LauncherTest::TestCase
{
    Q_OBJECT

private Q_SLOTS:
    void opensWithoutWarnings_data()
    {
        QTest::addColumn<bool>("portal");
        QTest::newRow("standalone") << false;
        QTest::newRow("App Portal") << true;
    }

    void opensWithoutWarnings()
    {
        QFETCH(bool, portal);
        QVERIFY(m_harness.openHost(portal));
        QCOMPARE(eval(QStringLiteral("launcher.compact")).toBool(), portal);
        QVERIFY(eval(QStringLiteral("launcher.shown")).toBool());
    }

    void openingShowsTheDefaultPageWithAnEmptyFocusedSearch()
    {
        QObject *host = m_harness.host(false, {{QStringLiteral("defaultPage"), QStringLiteral("apps")}});
        QVERIFY(host);
        QSignalSpy activated(m_harness.view(), SIGNAL(activateRequested()));
        host->setProperty("open", true);
        QCOMPARE(activated.count(), 1);
        QVERIFY(QTest::qWaitForWindowActive(m_harness.window()));
        QCOMPARE(eval(QStringLiteral("launcher.page")).toString(), QStringLiteral("apps"));
        QCOMPARE(host->property("currentPage").toString(), QStringLiteral("apps"));
        TRY_VERIFY(eval(QStringLiteral("field.activeFocus")).toBool());
        QCOMPARE(eval(QStringLiteral("field.text")).toString(), QString());
        TRY_COMPARE(m_harness.view()->property("contentProgress").toDouble(), 1.0);
    }

    void aRequestedPageWinsOnce()
    {
        QObject *host = m_harness.host(false);
        QVERIFY(host);
        host->setProperty("requestedPage", QStringLiteral("files"));
        host->setProperty("open", true);
        QCOMPARE(eval(QStringLiteral("launcher.page")).toString(), QStringLiteral("files"));
        QCOMPARE(host->property("requestedPage").toString(), QString());
        host->setProperty("open", false);
        TRY_VERIFY(!eval(QStringLiteral("launcher.shown")).toBool());
        host->setProperty("open", true);
        QCOMPARE(eval(QStringLiteral("launcher.page")).toString(), QStringLiteral("home"));
    }

    void aPageTheSettingsHideFallsBackToHome_data()
    {
        QTest::addColumn<QString>("page");
        QTest::addColumn<QString>("setting");
        QTest::newRow("games") << QStringLiteral("games") << QStringLiteral("showGames");
        QTest::newRow("friends") << QStringLiteral("friends") << QStringLiteral("showFriends");
        QTest::newRow("unknown") << QStringLiteral("nowhere") << QStringLiteral("showGames");
    }

    void aPageTheSettingsHideFallsBackToHome()
    {
        QFETCH(QString, page);
        QFETCH(QString, setting);
        QObject *host = m_harness.host(false, {{setting, false}, {QStringLiteral("defaultPage"), page}});
        QVERIFY(host);
        host->setProperty("open", true);
        QCOMPARE(eval(QStringLiteral("launcher.page")).toString(), QStringLiteral("home"));
        QVERIFY(!eval(QStringLiteral("launcher.pageDefs.map(d => d.key)")).toStringList().contains(page));
    }

    void pagesFollowTheSettings()
    {
        QVERIFY(m_harness.host(false));
        const QString keys = QStringLiteral("launcher.pageDefs.map(d => d.key).join(',')");
        QCOMPARE(eval(keys).toString(), QStringLiteral("home,apps,games,files,friends,system,shortcuts,settings"));
        m_harness.config()->insert(QStringLiteral("showGames"), false);
        QCOMPARE(eval(keys).toString(), QStringLiteral("home,apps,files,friends,system,shortcuts,settings"));
        m_harness.config()->insert(QStringLiteral("showFriends"), false);
        QCOMPARE(eval(keys).toString(), QStringLiteral("home,apps,files,system,shortcuts,settings"));
    }

    void closingRunsTheAnimationAndResets()
    {
        QObject *host = m_harness.openHost(false);
        QVERIFY(host);
        QSignalSpy finished(m_harness.view(), SIGNAL(closeFinished()));
        eval(QStringLiteral("field.text = 'kon'"));
        eval(QStringLiteral("launcher.railIndex = 0"));
        host->setProperty("open", false);
        QCOMPARE(eval(QStringLiteral("launcher.railIndex")).toInt(), -1);
        QVERIFY(eval(QStringLiteral("launcher.shown")).toBool());
        QVERIFY(finished.wait(30000));
        QCOMPARE(finished.count(), 1);
        QVERIFY(!eval(QStringLiteral("launcher.shown")).toBool());
        QCOMPARE(m_harness.view()->property("progress").toDouble(), 0.0);
        QCOMPARE(eval(QStringLiteral("field.text")).toString(), QString());
    }

    void reopeningWhileClosingCancelsTheClose()
    {
        QObject *host = m_harness.openHost(false);
        QVERIFY(host);
        QSignalSpy finished(m_harness.view(), SIGNAL(closeFinished()));
        host->setProperty("open", false);
        QVERIFY(eval(QStringLiteral("closeAnimation.running")).toBool());
        host->setProperty("open", true);
        QVERIFY(!eval(QStringLiteral("closeAnimation.running")).toBool());
        TRY_COMPARE(m_harness.view()->property("progress").toDouble(), 1.0);
        QVERIFY(!eval(QStringLiteral("closeAnimation.running")).toBool());
        QCOMPARE(finished.count(), 0);
        QVERIFY(eval(QStringLiteral("launcher.shown")).toBool());
    }

    void opensWhenTheHostStartsOpen()
    {
        const QString schema = LauncherTest::source("widgets/portals/kontrol-panel/config/main.xml");
        KConfigPropertyMap *config = m_harness.loadConfig(schema, QStringLiteral("config/kontrolpanelrc"));
        QVERIFY(m_harness.create(QStringLiteral("LauncherHost.qml"),
            {{QStringLiteral("config"), QVariant::fromValue<QObject *>(config)}, {QStringLiteral("portal"), false},
                {QStringLiteral("open"), true}}));
        TRY_VERIFY(eval(QStringLiteral("launcher.shown")).toBool());
    }

    void pendingPinsBecomeFavoritesOnce()
    {
        QObject *host = m_harness.host(false);
        QVERIFY(host);
        const QString path = QStringLiteral("/usr/share/applications/org.kde.kate.desktop");
        TRY_COMPARE(eval(QStringLiteral("launcherData.favorites.count")).toInt(), 2);
        host->setProperty("pendingPins", QStringList {path, path});
        QVERIFY(QMetaObject::invokeMethod(host, "pinsRequested"));
        QCOMPARE(eval(QStringLiteral("launcherData.favorites.count")).toInt(), 3);
        QVERIFY(eval(QStringLiteral("launcherData.favorites.isFavorite('%1')").arg(path)).toBool());
        QCOMPARE(host->property("pendingPins").toStringList(), QStringList());
        host->setProperty("pendingPins", QStringList {path});
        QVERIFY(QMetaObject::invokeMethod(host, "pinsRequested"));
        QCOMPARE(eval(QStringLiteral("launcherData.favorites.count")).toInt(), 3);
    }

    void searchPrefixesPickTheMode_data()
    {
        QTest::addColumn<QString>("text");
        QTest::addColumn<QString>("mode");
        QTest::addColumn<QString>("term");
        QTest::newRow("all") << QStringLiteral("  fire ") << QStringLiteral("all") << QStringLiteral("fire");
        QTest::newRow("games") << QStringLiteral("g portal") << QStringLiteral("games") << QStringLiteral("portal");
        QTest::newRow("files") << QStringLiteral("f notes") << QStringLiteral("files") << QStringLiteral("notes");
        QTest::newRow("apps") << QStringLiteral("a kon") << QStringLiteral("apps") << QStringLiteral("kon");
        QTest::newRow("packages") << QStringLiteral("s vim") << QStringLiteral("packages") << QStringLiteral("vim");
        QTest::newRow("friends") << QStringLiteral("@bob") << QStringLiteral("friends") << QStringLiteral("bob");
        QTest::newRow("calc") << QStringLiteral("=2+2") << QStringLiteral("calc") << QStringLiteral("2+2");
        QTest::newRow("command") << QStringLiteral(">ls -l") << QStringLiteral("command") << QStringLiteral("ls -l");
        QTest::newRow("no space after the letter") << QStringLiteral("gportal") << QStringLiteral("all") << QStringLiteral("gportal");
    }

    void searchPrefixesPickTheMode()
    {
        QFETCH(QString, text);
        QFETCH(QString, mode);
        QFETCH(QString, term);
        QVERIFY(m_harness.host(false));
        eval(QStringLiteral("field.text = '%1'").arg(text));
        QCOMPARE(eval(QStringLiteral("launcher.mode")).toString(), mode);
        QCOMPARE(eval(QStringLiteral("launcher.term")).toString(), term);
        QVERIFY(eval(QStringLiteral("launcher.searching")).toBool());
        TRY_VERIFY(eval(QStringLiteral("launcher.searchSettled")).toBool());
        QCOMPARE(eval(QStringLiteral("launcher.presentedMode")).toString(), mode);
    }

    void clearingTheSearchSettlesAtOnce()
    {
        QVERIFY(m_harness.host(false));
        eval(QStringLiteral("field.text = 'kon'"));
        QVERIFY(!eval(QStringLiteral("launcher.searchSettled")).toBool());
        eval(QStringLiteral("field.text = ''"));
        QVERIFY(eval(QStringLiteral("launcher.searchSettled")).toBool());
        QVERIFY(!eval(QStringLiteral("launcher.searching")).toBool());
    }

    void stepPageWraps()
    {
        QVERIFY(m_harness.openHost(false));
        eval(QStringLiteral("launcher.stepPage(-1)"));
        QCOMPARE(eval(QStringLiteral("launcher.page")).toString(), QStringLiteral("settings"));
        eval(QStringLiteral("launcher.stepPage(1)"));
        QCOMPARE(eval(QStringLiteral("launcher.page")).toString(), QStringLiteral("home"));
        QVERIFY(eval(QStringLiteral("launcher.visited.settings === true")).toBool());
    }
};

LAUNCHER_TEST_MAIN(TestLauncherViewQml)
#include "test_launcher_view_qml.moc"
