#include "launcherharness.h"

namespace
{

struct Pair
{
    const char *topBar;
    const char *elsewhere;
    const char *topBarItem;
    const char *elsewhereItem;
};

const QList<Pair> pairs {
    {"showTopBarFriends", "showSidebarFriends", "friendsPill", "sidebar:friends"},
    {"showTopBarClock", "showHomeDate", "clock", "homeDate"},
    {"showTopBarSettings", "showSidebarSettings", "settingsButton", "sidebar:settings"},
    {"showTopBarPower", "showSidebarSystem", "powerButton", "sidebar:system"},
};

const QStringList buttonKeys {QStringLiteral("showTopBarFriends"), QStringLiteral("showTopBarClock"), QStringLiteral("showTopBarSettings"),
    QStringLiteral("showTopBarPower"), QStringLiteral("showSidebarFriends"), QStringLiteral("showSidebarSystem"),
    QStringLiteral("showSidebarSettings"), QStringLiteral("showHomeDate")};

}

class TestLauncherButtonsQml : public LauncherTest::TestCase
{
    Q_OBJECT

private:
    QStringList sidebar()
    {
        return eval(QStringLiteral("rail.children.filter(c => c.modelData && c.visible).map(c => c.modelData.key)")).toStringList();
    }

    bool homeDateShown()
    {
        return eval(QStringLiteral("(function walk(i, out) { out.push(i); for (const c of i.children) walk(c, out); return out })"
                                   "(launcher.currentView(), []).some(c => c.text === launcher.currentView().today && c.visible)"))
            .toBool();
    }

    bool shown(const QString &item)
    {
        if (item.startsWith(QLatin1String("sidebar:"))) {
            return sidebar().contains(item.mid(8));
        }
        if (item == QLatin1String("homeDate")) {
            return homeDateShown();
        }
        return eval(item + QStringLiteral(".visible")).toBool();
    }

    QObject *openHome(bool portal, const QVariantMap &settings = {})
    {
        QObject *host = openLibrary(portal, settings);
        return host && goTo(QStringLiteral("home")) ? host : nullptr;
    }

private Q_SLOTS:
    void theDefaultsShowEveryButton()
    {
        QVERIFY(openHome(false));
        const QStringList shownInBothPlaces {QStringLiteral("showTopBarSettings"), QStringLiteral("showTopBarPower")};
        for (const Pair &pair : pairs) {
            const bool topBar = shown(QLatin1String(pair.topBarItem));
            const bool elsewhere = shown(QLatin1String(pair.elsewhereItem));
            QVERIFY2(shownInBothPlaces.contains(QLatin1String(pair.topBar)) ? topBar && elsewhere : topBar != elsewhere, pair.topBar);
        }
        QVERIFY(shown(QStringLiteral("clock")));
        QVERIFY(!shown(QStringLiteral("homeDate")));
        QCOMPARE(sidebar(),
            QStringList({QStringLiteral("home"), QStringLiteral("apps"), QStringLiteral("games"), QStringLiteral("files"),
                QStringLiteral("friends"), QStringLiteral("system"), QStringLiteral("shortcuts"), QStringLiteral("settings")}));
    }

    void theDefaultsAreStoredInKontrolpanelrc()
    {
        QVERIFY(m_harness.host(false));
        const QMap<QString, bool> expected {{QStringLiteral("showTopBarFriends"), false}, {QStringLiteral("showTopBarClock"), true},
            {QStringLiteral("showTopBarSettings"), true}, {QStringLiteral("showTopBarPower"), true},
            {QStringLiteral("showSidebarFriends"), true}, {QStringLiteral("showSidebarSystem"), true},
            {QStringLiteral("showSidebarSettings"), true}, {QStringLiteral("showHomeDate"), false}};
        for (auto it = expected.cbegin(); it != expected.cend(); ++it) {
            QCOMPARE(m_harness.config()->value(it.key()).toBool(), it.value());
        }
        eval(QStringLiteral("launcherData.config.showTopBarPower = false"));
        QFile file(m_harness.path(QStringLiteral("config/kontrolpanelrc")));
        QVERIFY(file.open(QIODevice::ReadOnly));
        QVERIFY(file.readAll().contains("showTopBarPower=false"));
    }

    void everyPairFollowsItsTwoSettings_data()
    {
        QTest::addColumn<int>("pair");
        QTest::addColumn<bool>("topBar");
        QTest::addColumn<bool>("elsewhere");
        for (int index = 0; index < pairs.size(); ++index) {
            for (const bool topBar : {false, true}) {
                for (const bool elsewhere : {false, true}) {
                    QTest::addRow("%s=%d %s=%d", pairs.at(index).topBar, topBar, pairs.at(index).elsewhere, elsewhere)
                        << index << topBar << elsewhere;
                }
            }
        }
    }

    void everyPairFollowsItsTwoSettings()
    {
        QFETCH(int, pair);
        QFETCH(bool, topBar);
        QFETCH(bool, elsewhere);
        const Pair &buttons = pairs.at(pair);
        QVERIFY(openHome(false, {{QLatin1String(buttons.topBar), topBar}, {QLatin1String(buttons.elsewhere), elsewhere}}));
        QCOMPARE(shown(QLatin1String(buttons.topBarItem)), topBar);
        QCOMPARE(shown(QLatin1String(buttons.elsewhereItem)), elsewhere);
    }

