#include "portalharness.h"

#include <QTest>

using Konveyor::Test::FakePlasmoidAttached;
using Konveyor::Test::PortalHarness;
using Konveyor::Test::portalWarnings;

namespace
{

bool writeFile(const QString &path, const QByteArray &contents)
{
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(contents) == contents.size();
}

}

class TestPortalPlasmoidQml : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void loadsOnTheDesktopAndInAPanel_data()
    {
        QTest::addColumn<int>("formFactor");
        QTest::addColumn<bool>("inPanel");
        QTest::newRow("desktop") << int(PortalHarness::Planar) << false;
        QTest::newRow("horizontal panel") << int(PortalHarness::Horizontal) << true;
        QTest::newRow("vertical panel") << int(PortalHarness::Vertical) << true;
    }

    void loadsOnTheDesktopAndInAPanel()
    {
        QFETCH(int, formFactor);
        QFETCH(bool, inPanel);
        PortalHarness harness;
        QVERIFY(load(harness, PortalHarness::FormFactor(formFactor)));
        QObject *root = harness.root();
        QCOMPARE(root->property("inPanel").toBool(), inPanel);
        QCOMPARE(root->property("popupAlive").toBool(), !inPanel);
        QCOMPARE(root->property("preferredRepresentation").value<QObject *>(),
            root->property(inPanel ? "compactRepresentation" : "fullRepresentation").value<QObject *>());
        QCOMPARE(FakePlasmoidAttached::instance().icon, QStringLiteral("view-app-grid-symbolic"));
        QCOMPARE(FakePlasmoidAttached::instance().title, QStringLiteral("App Portal"));
        QCOMPARE(root->property("favoritesClient").toString(), QStringLiteral("org.kde.plasma.kicker.favorites.instance-7"));
        QCOMPARE(root->property("toolTipSubText").toString(), QStringLiteral("Apps, games, files and friends"));
        QCOMPARE(portalWarnings().join(QLatin1Char('\n')), QString());
    }

    void aCustomIconReplacesTheDefault()
    {
        PortalHarness harness;
        QVERIFY(load(harness, PortalHarness::Planar, {{QStringLiteral("icon"), QStringLiteral("go-home")}}));
        QCOMPARE(FakePlasmoidAttached::instance().icon, QStringLiteral("go-home"));
    }

    void hideOnlyCollapsesAPanelPopup()
    {
        PortalHarness harness;
        QVERIFY(load(harness, PortalHarness::Horizontal));
        QObject *root = harness.root();
        root->setProperty("expanded", true);
        QVERIFY(root->property("open").toBool());
        QVERIFY(root->property("popupAlive").toBool());
        harness.call("hide");
        QVERIFY(!root->property("expanded").toBool());
        QVERIFY(root->property("popupAlive").toBool());
        QObject *release = releaseTimer(root);
        QVERIFY(release->property("running").toBool());
        QVERIFY(QMetaObject::invokeMethod(release, "triggered"));
        QVERIFY(!root->property("popupAlive").toBool());
        root->setProperty("expanded", true);
        QVERIFY(!release->property("running").toBool());
        QVERIFY(root->property("popupAlive").toBool());
    }

    void configureOpensThePlasmoidSettings()
    {
        PortalHarness harness;
        QVERIFY(load(harness, PortalHarness::Planar));
        harness.call("configure");
        QCOMPARE(FakePlasmoidAttached::instance().requestedActions, QStringList {QStringLiteral("configure")});
        QVERIFY(FakePlasmoidAttached::instance().action.triggered);
    }

    void migratesTheOldPortalSettingsOnce_data()
    {
        QTest::addColumn<QVariantMap>("old");
        QTest::addColumn<QVariantMap>("expected");
        const auto map = [](std::initializer_list<std::pair<const char *, QVariant>> pairs) {
            QVariantMap result;
            for (const auto &[key, value] : pairs) {
                result.insert(QString::fromLatin1(key), value);
            }
            return result;
        };
        QTest::newRow("defaults") << QVariantMap {} << map({{"defaultPage", "home"}, {"gamesView", "grid"}, {"gamesSort", "recent"}});
        QTest::newRow("games") << map({{"defaultCategory", "Games"}}) << map({{"defaultPage", "games"}});
        QTest::newRow("favorites") << map({{"defaultCategory", "Favorites"}}) << map({{"defaultPage", "home"}});
        QTest::newRow("a category") << map({{"defaultCategory", "Development"}})
                                    << map({{"defaultPage", "apps"}, {"appsCategory", "Development"}});
        QTest::newRow("cover flow") << map({{"gamesViewMode", "carousel3d"}}) << map({{"gamesView", "coverflow"}});
        QTest::newRow("banner") << map({{"gamesViewMode", "banner"}}) << map({{"gamesView", "banner"}});
        QTest::newRow("unknown view") << map({{"gamesViewMode", "wall"}}) << map({{"gamesView", "grid"}});
        QTest::newRow("app list") << map({{"appViewMode", "list"}}) << map({{"appsView", "list"}});
        QTest::newRow("sort by name") << map({{"defaultSort", "name_desc"}, {"gamesSort", "recent"}}) << map({{"gamesSort", "name"}});
        QTest::newRow("sort by recent") << map({{"defaultSort", "recent"}, {"gamesSort", "name"}}) << map({{"gamesSort", "recent"}});
        QTest::newRow("tiny cards") << map({{"gameCardWidth", 1}}) << map({{"gameCardSize", 6}});
        QTest::newRow("huge cards") << map({{"gameCardWidth", 100000}}) << map({{"gameCardSize", 18}});
        QTest::newRow("already migrated") << map({{"migratedFromPortal", true}, {"defaultCategory", "Games"}, {"defaultPage", "system"}})
                                          << map({{"defaultPage", "system"}});
    }

    void migratesTheOldPortalSettingsOnce()
    {
        QFETCH(QVariantMap, old);
        QFETCH(QVariantMap, expected);
        PortalHarness harness;
        QVERIFY(load(harness, PortalHarness::Planar, old));
        for (auto it = expected.cbegin(); it != expected.cend(); ++it) {
            QCOMPARE(harness.config()->value(it.key()).toString(), it.value().toString());
        }
        QVERIFY(harness.config()->value(QStringLiteral("migratedFromPortal")).toBool());
    }

    void cardWidthBecomesAGridUnitSize()
    {
        PortalHarness harness;
        QVERIFY(load(harness, PortalHarness::Planar, {{QStringLiteral("gameCardWidth"), 180}}));
        const int size = harness.config()->value(QStringLiteral("gameCardSize")).toInt();
        QVERIFY(size >= 6 && size <= 18);
    }

    void thePanelBadgeCountsFriendsInGame()
    {
        PortalHarness harness;
        QVERIFY(load(harness, PortalHarness::Horizontal));
        QObject *root = harness.root();
        QVERIFY(root->property("badgeWanted").toBool());
        QObject *path = pathHelper(harness);
        QVERIFY(path);
        const QString snapshot = harness.runtimeFile(QStringLiteral("friends.json"));
        QVERIFY(writeFile(snapshot, R"({"by_appid":{"570":[{"name":"a"},{"name":"b"}],"730":[{"name":"c"}]}})"));
        QVERIFY(PortalHarness::answer(path, path->property("connectedSources").toStringList().value(0), snapshot + QStringLiteral("\n")));
        QCOMPARE(root->property("friendsPath").toString(), snapshot);
        QVERIFY(path->property("connectedSources").toStringList().isEmpty());
        harness.call("readFriends");
        QTRY_COMPARE_WITH_TIMEOUT(root->property("friendsPlaying").toInt(), 3, 30000);
        QCOMPARE(root->property("toolTipSubText").toString(), QStringLiteral("3 friends in game"));

        QVERIFY(writeFile(snapshot, "{not json"));
        harness.call("readFriends");
        QVERIFY(writeFile(snapshot, R"({"by_appid":{"570":[{"name":"a"}]}})"));
        harness.call("readFriends");
        QTRY_COMPARE_WITH_TIMEOUT(root->property("friendsPlaying").toInt(), 1, 30000);
        QCOMPARE(root->property("toolTipSubText").toString(), QStringLiteral("1 friend in game"));
        QCOMPARE(portalWarnings().join(QLatin1Char('\n')), QString());
    }

    void theBadgeIsOnlyWantedInAPanelWithFriendsShown_data()
    {
        QTest::addColumn<int>("formFactor");
        QTest::addColumn<bool>("badge");
        QTest::addColumn<bool>("friends");
        QTest::addColumn<bool>("wanted");
        QTest::newRow("panel") << int(PortalHarness::Horizontal) << true << true << true;
        QTest::newRow("desktop") << int(PortalHarness::Planar) << true << true << false;
        QTest::newRow("badge off") << int(PortalHarness::Vertical) << false << true << false;
        QTest::newRow("friends off") << int(PortalHarness::Vertical) << true << false << false;
    }

    void theBadgeIsOnlyWantedInAPanelWithFriendsShown()
    {
        QFETCH(int, formFactor);
        QFETCH(bool, badge);
        QFETCH(bool, friends);
        QFETCH(bool, wanted);
        PortalHarness harness;
        QVERIFY(load(harness, PortalHarness::FormFactor(formFactor),
            {{QStringLiteral("showFriendsBadge"), badge}, {QStringLiteral("showFriends"), friends}}));
        QCOMPARE(harness.root()->property("badgeWanted").toBool(), wanted);
        QCOMPARE(pathHelper(harness)->property("connectedSources").toStringList().isEmpty(), !wanted);
    }

    void theCompactButtonShowsTheBadge()
    {
        PortalHarness harness;
        QVERIFY(load(harness, PortalHarness::Horizontal));
        QObject *root = harness.root();
        auto *component = root->property("compactRepresentation").value<QQmlComponent *>();
        std::unique_ptr<QObject> button(component->create(qmlContext(root)));
        QVERIFY2(button, qPrintable(component->errorString()));
        QVERIFY(!button->property("showBadge").toBool());
        root->setProperty("friendsPlaying", 12);
        QVERIFY(button->property("showBadge").toBool());
        harness.config()->insert(QStringLiteral("showFriendsBadge"), false);
        QVERIFY(!button->property("showBadge").toBool());
        QCOMPARE(portalWarnings().join(QLatin1Char('\n')), QString());
    }

private:
    static bool load(PortalHarness &harness, PortalHarness::FormFactor formFactor, const QVariantMap &settings = {})
    {
        harness.stub(QStringLiteral("LauncherView.qml"), "import QtQuick\nItem { property bool compact }\n");
        return harness.load(QStringLiteral("org.devl0rd.portal"), formFactor, settings);
    }

    static QObject *releaseTimer(QObject *root)
    {
        const QList<QObject *> children = root->findChildren<QObject *>();
        for (QObject *child : children) {
            if (child->property("interval").toInt() == 1500) {
                return child;
            }
        }
        return nullptr;
    }

    static QObject *pathHelper(const PortalHarness &harness)
    {
        const QList<QObject *> sources = harness.dataSources();
        return sources.isEmpty() ? nullptr : sources.first();
    }
};

QTEST_MAIN(TestPortalPlasmoidQml)
#include "test_portal_plasmoid_qml.moc"
