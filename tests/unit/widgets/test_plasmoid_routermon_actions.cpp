#include "routerfixture.h"

using Router::clientMacs;
using Router::feed;
using Router::runtime;
using Router::shell;
using Router::started;

namespace
{

const QString ctl = QStringLiteral("$HOME/.local/bin/routermon-ctl");
const QString speedtest = QStringLiteral("$HOME/.local/bin/routermon-speedtest");

QVariantList macs(std::initializer_list<int> endings)
{
    QVariantList out;
    for (int ending : endings) {
        out.append(QStringLiteral("aa:bb:cc:00:00:0%1").arg(ending));
    }
    return out;
}

std::unique_ptr<PlasmoidHarness> fed(const QVariantMap &config = {})
{
    auto harness = started(Form::Planar, config);
    if (!harness || !feed(*harness)) {
        return {};
    }
    return harness;
}

}

class TestPlasmoidRoutermonActions : public QObject
{
    Q_OBJECT

public:
    static void initMain() { PlasmoidHarness::prepareEnvironment(); }

private Q_SLOTS:
    void cleanup()
    {
        QFile::remove(runtime() + QStringLiteral("/data.json"));
        QFile::remove(runtime() + QStringLiteral("/speedtest_live.json"));
    }

    void listsClientsByFilterAndOrder_data()
    {
        QTest::addColumn<QString>("filter");
        QTest::addColumn<QString>("order");
        QTest::addColumn<QVariantList>("expected");
        QTest::newRow("all by traffic") << QStringLiteral("all") << QStringLiteral("traffic") << macs({1, 2, 3});
        QTest::newRow("all by name") << QStringLiteral("all") << QStringLiteral("name") << macs({1, 3, 2});
        QTest::newRow("all by ip") << QStringLiteral("all") << QStringLiteral("ip") << macs({2, 1, 3});
        QTest::newRow("all by signal") << QStringLiteral("all") << QStringLiteral("signal") << macs({1, 2, 3});
        QTest::newRow("online") << QStringLiteral("online") << QStringLiteral("name") << macs({1, 2});
        QTest::newRow("wifi") << QStringLiteral("wifi") << QStringLiteral("name") << macs({1});
        QTest::newRow("wired") << QStringLiteral("wired") << QStringLiteral("name") << macs({3, 2});
        QTest::newRow("blocked") << QStringLiteral("blocked") << QStringLiteral("name") << macs({2});
    }

