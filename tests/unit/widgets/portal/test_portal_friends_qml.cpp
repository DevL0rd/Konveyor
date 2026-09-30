#include "portalharness.h"

#include <QAbstractItemModel>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>

using Konveyor::Test::PortalHarness;
using Konveyor::Test::portalWarnings;

namespace
{

QJsonObject friendJson(const QString &id, const QString &name, int state, const QString &appid = {}, const QString &game = {})
{
    return {{QStringLiteral("steamid"), id}, {QStringLiteral("name"), name}, {QStringLiteral("state"), state},
        {QStringLiteral("ingame"), !appid.isEmpty() || !game.isEmpty()}, {QStringLiteral("appid"), appid}, {QStringLiteral("game"), game},
        {QStringLiteral("chat"), QStringLiteral("steam://friends/message/") + id}};
}

QString snapshot(const QJsonArray &friends, bool ok = true, const QString &error = {})
{
    return QString::fromUtf8(
        QJsonDocument(QJsonObject {{QStringLiteral("ok"), ok}, {QStringLiteral("error"), error}, {QStringLiteral("friends"), friends}})
            .toJson(QJsonDocument::Compact));
}

const QJsonArray crowd {
    friendJson(QStringLiteral("1"), QStringLiteral("Zed"), 1),
    friendJson(QStringLiteral("2"), QStringLiteral("amy"), 0),
    friendJson(QStringLiteral("3"), QStringLiteral("Bob"), 1, QStringLiteral("570"), QStringLiteral("Dota 2")),
    friendJson(QStringLiteral("4"), QStringLiteral("Cat"), 3),
    friendJson(QStringLiteral("5"), QStringLiteral("Dan"), 1, QStringLiteral("570"), QStringLiteral("Dota 2")),
    friendJson(QStringLiteral("6"), QStringLiteral("Eve"), 1, QStringLiteral("730"), QStringLiteral("Counter-Strike 2")),
};

bool writeSnapshot(const QString &path, const QString &text)
{
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(text.toUtf8()) == text.toUtf8().size();
}

}

