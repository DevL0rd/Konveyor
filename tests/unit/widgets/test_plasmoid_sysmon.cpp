#include "plasmoidharness.h"

#include <QTest>

namespace
{

const QByteArray snapshot = R"({"ts": 100, "host": "box", "cpu_model": "13th Gen Intel(R) Core(TM) i9-13980HX", "uptime": 90061,
 "load": [1, 2, 3], "ncpu": 4,
 "cpu": {"total": 42.5, "hybrid": false, "temp": 71, "cores": [10, 20, 90, 50], "freq": 3.21, "fan": 2400,
         "clock_max": 5.6, "power_max": 157, "fan_max": 6000, "watts": 35.2},
 "mem": {"total": 34359738368, "used": 17179869184, "pct": 50, "swap_total": 8589934592, "swap_used": 1073741824, "swap_pct": 12.5},
 "gpu": {"name": "NVIDIA GeForce RTX 4090 Laptop GPU", "util": 97, "vram_used": 4294967296, "vram_total": 17179869184, "vram_pct": 25,
         "temp": 66, "power": 80.5, "clock_gr": 2100, "clock_mem": 9000, "fan": 45, "clock_max": 3000, "power_max": 175}})";

const QByteArray hybridSnapshot = R"({"ts": 1, "host": "box", "cpu_model": "AMD Ryzen 9", "uptime": 59, "ncpu": 3,
 "cpu": {"total": 5, "hybrid": true, "temp": 40, "p": [10, 20], "e": [30], "p_total": 15, "e_total": 30, "p_freq": 4.2, "e_freq": 3.1,
         "freq": 3.9, "fan": 0, "clock_max": 0, "power_max": 0, "fan_max": 0, "watts": 0},
 "mem": {"total": 1024, "used": 512, "pct": 50, "swap_total": 0, "swap_used": 0, "swap_pct": 0}, "gpu": null})";

const QByteArray emptySnapshot = R"({"ts":0,"cpu":{},"mem":{},"gpu":null})";

const PlasmoidSpec panel {QStringLiteral("system-monitor/plasmoids/org.devl0rd.sysmon.panel"), QStringLiteral("org.devl0rd.sysmon.panel"),
    QStringLiteral("cpu"), {}};
const PlasmoidSpec overlayHost {QStringLiteral("system-monitor/plasmoids/org.devl0rd.sysmon.overlay"),
    QStringLiteral("org.devl0rd.sysmon.overlay"), QStringLiteral("cpu"), {}};

QString dataPath()
{
    return qEnvironmentVariable("XDG_RUNTIME_DIR") + QStringLiteral("/Linux-System-Monitor/data.json");
}

std::unique_ptr<PlasmoidHarness> started(int form, const QVariantMap &config = {}, const PlasmoidSpec &spec = panel)
{
    auto harness = std::make_unique<PlasmoidHarness>(spec);
    if (!harness->load(form, config) || !harness->show("compactRepresentation") || !harness->show("fullRepresentation")) {
        qWarning("%s", qPrintable(harness->error));
        return {};
    }
    harness->resolveRuntime(QStringLiteral("printf %s"));
    return harness;
}

bool feed(PlasmoidHarness &harness, const QByteArray &json)
{
    const int tick = harness.root()->property("tick").toInt();
    return harness.deliver(dataPath(), json, [&] { return harness.root()->property("tick").toInt() > tick; });
}

QObject *shell(PlasmoidHarness &harness)
{
    const QList<QObject *> shells = harness.findAll("PopupShell");
    return shells.isEmpty() ? nullptr : shells.first();
}

}

class TestPlasmoidSysmon : public QObject
{
    Q_OBJECT

public:
    static void initMain() { PlasmoidHarness::prepareEnvironment(); }

private Q_SLOTS:
    void cleanup() { QFile::remove(dataPath()); }

    void loadsInEveryFormFactor_data()
    {
        QTest::addColumn<int>("form");
        QTest::addColumn<bool>("compact");
        QTest::newRow("desktop") << Form::Planar << false;
        QTest::newRow("horizontal panel") << Form::Horizontal << true;
        QTest::newRow("vertical panel") << Form::Vertical << true;
    }

