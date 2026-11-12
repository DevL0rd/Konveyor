#include "sysmonfixture.h"

using Sysmon::dataPath;
using Sysmon::emptySnapshot;
using Sysmon::feed;
using Sysmon::hybridSnapshot;
using Sysmon::intelSnapshot;
using Sysmon::shell;
using Sysmon::snapshot;
using Sysmon::started;

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
                           "RAM 16.0 GB / 32.0 GB · Swap 1.0 GB / 8.0 GB\nNetwork ↓ 10.0 Mb/s · ↑ 320 Kb/s"));
        QCOMPARE(harness->eval(QStringLiteral("cpuShort()")).toString(), QStringLiteral("i9-13980HX"));
        QCOMPARE(harness->eval(QStringLiteral("gpuShort(gpu.name)")).toString(), QStringLiteral("RTX 4090 Laptop GPU"));
        QCOMPARE(harness->eval(QStringLiteral("gpuShort('')")).toString(), QStringLiteral("GPU"));
        QVERIFY(feed(*harness, hybridSnapshot));
        QCOMPARE(harness->root()->property("toolTipSubText").toString(),
            QStringLiteral("Up 0m\nCPU 5% · 40°C · 3.9 GHz · 0 W\nRAM 512 B / 1 KB"));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void leavesOutGpuMetricsTheDriverCannotRead()
    {
        auto harness = started(Form::Planar);
        QVERIFY(harness);
        QVERIFY(feed(*harness, intelSnapshot));
        harness->root()->setProperty("tooltipWanted", true);
        QCOMPARE(harness->root()->property("toolTipSubText").toString(),
            QStringLiteral("Up 1d 1h\nCPU 43% · 71°C · 3.2 GHz · 35 W\nGPU 31% · 1250 MHz\nRAM 16.0 GB / 32.0 GB · Swap 1.0 GB / 8.0 GB"));
        QCOMPARE(harness->eval(QStringLiteral("gpuShown.map(m => m.key)")).toStringList(),
            (QStringList {QStringLiteral("usage"), QStringLiteral("clock")}));
        QCOMPARE(harness->eval(QStringLiteral("series('gpu.temp')")).toList(), QVariantList());
        QCOMPARE(harness->eval(QStringLiteral("series('gpu.clock')")).toList(), (QVariantList {1250}));
        QStringList captions;
        for (QObject *ring : harness->findAll("PopRing")) {
            captions.append(ring->property("caption").toString());
        }
        QVERIFY2(captions.contains(QStringLiteral("1250 MHz")), qPrintable(captions.join(QLatin1Char('|'))));
        const QStringList texts = visibleTexts(harness->scene());
        QVERIFY2(texts.contains(QStringLiteral("Max clock")), qPrintable(texts.join(QLatin1Char('|'))));
        QVERIFY(!texts.contains(QStringLiteral("Power limit")));
        QVERIFY(!texts.contains(QStringLiteral("Memory clock")));
        QVERIFY(!texts.contains(QStringLiteral("Network")));
        QVERIFY(!texts.contains(QStringLiteral("NET")));
        QObject *popup = shell(*harness);
        QVERIFY(popup);
        popup->setProperty("searchText", QStringLiteral("temp"));
        QCOMPARE(popup->property("matchCount").toInt(), 1);
        popup->setProperty("searchText", QStringLiteral("clock"));
        QCOMPARE(popup->property("matchCount").toInt(), 2);
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void showsWirelessNetworksInGigabits()
    {
        auto harness = started(Form::Planar);
        QVERIFY(harness);
        QVERIFY(feed(*harness, QByteArray(snapshot).replace("\"down\": 1250000", "\"down\": 187500000").replace("enp5s0", "wlan0")));
        harness->root()->setProperty("tooltipWanted", true);
        QVERIFY2(harness->root()->property("toolTipSubText").toString().endsWith(QStringLiteral("\nNetwork ↓ 1.50 Gb/s · ↑ 320 Kb/s")),
            qPrintable(harness->root()->property("toolTipSubText").toString()));
        QObject *card = nullptr;
        for (QObject *candidate : harness->findAll("NetCard")) {
            card = candidate;
        }
        QVERIFY(card);
        QCOMPARE(card->property("icon").toString(), QStringLiteral("network-wireless"));
        QCOMPARE(card->property("trailing").toString(), QStringLiteral("1.50 Gb/s"));
        QVERIFY(harness->eval(QStringLiteral("netMetrics[0].color(0) === Style.hue('down', Kirigami.Theme)")).toBool());
        QObject *chart = nullptr;
        for (QObject *candidate : findByType(card, "Sparkline")) {
            chart = candidate;
        }
        QVERIFY(chart);
        QCOMPARE(chart->property("gradient").toBool(), false);
        QVERIFY(feed(*harness, QByteArray(snapshot).replace("\"enp5s0\"", "\"wwan0\", \"enp5s0\"")));
        QCOMPARE(card->property("icon").toString(), QStringLiteral("network-wired"));
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
        const QString set = QStringLiteral("PATH=\"$HOME/.local/bin:$PATH\" sysmon-collect --set-interval ");
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
};

QTEST_MAIN(TestPlasmoidSysmon)

#include "test_plasmoid_sysmon.moc"