    void listsClientsByFilterAndOrder()
    {
        QFETCH(QString, filter);
        QFETCH(QString, order);
        QFETCH(QVariantList, expected);
        auto harness = fed({{QStringLiteral("clientFilter"), filter}, {QStringLiteral("sortBy"), order}});
        QVERIFY(harness);
        QCOMPARE(clientMacs(*harness), expected);
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void describesEachClient()
    {
        auto harness = fed();
        QVERIFY(harness);
        const auto field = [&](int row, const char *name) {
            return harness->eval(QStringLiteral("clients.get(%1).%2").arg(row).arg(QLatin1String(name)));
        };
        QCOMPARE(field(0, "name").toString(), QStringLiteral("laptop"));
        QCOMPARE(field(0, "wireless").toBool(), true);
        QCOMPARE(field(0, "band").toString(), QStringLiteral("5GHz-1"));
        QCOMPARE(field(0, "rssi").toInt(), -55);
        QCOMPARE(field(0, "txMbps").toDouble(), 866.0);
        QCOMPARE(field(0, "avgTraffic").toDouble(), 1000000.0);
        QCOMPARE(field(1, "traffic").toInt(), -1);
        QCOMPARE(field(1, "blocked").toBool(), true);
        QCOMPARE(field(1, "wireless").toBool(), false);
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void pinnedClientsComeFirst()
    {
        auto harness = fed({{QStringLiteral("sortBy"), QStringLiteral("name")}});
        QVERIFY(harness);
        harness->eval(QStringLiteral("togglePin('aa:bb:cc:00:00:02')"));
        QCOMPARE(harness->config(QStringLiteral("pinnedMacs")).toString(), QStringLiteral("aa:bb:cc:00:00:02"));
        QCOMPARE(clientMacs(*harness), macs({2, 1, 3}));
        harness->eval(QStringLiteral("togglePin('aa:bb:cc:00:00:03')"));
        QCOMPARE(harness->config(QStringLiteral("pinnedMacs")).toString(), QStringLiteral("aa:bb:cc:00:00:02,aa:bb:cc:00:00:03"));
        QCOMPARE(clientMacs(*harness), macs({3, 2, 1}));
        harness->eval(QStringLiteral("togglePin('aa:bb:cc:00:00:02')"));
        QCOMPARE(clientMacs(*harness), macs({3, 1, 2}));
        QVERIFY(harness->eval(QStringLiteral("isPinned('aa:bb:cc:00:00:03') && !isPinned('aa:bb:cc:00:00:02')")).toBool());
        harness->setConfig(QStringLiteral("clientFilter"), QStringLiteral("online"));
        QCOMPARE(clientMacs(*harness), macs({1, 2}));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void showsTheResultOfRouterCommands()
    {
        auto harness = fed();
        QVERIFY(harness);
        QObject *root = harness->root();
        harness->eval(QStringLiteral("ctlRun('reboot')"));
        QVERIFY(harness->reply(ctl + QStringLiteral(" reboot"), QStringLiteral("{\"ok\": true, \"msg\": \"Reboot command sent\"}\n")));
        QCOMPARE(root->property("message").toString(), QStringLiteral("Reboot command sent"));
        QVERIFY(!root->property("messageError").toBool());
        harness->eval(QStringLiteral("ctlRun('block aa:bb:cc:00:00:01')"));
        QVERIFY(harness->reply(ctl + QStringLiteral(" block"), QStringLiteral("{\"ok\": false, \"msg\": \"router command failed\"}\n"), 1));
        QCOMPARE(root->property("message").toString(), QStringLiteral("router command failed"));
        QVERIFY(root->property("messageError").toBool());
        harness->eval(QStringLiteral("ctlRun('status')"));
        QVERIFY(harness->reply(ctl + QStringLiteral(" status"), QString(), 1, QStringLiteral("Traceback\n")));
        QCOMPARE(root->property("message").toString(), QStringLiteral("Traceback"));
        QVERIFY(root->property("messageError").toBool());
        QTRY_COMPARE_WITH_TIMEOUT(root->property("message").toString(), QString(), 10000);
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void renamesAndReservesWithQuotedValues()
    {
        auto harness = fed();
        QVERIFY(harness);
        QObject *prompt = harness->findAll("PromptDialog").value(0);
        QVERIFY(prompt);
        QMetaObject::invokeMethod(harness->root(), "promptRequested", Q_ARG(QString, QStringLiteral("rename")),
            Q_ARG(QString, QStringLiteral("aa:bb:cc:00:00:01")), Q_ARG(QString, QStringLiteral("Mom's iPad")));
        QMetaObject::invokeMethod(prompt, "accepted");
        QCOMPARE(harness->command(ctl), ctl + QStringLiteral(" rename 'aa:bb:cc:00:00:01' 'Mom'\\''s iPad'"));
        QMetaObject::invokeMethod(harness->root(), "promptRequested", Q_ARG(QString, QStringLiteral("reserve")),
            Q_ARG(QString, QStringLiteral("aa:bb:cc:00:00:03")), Q_ARG(QString, QStringLiteral(" 192.168.1.50 ")));
        QMetaObject::invokeMethod(prompt, "accepted");
        QCOMPARE(harness->command(ctl), ctl + QStringLiteral(" reserve 'aa:bb:cc:00:00:03' '192.168.1.50'"));
        const int before = DataSourceDouble::commands().size();
        QMetaObject::invokeMethod(harness->root(), "promptRequested", Q_ARG(QString, QStringLiteral("rename")),
            Q_ARG(QString, QStringLiteral("aa:bb:cc:00:00:03")), Q_ARG(QString, QStringLiteral("   ")));
        QMetaObject::invokeMethod(prompt, "accepted");
        QCOMPARE(DataSourceDouble::commands().size(), before);
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void opensTheRouterPagesQuoted()
    {
        auto harness = fed();
        QVERIFY(harness);
        const QList<QObject *> buttons = harness->findAll("ToolButton");
        for (QObject *button : buttons) {
            if (button->property("text").toString() == QStringLiteral("Open AdGuard Home")) {
                QMetaObject::invokeMethod(button, "clicked");
            }
        }
        QCOMPARE(harness->command(QStringLiteral("xdg-open")), QStringLiteral("xdg-open 'http://192.168.1.1:3000'"));
        harness->eval(QStringLiteral("middleClick()"));
        QCOMPARE(harness->command(ctl), ctl + QStringLiteral(" pause toggle"));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void runsASpeedTestAndKeepsItsHistory()
    {
        auto harness = fed();
        QVERIFY(harness);
        QObject *root = harness->root();
        harness->eval(QStringLiteral("runSpeedTest()"));
        harness->eval(QStringLiteral("runSpeedTest()"));
        QCOMPARE(DataSourceDouble::commands().count(speedtest), 1);
        QVERIFY(root->property("testing").toBool());
        QVERIFY(harness->deliver(runtime() + QStringLiteral("/speedtest_live.json"), R"({"phase": "download", "mbps": 120.5})",
            [root] { return root->property("live").toMap().value(QStringLiteral("mbps")).toDouble() == 120.5; }));
        QVERIFY(
            harness->reply(speedtest, QStringLiteral("{\"ok\": true, \"down_mbps\": 300, \"up_mbps\": 30, \"ping_ms\": 9, \"ts\": 5}\n")));
        QVERIFY(!root->property("testing").toBool());
        QCOMPARE(root->property("live"), QVariant::fromValue(nullptr));
        QCOMPARE(harness->config(QStringLiteral("peakDown")).toDouble(), 300.0);
        QCOMPARE(harness->eval(QStringLiteral("lastResult.down_mbps")).toInt(), 300);
        QCOMPARE(harness->eval(QStringLiteral("speedHistory.length")).toInt(), 1);
        harness->eval(QStringLiteral("runSpeedTest()"));
        QVERIFY(harness->reply(speedtest, QStringLiteral("{\"ok\": false, \"error\": \"curl failed\"}\n")));
        QCOMPARE(root->property("message").toString(), QStringLiteral("curl failed"));
        QCOMPARE(harness->eval(QStringLiteral("speedHistory.length")).toInt(), 1);
        harness->eval(QStringLiteral("runSpeedTest()"));
        QVERIFY(harness->reply(speedtest, QStringLiteral("not json")));
        QCOMPARE(root->property("message").toString(), QStringLiteral("Speed test failed"));
        harness->eval(QStringLiteral("deleteSpeedResult(0)"));
        QCOMPARE(harness->config(QStringLiteral("speedHistory")).toString(), QStringLiteral("[]"));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void keepsTwentySpeedResults()
    {
        auto harness = fed();
        QVERIFY(harness);
        for (int run = 1; run <= 21; ++run) {
            harness->eval(QStringLiteral("runSpeedTest()"));
            QVERIFY(harness->reply(speedtest, QStringLiteral("{\"ok\": true, \"down_mbps\": %1, \"ts\": %1}").arg(run)));
        }
        QCOMPARE(harness->eval(QStringLiteral("speedHistory.map(r => r.ts)")).toList().size(), 20);
        QCOMPARE(harness->eval(QStringLiteral("speedHistory[0].ts")).toInt(), 21);
        harness->setConfig(QStringLiteral("speedHistory"), QStringLiteral("{broken"));
        QCOMPARE(harness->eval(QStringLiteral("speedHistory")).toList(), QVariantList());
        harness->setConfig(QStringLiteral("lastResult"), QStringLiteral("{broken"));
        QCOMPARE(harness->eval(QStringLiteral("lastResult")), QVariant::fromValue(nullptr));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void overlayHostWaitsForAWindow()
    {
        auto harness = started(Form::Planar, {}, QStringLiteral("org.devl0rd.routermon.overlay"));
        QVERIFY(harness);
        QObject *root = harness->root();
        QCOMPARE(harness->plasmoid()->status, 6);
        QVERIFY(!harness->eval(QStringLiteral("routerData.active")).toBool());
        const QString state = harness->runtimePath(QStringLiteral("Konveyor-Monitor-Overlay/state.json"));
        QVERIFY(
            harness->deliver(state, R"({"targets": [{"key": "a", "pid": 7, "windowId": 3, "x": 0, "y": 0, "width": 900, "height": 600}]})",
                [root] { return root->property("overlayVisible").toBool(); }));
        QVERIFY(feed(*harness));
        QVERIFY(harness->deliver(state, R"({"targets": []})", [root] { return !root->property("overlayVisible").toBool(); }));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }
};

QTEST_MAIN(TestPlasmoidRoutermonActions)

#include "test_plasmoid_routermon_actions.moc"