    void loadsInEveryFormFactor()
    {
        QFETCH(int, form);
        QFETCH(bool, compact);
        auto harness = started(form);
        QVERIFY(harness);
        QVERIFY(feed(*harness, snapshot));
        QObject *root = harness->root();
        const char *wanted = compact ? "compactRepresentation" : "fullRepresentation";
        QCOMPARE(root->property("preferredRepresentation").value<QQmlComponent *>(), root->property(wanted).value<QQmlComponent *>());
        QCOMPARE(root->property("popupAlive").toBool(), !compact);
        QCOMPARE(harness->plasmoid()->icon, QStringLiteral("cpu"));
        QCOMPARE(harness->plasmoid()->status, 2);
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void waitsForTheCollector()
    {
        auto harness = started(Form::Planar);
        QVERIFY(harness);
        QCOMPARE(harness->eval(QStringLiteral("tooltipText()")).toString(), QStringLiteral("Waiting for the collector"));
        QVERIFY(visibleTexts(harness->scene()).contains(QStringLiteral("Waiting for data")));
        QVERIFY(!harness->root()->property("collectorAlive").toBool());
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void readsSnapshotsIntoHistory()
    {
        auto harness = started(Form::Horizontal);
        QVERIFY(harness);
        QVERIFY(feed(*harness, snapshot));
        QVERIFY(harness->root()->property("collectorAlive").toBool());
        QVERIFY(feed(*harness, QByteArray(snapshot).replace("\"total\": 42.5", "\"total\": 60")));
        QCOMPARE(harness->eval(QStringLiteral("series('cpu.usage')")).toList(), (QVariantList {42.5, 60}));
        QCOMPARE(harness->eval(QStringLiteral("series('gpu.clock')")).toList(), (QVariantList {2100, 2100}));
        QCOMPARE(harness->eval(QStringLiteral("series('mem.usage')")).toList().size(), 2);
        QCOMPARE(harness->eval(QStringLiteral("series('missing')")).toList(), QVariantList());
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void ignoresUnreadableSnapshots()
    {
        auto harness = started(Form::Horizontal);
        QVERIFY(harness);
        QVERIFY(feed(*harness, snapshot));
        harness->writeFile(dataPath(), "{\"ts\": 1, \"cpu\":");
        QVERIFY(feed(*harness, QByteArray(snapshot).replace("\"total\": 42.5", "\"total\": 7")));
        QCOMPARE(harness->eval(QStringLiteral("series('cpu.usage')")).toList(), (QVariantList {42.5, 7}));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void describesTheMachineInTheTooltip()
    {
        auto harness = started(Form::Horizontal);
        QVERIFY(harness);
        QVERIFY(feed(*harness, snapshot));
        QCOMPARE(harness->root()->property("toolTipMainText").toString(), QStringLiteral("box"));
        harness->root()->setProperty("tooltipWanted", false);
        QCOMPARE(harness->root()->property("toolTipSubText").toString(), QString());
        harness->root()->setProperty("tooltipWanted", true);
        QCOMPARE(harness->root()->property("toolTipSubText").toString(),
            QStringLiteral("Up 1d 1h\nCPU 43% · 71°C · 3.2 GHz · 35 W\nGPU 97% · 66°C · 81 W · VRAM 4.0 GB / 16.0 GB\n"
                           "RAM 16.0 GB / 32.0 GB · Swap 1.0 GB / 8.0 GB"));
        QCOMPARE(harness->eval(QStringLiteral("cpuShort()")).toString(), QStringLiteral("i9-13980HX"));
        QCOMPARE(harness->eval(QStringLiteral("gpuShort(gpu.name)")).toString(), QStringLiteral("RTX 4090 Laptop GPU"));
        QCOMPARE(harness->eval(QStringLiteral("gpuShort('')")).toString(), QStringLiteral("GPU"));
        QVERIFY(feed(*harness, hybridSnapshot));
        QCOMPARE(harness->root()->property("toolTipSubText").toString(),
            QStringLiteral("Up 0m\nCPU 5% · 40°C · 3.9 GHz · 0 W\nRAM 512 B / 1 KB"));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void survivesTheCollectorsEmptySnapshot()
    {
        auto harness = started(Form::Planar);
        QVERIFY(harness);
        QVERIFY(feed(*harness, emptySnapshot));
        harness->root()->setProperty("tooltipWanted", true);
        QCOMPARE(harness->root()->property("toolTipSubText").toString(), QStringLiteral("CPU 0% · 0°C · 0.0 GHz · 0 W\nRAM 0 B / 0 B"));
        QCOMPARE(harness->eval(QStringLiteral("gpu")), QVariant::fromValue(nullptr));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void scalesGraphsToTheConfiguredMaximum()
    {
        auto harness = started(Form::Horizontal, {{QStringLiteral("cpuClockMax"), 5}, {QStringLiteral("gpuFanMax"), 80}});
        QVERIFY(harness);
        QVERIFY(feed(*harness, snapshot));
        QCOMPARE(harness->eval(QStringLiteral("cpuMetrics.map(m => m.max(cpu))")).toList(), (QVariantList {100, 100, 5, 157, 6000}));
        QCOMPARE(harness->eval(QStringLiteral("gpuMetrics.map(m => m.max(gpu))")).toList(), (QVariantList {100, 100, 3000, 175, 80}));
        QVERIFY(feed(*harness, hybridSnapshot));
        QCOMPARE(harness->eval(QStringLiteral("cpuMetrics.map(m => m.max(cpu))")).toList(), (QVariantList {100, 100, 5, 100, 6000}));
        QCOMPARE(harness->eval(QStringLiteral("cpuMetrics.map(m => m.fmt(m.get(cpu)))")).toList(),
            (QVariantList {
                QStringLiteral("5%"), QStringLiteral("40°C"), QStringLiteral("3.90 GHz"), QStringLiteral("0 W"), QStringLiteral("—")}));
        QCOMPARE(harness->eval(QStringLiteral("cpuMetrics[0].color(90) === Kirigami.Theme.negativeTextColor")).toBool(), true);
        QCOMPARE(harness->eval(QStringLiteral("cpuMetrics[2].color")), QVariant());
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void keepsHistoryWithinItsConfiguredLength()
    {
        auto harness = started(Form::Horizontal, {{QStringLiteral("historyLength"), 10}});
        QVERIFY(harness);
        QCOMPARE(harness->root()->property("historyLength").toInt(), 60);
        for (int value = 1; value <= 3; ++value) {
            QVERIFY(feed(*harness, QByteArray(snapshot).replace("\"total\": 42.5", "\"total\": " + QByteArray::number(value))));
        }
        harness->setConfig(QStringLiteral("historyLength"), 5000);
        QCOMPARE(harness->root()->property("historyLength").toInt(), 600);
        QCOMPARE(harness->eval(QStringLiteral("series('cpu.usage')")).toList(), (QVariantList {1, 2, 3}));
        QCOMPARE(harness->eval(QStringLiteral("history.len")).toInt(), 600);
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void tellsTheCollectorTheUpdateInterval()
    {
        auto harness = started(Form::Horizontal);
        QVERIFY(harness);
        const QString set = QStringLiteral("$HOME/.local/bin/sysmon-collect --set-interval ");
        QCOMPARE(harness->command(set), set + QStringLiteral("0.5"));
        harness->reply(set, QString());
        harness->setConfig(QStringLiteral("updateInterval"), 2000);
        QCOMPARE(harness->command(set), set + QStringLiteral("2"));
        harness->reply(set, QString());
        harness->setConfig(QStringLiteral("updateInterval"), 100);
        QCOMPARE(harness->command(set), set + QStringLiteral("0.5"));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void middleClickOpensSystemMonitorOnlyWhenAsked()
    {
        auto harness = started(Form::Horizontal, {{QStringLiteral("middleClickAction"), QStringLiteral("none")}});
        QVERIFY(harness);
        harness->eval(QStringLiteral("middleClick()"));
        QCOMPARE(harness->command(QStringLiteral("plasma-systemmonitor")), QString());
        harness->setConfig(QStringLiteral("middleClickAction"), QStringLiteral("systemmonitor"));
        harness->eval(QStringLiteral("middleClick()"));
        QCOMPARE(harness->command(QStringLiteral("plasma-systemmonitor")), QStringLiteral("plasma-systemmonitor"));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void keepsThePopupLoadedBrieflyAfterClosing()
    {
        auto harness = started(Form::Horizontal);
        QVERIFY(harness);
        QObject *root = harness->root();
        QVERIFY(!root->property("popupAlive").toBool());
        root->setProperty("expanded", true);
        QVERIFY(root->property("popupAlive").toBool());
        QVERIFY(shell(*harness));
        root->setProperty("expanded", false);
        QVERIFY(root->property("popupAlive").toBool());
        QTRY_VERIFY(!root->property("popupAlive").toBool());
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void marksTheCollectorStoppedWhenSnapshotsStop()
    {
        auto harness = started(Form::Planar);
        QVERIFY(harness);
        QVERIFY(feed(*harness, snapshot));
        QObject *popup = shell(*harness);
        QVERIFY(popup);
        QCOMPARE(popup->property("statusText").toString(), QStringLiteral("Live"));
        QTRY_VERIFY_WITH_TIMEOUT(!harness->root()->property("collectorAlive").toBool(), 10000);
        QCOMPARE(popup->property("statusText").toString(), QStringLiteral("Collector stopped"));
        QVERIFY(visibleTexts(harness->scene()).contains(QStringLiteral("Collector not running")));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void searchFiltersTheCards_data()
    {
        QTest::addColumn<QString>("query");
        QTest::addColumn<int>("matches");
        QTest::newRow("empty") << QString() << -1;
        QTest::newRow("gpu") << QStringLiteral("gpu") << 1;
        QTest::newRow("temperature") << QStringLiteral("temp") << 2;
        QTest::newRow("core") << QStringLiteral("core 3") << 1;
        QTest::newRow("swap") << QStringLiteral("swap") << 1;
        QTest::newRow("model name") << QStringLiteral("rtx") << 1;
        QTest::newRow("nothing") << QStringLiteral("zzz") << 0;
    }

    void searchFiltersTheCards()
    {
        QFETCH(QString, query);
        QFETCH(int, matches);
        auto harness = started(Form::Planar);
        QVERIFY(harness);
        QVERIFY(feed(*harness, snapshot));
        QObject *popup = shell(*harness);
        QVERIFY(popup);
        popup->setProperty("searchText", query);
        QCOMPARE(popup->property("matchCount").toInt(), matches);
        QCOMPARE(visibleTexts(harness->scene()).contains(QStringLiteral("No matches")), matches == 0);
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void collapsesCardsIntoTheConfiguration()
    {
        auto harness = started(Form::Planar, {{QStringLiteral("collapsedCards"), QStringLiteral("gpu")}});
        QVERIFY(harness);
        QVERIFY(feed(*harness, snapshot));
        QObject *popup = shell(*harness);
        QVERIFY(popup);
        QVERIFY(harness->eval(QStringLiteral("isCollapsed('gpu')"), popup).toBool());
        harness->eval(QStringLiteral("toggleCollapsed('cpu')"), popup);
        QCOMPARE(harness->config(QStringLiteral("collapsedCards")).toString(), QStringLiteral("gpu,cpu"));
        harness->eval(QStringLiteral("toggleCollapsed('gpu')"), popup);
        QCOMPARE(harness->config(QStringLiteral("collapsedCards")).toString(), QStringLiteral("cpu"));
        popup->setProperty("searchText", QStringLiteral("cpu"));
        QVERIFY(!harness->eval(QStringLiteral("isCollapsed('cpu')"), popup).toBool());
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void showsHybridCoreGroups()
    {
        auto harness = started(Form::Planar);
        QVERIFY(harness);
        QVERIFY(feed(*harness, hybridSnapshot));
        QObject *popup = shell(*harness);
        QVERIFY(popup);
        const QStringList texts = visibleTexts(harness->scene());
        QVERIFY2(texts.contains(QStringLiteral("2 P · 1 E")), qPrintable(texts.join(QLatin1Char('|'))));
        QVERIFY(texts.contains(QStringLiteral("Performance cores")));
        popup->setProperty("searchText", QStringLiteral("e-core 1"));
        QCOMPARE(harness->eval(QStringLiteral("coreQuery"), popup).toMap(),
            (QVariantMap {{QStringLiteral("group"), QStringLiteral("e")}, {QStringLiteral("index"), 1}}));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void opensTheSettingsFromThePopup()
    {
        auto harness = started(Form::Planar);
        QVERIFY(harness);
        QVERIFY(feed(*harness, snapshot));
        QObject *popup = shell(*harness);
        QVERIFY(popup);
        const QList<QObject *> buttons = findByType(popup, "ToolButton");
        for (QObject *button : buttons) {
            if (button->property("text").toString() == QStringLiteral("Configure…")) {
                QMetaObject::invokeMethod(button, "clicked");
            }
        }
        QVERIFY(harness->plasmoid()->action(QStringLiteral("configure")));
        QCOMPARE(harness->plasmoid()->action(QStringLiteral("configure"))->triggered, 1);
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void overlayHostStaysHiddenUntilAWindowAsks()
    {
        auto harness = started(Form::Planar, {}, overlayHost);
        QVERIFY(harness);
        QCOMPARE(harness->plasmoid()->status, 6);
        QVERIFY(!harness->root()->property("dataWanted").toBool());
        const QString state = harness->runtimePath(QStringLiteral("Konveyor-Monitor-Overlay/state.json"));
        QObject *root = harness->root();
        QVERIFY(harness->deliver(state,
            R"({"targets": [{"key": "app", "pid": 5, "windowId": 3, "x": 0, "y": 0, "width": 900, "height": 600}]})",
            [&] { return root->property("overlayVisible").toBool(); }));
        QVERIFY(root->property("dataWanted").toBool());
        QVERIFY(feed(*harness, snapshot));
        QVERIFY(harness->deliver(state, R"({"targets": []})", [&] { return !root->property("overlayVisible").toBool(); }));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }
};

QTEST_MAIN(TestPlasmoidSysmon)

#include "test_plasmoid_sysmon.moc"
