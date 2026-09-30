#include "routerfixture.h"

using Router::changed;
using Router::feed;
using Router::feedState;
using Router::runtime;
using Router::shell;
using Router::started;

class TestPlasmoidRoutermon : public QObject
{
    Q_OBJECT

public:
    static void initMain() { PlasmoidHarness::prepareEnvironment(); }

private Q_SLOTS:
    void cleanup() { QFile::remove(runtime() + QStringLiteral("/data.json")); }

    void loadsEveryVariant_data()
    {
        QTest::addColumn<QString>("id");
        QTest::addColumn<int>("form");
        QTest::addColumn<QString>("tab");
        QTest::addColumn<QString>("title");
        const QList<QPair<QString, QString>> variants {{QStringLiteral("panel"), QString()},
            {QStringLiteral("network"), QStringLiteral("Network")}, {QStringLiteral("wifi"), QStringLiteral("WiFi")},
            {QStringLiteral("clients"), QStringLiteral("Clients")}, {QStringLiteral("dns"), QStringLiteral("DNS")},
            {QStringLiteral("speedtest"), QStringLiteral("Speed Test")}, {QStringLiteral("system"), QStringLiteral("System")}};
        for (const auto &[name, label] : variants) {
            const QString tab = name == QLatin1String("panel") ? QStringLiteral("overview")
                : name == QLatin1String("speedtest")           ? QStringLiteral("speed")
                                                               : name;
            const QString title = label.isEmpty() ? QStringLiteral("Router") : QStringLiteral("Router · ") + label;
            for (int form : {Form::Planar, Form::Horizontal}) {
                QTest::addRow("%s %d", qPrintable(name), form) << QStringLiteral("org.devl0rd.routermon.") + name << form << tab << title;
            }
        }
    }