class TestPortalFriendsQml : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init()
    {
        m_harness = std::make_unique<PortalHarness>();
        QVERIFY(m_harness->load(QStringLiteral("org.devl0rd.portal.friends"), PortalHarness::Planar));
        QCOMPARE(portalWarnings().join(QLatin1Char('\n')), QString());
    }

    void cleanup() { m_harness.reset(); }

    void findsItsSnapshotThroughTheRuntimeDir()
    {
        const QStringList requested = m_harness->requested();
        QCOMPARE(requested, QStringList {QStringLiteral("printf %s \"$XDG_RUNTIME_DIR/Plasma-App-Portal/friends.json\"")});
    }

    void aMissingSnapshotSaysTheCollectorIsNotRunning()
    {
        answerPath(m_harness->runtimeFile(QStringLiteral("missing.json")));
        QTRY_COMPARE_WITH_TIMEOUT(root()->property("error").toString(), QStringLiteral("No snapshot — is the collector running?"), 30000);
    }

    void aRewrittenSnapshotIsPickedUp()
    {
        const QString path = m_harness->runtimeFile(QStringLiteral("friends.json"));
        QVERIFY(writeSnapshot(path, snapshot({crowd.at(0)})));
        answerPath(path);
        QTRY_COMPARE_WITH_TIMEOUT(root()->property("friends").toList().size(), 1, 30000);
        QVERIFY(writeSnapshot(path + QStringLiteral(".tmp"), snapshot(crowd)));
        QVERIFY(QFile::remove(path));
        QVERIFY(QFile::rename(path + QStringLiteral(".tmp"), path));
        QTRY_COMPARE_WITH_TIMEOUT(root()->property("friends").toList().size(), 6, 30000);
    }

    void aSnapshotFillsTheListAndTheCounts()
    {
        process(snapshot(crowd));
        QCOMPARE(root()->property("error").toString(), QString());
        QVERIFY(root()->property("ready").toBool());
        QCOMPARE(root()->property("onlineCount").toInt(), 5);
        QCOMPARE(root()->property("inGameCount").toInt(), 3);
        QCOMPARE(rows(),
            QStringList({QStringLiteral("3:In Game"), QStringLiteral("5:In Game"), QStringLiteral("6:In Game"), QStringLiteral("4:Online"),
                QStringLiteral("1:Online"), QStringLiteral("2:Offline")}));
        QCOMPARE(root()->property("toolTipSubText").toString(),
            QStringLiteral("5 online · 3 in game · 6 friends\nDota 2: Bob, Dan\nCounter-Strike 2: Eve"));
    }

    void playingNowGroupsByGameBiggestFirst()
    {
        process(snapshot(crowd));
        const QVariantList groups = root()->property("playingNow").toList();
        QCOMPARE(groups.size(), 2);
        QCOMPARE(groups.at(0).toMap().value(QStringLiteral("game")).toString(), QStringLiteral("Dota 2"));
        QCOMPARE(groups.at(0).toMap().value(QStringLiteral("friends")).toList().size(), 2);
        QCOMPARE(groups.at(1).toMap().value(QStringLiteral("appid")).toString(), QStringLiteral("730"));
    }

    void badOrFailedSnapshotsShowAnError()
    {
        process(QStringLiteral("{nope"));
        QCOMPARE(root()->property("error").toString(), QStringLiteral("Could not read friends data"));
        process(snapshot({}, false, QStringLiteral("no steam_api_key in config")));
        QCOMPARE(root()->property("error").toString(), QStringLiteral("no steam_api_key in config"));
        QCOMPARE(root()->property("toolTipSubText").toString(), QStringLiteral("no steam_api_key in config"));
        process(snapshot({}, false));
        QCOMPARE(root()->property("error").toString(), QStringLiteral("No data"));
        process(snapshot(crowd));
        QCOMPARE(root()->property("error").toString(), QString());
    }

    void tabsFilterTheList_data()
    {
        QTest::addColumn<QString>("tab");
        QTest::addColumn<QStringList>("expected");
        QTest::newRow("in game") << "ingame"
                                 << QStringList {QStringLiteral("5:Favourites"), QStringLiteral("3:In Game"), QStringLiteral("6:In Game")};
        QTest::newRow("online") << "online"
                                << QStringList {QStringLiteral("5:Favourites"), QStringLiteral("3:In Game"), QStringLiteral("6:In Game"),
                                       QStringLiteral("4:Online"), QStringLiteral("1:Online")};
        QTest::newRow("favourites") << "favorites" << QStringList {QStringLiteral("5:In Game"), QStringLiteral("2:Offline")};
    }

    void tabsFilterTheList()
    {
        QFETCH(QString, tab);
        QFETCH(QStringList, expected);
        m_harness->config()->insert(QStringLiteral("favorites"), QStringLiteral("2,5"));
        process(snapshot(crowd));
        root()->setProperty("tabKey", tab);
        QCOMPARE(rows(), expected);
        QCOMPARE(m_harness->config()->value(QStringLiteral("currentTab")).toString(), tab);
    }

    void favouritesComeFirstOutsideTheirTab()
    {
        process(snapshot(crowd));
        m_harness->call("toggleFavorite", QStringLiteral("2"));
        QCOMPARE(m_harness->config()->value(QStringLiteral("favorites")).toString(), QStringLiteral("2"));
        QCOMPARE(rows().first(), QStringLiteral("2:Favourites"));
        QCOMPARE(root()->property("favoriteCount").toInt(), 1);
        QVERIFY(m_harness->call("isFavorite", 2).toBool());
        m_harness->call("toggleFavorite", QStringLiteral("2"));
        QCOMPARE(m_harness->config()->value(QStringLiteral("favorites")).toString(), QString());
        QCOMPARE(rows().last(), QStringLiteral("2:Offline"));
    }

    void hideOfflineAndSortByNameDescending()
    {
        process(snapshot(crowd));
        m_harness->config()->insert(QStringLiteral("hideOffline"), true);
        m_harness->config()->insert(QStringLiteral("sortMode"), QStringLiteral("name_desc"));
        QCOMPARE(rows(),
            QStringList({QStringLiteral("6:In Game"), QStringLiteral("5:In Game"), QStringLiteral("3:In Game"), QStringLiteral("1:Online"),
                QStringLiteral("4:Online")}));
    }

    void searchMatchesNamesAndGamesAcrossTabs()
    {
        process(snapshot(crowd));
        root()->setProperty("tabKey", QStringLiteral("ingame"));
        root()->setProperty("searchText", QStringLiteral("  AMY "));
        QCOMPARE(rows(), QStringList {QStringLiteral("2:Offline")});
        root()->setProperty("searchText", QStringLiteral("counter"));
        QCOMPARE(rows(), QStringList {QStringLiteral("6:In Game")});
        root()->setProperty("searchText", QString());
        QCOMPARE(rows().size(), 3);
    }

    void stateNamesAndColours_data()
    {
        QTest::addColumn<int>("state");
        QTest::addColumn<QString>("game");
        QTest::addColumn<QString>("text");
        QTest::addColumn<QString>("colour");
        QTest::newRow("offline") << 0 << "" << "Offline" << "#6a6a6a";
        QTest::newRow("online") << 1 << "" << "Online" << "#57cbde";
        QTest::newRow("busy") << 2 << "" << "Busy" << "#7e9bb5";
        QTest::newRow("away") << 3 << "" << "Away" << "#7e9bb5";
        QTest::newRow("snooze") << 4 << "" << "Snooze" << "#7e9bb5";
        QTest::newRow("looking to trade") << 5 << "" << "Online" << "#57cbde";
        QTest::newRow("in game") << 1 << "Dota 2" << "Dota 2" << "#90ba3c";
    }

    void stateNamesAndColours()
    {
        QFETCH(int, state);
        QFETCH(QString, game);
        QFETCH(QString, text);
        QFETCH(QString, colour);
        const QVariantMap person = friendJson(QStringLiteral("9"), QStringLiteral("x"), state, QString(), game).toVariantMap();
        QCOMPARE(m_harness->call("stateText", person).toString(), text);
        QCOMPARE(m_harness->call("stateColor", person).value<QColor>(), QColor(colour));
        QCOMPARE(m_harness->call("isOnline", person).toBool(), state != 0 || !game.isEmpty());
    }

    void lastOnlineIsRelative_data()
    {
        QTest::addColumn<qint64>("ago");
        QTest::addColumn<QString>("text");
        QTest::newRow("seconds") << qint64(5) << "just now";
        QTest::newRow("minutes") << qint64(5 * 60 + 30) << "5m ago";
        QTest::newRow("hours") << qint64(3 * 3600 + 5) << "3h ago";
        QTest::newRow("days") << qint64(2 * 86400 + 5) << "2d ago";
        QTest::newRow("years") << qint64(31536000 + 86400) << "1y ago";
    }

    void lastOnlineIsRelative()
    {
        QFETCH(qint64, ago);
        QFETCH(QString, text);
        QVariantMap person = friendJson(QStringLiteral("9"), QStringLiteral("x"), 0).toVariantMap();
        person.insert(QStringLiteral("lastlogoff"), QDateTime::currentSecsSinceEpoch() - ago);
        QCOMPARE(m_harness->call("lastOnlineText", person).toString(), text);
        person.insert(QStringLiteral("state"), 1);
        QCOMPARE(m_harness->call("lastOnlineText", person).toString(), QString());
        person.insert(QStringLiteral("state"), 0);
        person.insert(QStringLiteral("lastlogoff"), 0);
        QCOMPARE(m_harness->call("lastOnlineText", person).toString(), QString());
    }

    void countryFlags()
    {
        QCOMPARE(m_harness->call("flagEmoji", QStringLiteral("us")).toString(), QStringLiteral("\U0001F1FA\U0001F1F8"));
        for (const QString &bad : {QString(), QStringLiteral("U"), QStringLiteral("USA"), QStringLiteral("U1")}) {
            QCOMPARE(m_harness->call("flagEmoji", bad).toString(), QString());
        }
    }

    void savingAKeyRunsPortalFriendsWithTheKeyQuoted()
    {
        m_harness->call("saveKey", QStringLiteral("  "));
        QVERIFY(!root()->property("saving").toBool());
        m_harness->call("saveKey", QStringLiteral(" ab'cd "));
        QVERIFY(root()->property("saving").toBool());
        const QString command
            = QStringLiteral("$HOME/.local/bin/portal-friends --set-key 'ab'\\''cd' && systemctl --user restart konveyor-widgets.service");
        QVERIFY(m_harness->requested().contains(command));
        QVERIFY(PortalHarness::answer(runner(), command, QStringLiteral("ok\n")));
        QVERIFY(!root()->property("saving").toBool());
        QCOMPARE(root()->property("error").toString(), QString());
    }

    void aRejectedKeyShowsWhy()
    {
        m_harness->call("saveKey", QStringLiteral("bad"));
        const QString command
            = QStringLiteral("$HOME/.local/bin/portal-friends --set-key 'bad' && systemctl --user restart konveyor-widgets.service");
        const QVariantMap failed {{QStringLiteral("exit code"), 1},
            {QStringLiteral("stdout"), QStringLiteral("HTTP Error 403: Forbidden\n")}, {QStringLiteral("stderr"), QString()}};
        QVERIFY(QMetaObject::invokeMethod(runner(), "newData", Q_ARG(QString, command), Q_ARG(QVariant, QVariant(failed))));
        QVERIFY(!root()->property("saving").toBool());
        QCOMPARE(root()->property("error").toString(), QStringLiteral("HTTP Error 403: Forbidden"));

        m_harness->call("saveKey", QStringLiteral("bad"));
        const QVariantMap silent {{QStringLiteral("exit code"), 127}};
        QVERIFY(QMetaObject::invokeMethod(runner(), "newData", Q_ARG(QString, command), Q_ARG(QVariant, QVariant(silent))));
        QCOMPARE(root()->property("error").toString(), QStringLiteral("Could not save the Steam Web API key"));
    }

    void theRepresentationsLoadWithoutWarnings_data()
    {
        QTest::addColumn<QByteArray>("representation");
        QTest::addColumn<bool>("withFriends");
        QTest::newRow("compact") << QByteArray("compactRepresentation") << true;
        QTest::newRow("full") << QByteArray("fullRepresentation") << true;
        QTest::newRow("full before setup") << QByteArray("fullRepresentation") << false;
    }

    void theRepresentationsLoadWithoutWarnings()
    {
        QFETCH(QByteArray, representation);
        QFETCH(bool, withFriends);
        process(withFriends ? snapshot(crowd) : snapshot({}, false, QStringLiteral("no steam_api_key in config")));
        auto *component = root()->property(representation.constData()).value<QQmlComponent *>();
        std::unique_ptr<QObject> view(component->create(qmlContext(root())));
        QVERIFY2(view, qPrintable(component->errorString()));
        if (auto *item = qobject_cast<QQuickItem *>(view.get())) {
            item->setSize(QSizeF(600, 500));
        }
        QCoreApplication::processEvents();
        QCOMPARE(portalWarnings().join(QLatin1Char('\n')), QString());
    }

    void chatOpensSteamAndClosesThePopup()
    {
        root()->setProperty("expanded", true);
        m_harness->call("openChat", friendJson(QStringLiteral("3"), QStringLiteral("Bob"), 1).toVariantMap());
        QVERIFY(m_harness->requested().contains(QStringLiteral("steam 'steam://friends/message/3'")));
        QVERIFY(!root()->property("expanded").toBool());
    }

private:
    QObject *root() const { return m_harness->root(); }

    void answerPath(const QString &path) const
    {
        QObject *helper = m_harness->dataSources().first();
        QVERIFY(PortalHarness::answer(helper, helper->property("requested").toStringList().value(0), path));
    }

    QObject *runner() const { return m_harness->dataSources().last(); }

    void process(const QString &text) const { m_harness->call("processSnapshot", text); }

    QStringList rows() const
    {
        const auto *model = qobject_cast<QAbstractItemModel *>(root()->property("rows").value<QObject *>());
        const int steamid = model->roleNames().key("steamid");
        const int section = model->roleNames().key("section");
        QStringList result;
        for (int row = 0; row < model->rowCount(); ++row) {
            const QModelIndex index = model->index(row, 0);
            result.append(model->data(index, steamid).toString() + QLatin1Char(':') + model->data(index, section).toString());
        }
        return result;
    }

    std::unique_ptr<PortalHarness> m_harness;
};

QTEST_MAIN(TestPortalFriendsQml)
#include "test_portal_friends_qml.moc"