    void eachSettingOnlyMovesItsOwnButton_data()
    {
        QTest::addColumn<QString>("key");
        for (const QString &key : buttonKeys) {
            QTest::newRow(qPrintable(key)) << key;
        }
    }

    void eachSettingOnlyMovesItsOwnButton()
    {
        QFETCH(QString, key);
        QVERIFY(openHome(false));
        QMap<QString, bool> before;
        for (const Pair &pair : pairs) {
            before.insert(QLatin1String(pair.topBar), shown(QLatin1String(pair.topBarItem)));
            before.insert(QLatin1String(pair.elsewhere), shown(QLatin1String(pair.elsewhereItem)));
        }
        const bool flipped = !m_harness.config()->value(key).toBool();
        m_harness.config()->insert(key, flipped);
        for (const Pair &pair : pairs) {
            const QString topBar = QLatin1String(pair.topBar);
            const QString elsewhere = QLatin1String(pair.elsewhere);
            TRY_COMPARE(shown(QLatin1String(pair.topBarItem)), topBar == key ? flipped : before.value(topBar));
            TRY_COMPARE(shown(QLatin1String(pair.elsewhereItem)), elsewhere == key ? flipped : before.value(elsewhere));
        }
    }

    void turningFriendsOffHidesBothFriendsButtons()
    {
        QVERIFY(openHome(false, {{QStringLiteral("showTopBarFriends"), true}, {QStringLiteral("showSidebarFriends"), true}}));
        QVERIFY(shown(QStringLiteral("friendsPill")));
        m_harness.config()->insert(QStringLiteral("showFriends"), false);
        TRY_VERIFY(!shown(QStringLiteral("friendsPill")));
        QVERIFY(!sidebar().contains(QStringLiteral("friends")));
    }

    void theFriendsPillWaitsForFriends()
    {
        QVERIFY(m_harness.openHost(false, {{QStringLiteral("showTopBarFriends"), true}}));
        QVERIFY(!shown(QStringLiteral("friendsPill")));
    }

    void aPageWithoutItsSidebarButtonStillOpens_data()
    {
        QTest::addColumn<QString>("key");
        QTest::addColumn<QString>("page");
        QTest::newRow("friends") << QStringLiteral("showSidebarFriends") << QStringLiteral("friends");
        QTest::newRow("system") << QStringLiteral("showSidebarSystem") << QStringLiteral("system");
        QTest::newRow("settings") << QStringLiteral("showSidebarSettings") << QStringLiteral("settings");
    }

    void aPageWithoutItsSidebarButtonStillOpens()
    {
        QFETCH(QString, key);
        QFETCH(QString, page);
        QVERIFY(openLibrary(false, {{key, false}}));
        QVERIFY(!sidebar().contains(page));
        const int at = eval(QStringLiteral("launcher.pageDefs.findIndex(d => d.key === '%1')").arg(page)).toInt();
        QVERIFY(at >= 0);
        QTest::keyClick(m_harness.window(), static_cast<Qt::Key>(Qt::Key_1 + at), Qt::AltModifier);
        TRY_COMPARE(eval(QStringLiteral("launcher.page")).toString(), page);
    }

    void thePowerButtonOpensTheSessionMenu()
    {
        QVERIFY(m_harness.openHost(false, {{QStringLiteral("showTopBarPower"), true}}));
        eval(QStringLiteral("powerButton.clicked()"));
        const QStringList entries = eval(QStringLiteral("menu.entries.map(e => e.text || '')")).toStringList();
        QCOMPARE(entries.last(), QStringLiteral("Session page"));
        eval(QStringLiteral("menu.entries[menu.entries.length - 1].run()"));
        QCOMPARE(eval(QStringLiteral("launcher.page")).toString(), QStringLiteral("system"));
    }

    void theSettingsButtonOpensSystemSettings()
    {
        QVERIFY(m_harness.openHost(false, {{QStringLiteral("showTopBarSettings"), true}}));
        eval(QStringLiteral("settingsButton.clicked()"));
        QVERIFY(m_harness.commands().contains(QStringLiteral("systemsettings")));
    }

    void theAppPortalKeepsItsCompactTopBar()
    {
        QVariantMap everything;
        for (const QString &key : buttonKeys) {
            everything.insert(key, true);
        }
        QVERIFY(openHome(true, everything));
        QVERIFY(shown(QStringLiteral("friendsPill")));
        QVERIFY(shown(QStringLiteral("settingsButton")));
        QVERIFY(!shown(QStringLiteral("clock")));
        QVERIFY(!shown(QStringLiteral("powerButton")));
        QVERIFY(!shown(QStringLiteral("homeDate")));
        QVERIFY(sidebar().contains(QStringLiteral("system")));
    }
};

LAUNCHER_TEST_MAIN(TestLauncherButtonsQml)
#include "test_launcher_buttons_qml.moc"