    void loadsEveryVariant()
    {
        QFETCH(QString, id);
        QFETCH(int, form);
        QFETCH(QString, tab);
        QFETCH(QString, title);
        auto harness = started(form, {}, id);
        QVERIFY(harness);
        QVERIFY(feed(*harness));
        harness->root()->setProperty("expanded", true);
        QCOMPARE(harness->root()->property("activeTab").toString(), tab);
        QCOMPARE(harness->plasmoid()->title, title);
        QCOMPARE(harness->plasmoid()->icon, Router::variant(id).iconName);
        QCOMPARE(harness->plasmoid()->status, 2);
        QTRY_VERIFY(visibleTexts(harness->scene()).contains(QStringLiteral("RT-AX88U")) || !title.endsWith(QLatin1String("Router")));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void showsEveryTab_data()
    {
        QTest::addColumn<QString>("tab");
        QTest::addColumn<QString>("text");
        QTest::newRow("overview") << QStringLiteral("overview") << QStringLiteral("Router CPU");
        QTest::newRow("network") << QStringLiteral("network") << QStringLiteral("203.0.113.5");
        QTest::newRow("wifi") << QStringLiteral("wifi") << QStringLiteral("Home Net");
        QTest::newRow("clients") << QStringLiteral("clients") << QStringLiteral("laptop");
        QTest::newRow("dns") << QStringLiteral("dns") << QStringLiteral("ads.example");
        QTest::newRow("speed") << QStringLiteral("speed") << QStringLiteral("Run speed test");
        QTest::newRow("system") << QStringLiteral("system") << QStringLiteral("Reboot router");
    }

    void showsEveryTab()
    {
        QFETCH(QString, tab);
        QFETCH(QString, text);
        auto harness = started(Form::Planar);
        QVERIFY(harness);
        QVERIFY(feed(*harness));
        harness->root()->setProperty("tabKey", tab);
        QCOMPARE(harness->config(QStringLiteral("currentTab")).toString(), tab);
        QTRY_VERIFY2(visibleTexts(harness->scene()).join(QLatin1Char('|')).contains(text),
            qPrintable(visibleTexts(harness->scene()).join(QLatin1Char('|'))));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void reportsTheRouterState()
    {
        auto harness = started(Form::Horizontal);
        QVERIFY(harness);
        QObject *root = harness->root();
        QVERIFY(feed(*harness));
        root->setProperty("tooltipWanted", true);
        QCOMPARE(root->property("routerState").toString(), QStringLiteral("ok"));
        QCOMPARE(root->property("toolTipMainText").toString(), QStringLiteral("RT-AX88U · 203.0.113.5"));
        QCOMPARE(root->property("toolTipSubText").toString(),
            QStringLiteral("↓ 251 Mb/s  ↑ 20.3 Mb/s · 11 ms · 0% loss\n3 devices · 1 on WiFi\nAdGuard 25% blocked"));
        QCOMPARE(root->property("onlineCount").toInt(), 2);
        QVERIFY(feed(*harness,
            changed({{QStringLiteral("network"), QVariantMap {{QStringLiteral("wan_up"), false}}}, {QStringLiteral("dns"), QVariant()}})));
        QCOMPARE(root->property("routerState").toString(), QStringLiteral("wandown"));
        QCOMPARE(root->property("toolTipSubText").toString().section(QLatin1Char('\n'), 0, 0), QStringLiteral("Internet is down"));
        QVERIFY(feedState(*harness, R"({"online": true, "paused": true})", "paused", true));
        QCOMPARE(root->property("stateText").toString(), QStringLiteral("Monitoring paused"));
        QCOMPARE(root->property("toolTipSubText").toString(), QStringLiteral("Monitoring paused"));
        QCOMPARE(harness->eval(QStringLiteral("info.model")).toString(), QStringLiteral("RT-AX88U"));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void fallsBackToTheLocalNetwork()
    {
        auto harness = started(Form::Horizontal);
        QVERIFY(harness);
        QObject *root = harness->root();
        const QByteArray local = R"({"ts": 1, "online": false, "paused": false, "fallback": "local", "error": "Router not set up",
            "network": {"down_mbps": 0.5, "up_mbps": 0.25, "ping_rtt": 20, "ping_loss": 0}})";
        QVERIFY(feed(*harness, local));
        root->setProperty("tooltipWanted", true);
        QCOMPARE(root->property("routerState").toString(), QStringLiteral("offline"));
        QVERIFY(root->property("localFallback").toBool());
        QCOMPARE(root->property("toolTipSubText").toString(),
            QStringLiteral("Router not connected · showing this computer's network I/O and ping"));
        QCOMPARE(root->property("toolTipMainText").toString(), QStringLiteral("Router"));
        QVERIFY(feedState(*harness, R"({"online": false, "error": "no data"})", "online", false));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void formatsSpeedsAndNames()
    {
        auto harness = started(Form::Planar);
        QVERIFY(harness);
        QVERIFY(feed(*harness));
        QCOMPARE(harness->eval(QStringLiteral("[speedText(undefined), speedText(0.5), speedText(5), speedText(150.4), speedText(1500)]"))
                     .toStringList(),
            (QStringList {QStringLiteral("0 Kb/s"), QStringLiteral("500 Kb/s"), QStringLiteral("5.0 Mb/s"), QStringLiteral("150 Mb/s"),
                QStringLiteral("1.50 Gb/s")}));
        QCOMPARE(harness->eval(QStringLiteral("[kib(undefined), kib(1048576)]")).toStringList(),
            (QStringList {QStringLiteral("0 B"), QStringLiteral("1.0 GB")}));
        QCOMPARE(harness->eval(QStringLiteral("[nameForMac('AA:BB:CC:00:00:01'), nameForMac('aa:bb:cc:00:00:09'), nameForMac(undefined)]"))
                     .toList(),
            (QVariantList {QStringLiteral("laptop"), QStringLiteral("aa:bb:cc:00:00:09"), QVariant()}));
        QCOMPARE(harness->eval(QStringLiteral("[shortBand('5GHz-1'), shortBand(undefined)]")).toStringList(),
            (QStringList {QStringLiteral("5G · 1"), QString()}));
        QCOMPARE(harness->eval(QStringLiteral("sshTarget('192.168.1.10')")).toString(), QStringLiteral("192.168.1.10"));
        harness->setConfig(QStringLiteral("sshUser"), QStringLiteral("admin"));
        QCOMPARE(harness->eval(QStringLiteral("sshTarget('192.168.1.10')")).toString(), QStringLiteral("admin@192.168.1.10"));
        QCOMPARE(harness->eval(QStringLiteral("shq(\"Mom's iPad\")")).toString(), QStringLiteral("'Mom'\\''s iPad'"));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void describesAges()
    {
        auto harness = started(Form::Planar);
        QVERIFY(harness);
        const QString now = QStringLiteral("Date.now() / 1000");
        QCOMPARE(
            harness
                ->eval(QStringLiteral("[ago(0), ago(%1 - 30), ago(%1 + 30), ago(%1 - 600), ago(%1 - 7200), ago(%1 - 3 * 86400)]").arg(now))
                .toStringList(),
            (QStringList {QString(), QStringLiteral("just now"), QStringLiteral("just now"), QStringLiteral("10 minutes ago"),
                QStringLiteral("2 hours ago"), QStringLiteral("3 days ago")}));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void recordsHistoryAndPeaks()
    {
        auto harness = started(Form::Horizontal);
        QVERIFY(harness);
        QVERIFY(feed(*harness));
        QVERIFY(feed(*harness, changed({{QStringLiteral("ts"), 2}})));
        QCOMPARE(harness->eval(QStringLiteral("series('down')")).toList(), (QVariantList {250.5, 250.5}));
        QCOMPARE(harness->eval(QStringLiteral("series('cpu')")).toList(), (QVariantList {42.5, 42.5}));
        QCOMPARE(harness->eval(QStringLiteral("series('lan.rx')")).toList(), (QVariantList {5.5, 5.5}));
        QCOMPARE(harness->eval(QStringLiteral("clientSeries('AA:BB:CC:00:00:01')")).toList(), (QVariantList {1000000, 1000000}));
        QCOMPARE(harness->eval(QStringLiteral("clientSeries('aa:bb:cc:00:00:02')")).toList(), (QVariantList {0, 0}));
        QCOMPARE(harness->config(QStringLiteral("peakDown")).toDouble(), 250.5);
        QCOMPARE(harness->config(QStringLiteral("peakUp")).toDouble(), 20.25);
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void keepsOrForgetsTheTab()
    {
        auto harness
            = started(Form::Horizontal, {{QStringLiteral("rememberTab"), false}, {QStringLiteral("defaultTab"), QStringLiteral("dns")}});
        QVERIFY(harness);
        QObject *root = harness->root();
        QCOMPARE(root->property("tabKey").toString(), QStringLiteral("dns"));
        root->setProperty("tabKey", QStringLiteral("wifi"));
        root->setProperty("expanded", true);
        QCOMPARE(root->property("tabKey").toString(), QStringLiteral("dns"));
        harness->setConfig(QStringLiteral("rememberTab"), true);
        root->setProperty("tabKey", QStringLiteral("wifi"));
        root->setProperty("expanded", false);
        root->setProperty("expanded", true);
        QCOMPARE(root->property("tabKey").toString(), QStringLiteral("wifi"));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void searchFindsDevicesRadiosDomainsAndActions_data()
    {
        QTest::addColumn<QString>("query");
        QTest::addColumn<int>("count");
        QTest::newRow("device") << QStringLiteral("laptop") << 1;
        QTest::newRow("ip") << QStringLiteral("192.168.1.1") << 3;
        QTest::newRow("radio") << QStringLiteral("home net") << 2;
        QTest::newRow("domain") << QStringLiteral("ads.") << 1;
        QTest::newRow("action") << QStringLiteral("reboot") << 1;
        QTest::newRow("nothing") << QStringLiteral("zzzz") << 0;
    }

    void searchFindsDevicesRadiosDomainsAndActions()
    {
        QFETCH(QString, query);
        QFETCH(int, count);
        auto harness = started(Form::Planar);
        QVERIFY(harness);
        QVERIFY(feed(*harness));
        QObject *popup = shell(*harness);
        QVERIFY(popup);
        popup->setProperty("searchText", query);
        QTRY_COMPARE(popup->property("matchCount").toInt(), count);
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void acceptingASearchActsOnTheFirstResult()
    {
        auto harness = started(Form::Planar);
        QVERIFY(harness);
        QVERIFY(feed(*harness));
        QObject *popup = shell(*harness);
        QVERIFY(popup);
        popup->setProperty("searchText", QStringLiteral("laptop"));
        QTRY_COMPARE(popup->property("matchCount").toInt(), 1);
        QMetaObject::invokeMethod(popup, "searchAccepted");
        QCOMPARE(harness->root()->property("expandedMac").toString(), QStringLiteral("aa:bb:cc:00:00:01"));
        popup->setProperty("searchText", QStringLiteral("pause"));
        QTRY_COMPARE(popup->property("matchCount").toInt(), 1);
        QMetaObject::invokeMethod(popup, "searchAccepted");
        QCOMPARE(harness->command(QStringLiteral("$HOME/.local/bin/routermon-ctl")),
            QStringLiteral("$HOME/.local/bin/routermon-ctl pause toggle"));
        popup->setProperty("searchText", QStringLiteral("home net"));
        QTRY_COMPARE(popup->property("matchCount").toInt(), 2);
        QMetaObject::invokeMethod(popup, "searchAccepted");
        QCOMPARE(harness->root()->property("tabKey").toString(), QStringLiteral("wifi"));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }
};

QTEST_MAIN(TestPlasmoidRoutermon)

#include "test_plasmoid_routermon.moc"
