#include "procmonfixture.h"

using Procmon::feed;
using Procmon::panel;
using Procmon::rowsBecome;
using Procmon::runtime;
using Procmon::started;
using Procmon::tasks;

class TestPlasmoidProcmon : public QObject
{
    Q_OBJECT

public:
    static void initMain() { PlasmoidHarness::prepareEnvironment(); }

private Q_SLOTS:
    void cleanup()
    {
        QFile::remove(runtime() + QStringLiteral("/data.json"));
        QFile::remove(runtime() + QStringLiteral("/panel/panel.json"));
    }

    void loadsInEveryFormFactor_data()
    {
        QTest::addColumn<int>("form");
        QTest::newRow("desktop") << Form::Planar;
        QTest::newRow("horizontal panel") << Form::Horizontal;
        QTest::newRow("vertical panel") << Form::Vertical;
    }

    void loadsInEveryFormFactor()
    {
        QFETCH(int, form);
        auto harness = started(form);
        QVERIFY(harness);
        QCOMPARE(harness->plasmoid()->icon, QStringLiteral("utilities-system-monitor"));
        QCOMPARE(harness->plasmoid()->status, 2);
        QCOMPARE(harness->root()->property("runtimeDir").toString(), runtime());
        QCOMPARE(harness->root()->property("compactOnly").toBool(), form != Form::Planar);
        QCOMPARE(harness->command(QStringLiteral("$HOME/.local/bin/procmon-collect --set-interval ")),
            QStringLiteral("$HOME/.local/bin/procmon-collect --set-interval 0.5"));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void summarizesTheSnapshot()
    {
        auto harness = started(Form::Planar);
        QVERIFY(harness);
        QCOMPARE(harness->eval(QStringLiteral("tooltipText()")).toString(), QStringLiteral("Waiting for the collector"));
        QVERIFY(feed(*harness));
        const QVariantMap summary = harness->root()->property("summary").toMap();
        QCOMPARE(summary.value(QStringLiteral("count")).toInt(), 5);
        QCOMPARE(summary.value(QStringLiteral("cpu")).toDouble(), 27.0);
        QCOMPARE(summary.value(QStringLiteral("gpuTop")).toMap().value(QStringLiteral("name")).toString(), QStringLiteral("game"));
        QCOMPARE(harness->eval(QStringLiteral("tooltipText()")).toString(), QStringLiteral("5 processes"));
        QVERIFY(rowsBecome(*harness, {100, 300}));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void followsTheActiveWindow()
    {
        auto harness = started(Form::Planar);
        QVERIFY(harness);
        QVERIFY(feed(*harness));
        tasks()->activate(200, QStringLiteral("Game"));
        QTRY_COMPARE(harness->eval(QStringLiteral("focusProc ? focusProc.pid : 0")).toInt(), 200);
        const QString focus = runtime() + QStringLiteral("/focus");
        QCOMPARE(harness->command(QStringLiteral("printf '%s\\n%s\\n' 200")),
            QStringLiteral("printf '%s\\n%s\\n' 200 'Game' > '") + focus + QLatin1Char('\''));
        QCOMPARE(harness->eval(QStringLiteral("focusProc.fps")).toInt(), 144);
        QCOMPARE(harness->eval(QStringLiteral("focusProc.parentName")).toString(), QStringLiteral("plasmashell"));
        QCOMPARE(harness->root()->property("toolTipMainText").toString(), QStringLiteral("game · PID 200"));
        QCOMPARE(harness->eval(QStringLiteral("tooltipText()")).toString(),
            QStringLiteral("CPU 21% · GPU 80% · VRAM 4.0 GB · RAM 2.0 GB\n144 FPS · 6.9 ms · 1% low 120"));
        tasks()->clearActive();
        QCOMPARE(harness->root()->property("focusPid").toInt(), 200);
        QCOMPARE(harness->root()->property("focusAppName").toString(), QStringLiteral("Game"));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void focusedProcessWithoutFramesHasNoFrameLine()
    {
        auto harness = started(Form::Planar);
        QVERIFY(harness);
        QVERIFY(feed(*harness));
        tasks()->activate(300, QStringLiteral("it's"));
        QTRY_COMPARE(harness->eval(QStringLiteral("focusProc ? focusProc.pid : 0")).toInt(), 300);
        QCOMPARE(harness->eval(QStringLiteral("tooltipText()")).toString(), QStringLiteral("CPU 1% · GPU 0% · VRAM 0 B · RAM 1 KB"));
        QVERIFY(harness->command(QStringLiteral("printf '%s\\n%s\\n' 300")).contains(QStringLiteral("'it'\\''s'")));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void watchesFrameTelemetryForTheFocusedProcess()
    {
        auto harness = started(Form::Planar);
        QVERIFY(harness);
        QVERIFY(feed(*harness));
        QCOMPARE(SessionBusDouble::calls().value(0).value(QStringLiteral("member")).toString(), QStringLiteral("Watch"));
        QCOMPARE(SessionBusDouble::calls().value(0).value(QStringLiteral("arguments")).toList(), QVariantList {QStringLiteral("[]")});
        tasks()->activate(200, QStringLiteral("Game"));
        QTRY_COMPARE(
            SessionBusDouble::calls().constLast().value(QStringLiteral("arguments")).toList(), QVariantList {QStringLiteral("[200]")});
        SignalWatcherDouble *watcher = SignalWatcherDouble::instances().value(0);
        QVERIFY(watcher);
        QCOMPARE(watcher->service, QStringLiteral("org.devl0rd.ProcessMonitor.FrameTelemetry"));
        QVERIFY(watcher->enabled);
        for (const QVariant &frametime : {QVariant(7.5), QVariant(0), QVariant(2500), QVariant(8)}) {
            QMetaObject::invokeMethod(watcher, "dbusFrame", Q_ARG(QVariant, 200), Q_ARG(QVariant, frametime));
        }
        QMetaObject::invokeMethod(watcher, "dbusFrame", Q_ARG(QVariant, 0), Q_ARG(QVariant, 5));
        QCOMPARE(harness->eval(QStringLiteral("frametimesFor(200)")).toList(), (QVariantList {7.5, 8}));
        QCOMPARE(harness->eval(QStringLiteral("frametimesFor(999)")).toList(), QVariantList());
        harness->unload();
        QCOMPARE(SessionBusDouble::calls().constLast().value(QStringLiteral("arguments")).toList(), QVariantList {QStringLiteral("[]")});
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void thePanelReadsTheSmallSnapshotUntilThePopupOpens()
    {
        auto harness = started(Form::Horizontal);
        QVERIFY(harness);
        QObject *root = harness->root();
        QVERIFY(root->property("compactOnly").toBool());
        QVERIFY(harness->deliver(
            runtime() + QStringLiteral("/panel/panel.json"), panel, [root] { return root->property("hasData").toBool(); }));
        QCOMPARE(harness->eval(QStringLiteral("focusProc.name")).toString(), QStringLiteral("game"));
        QCOMPARE(harness->eval(QStringLiteral("summary.count")).toInt(), 5);
        QCOMPARE(harness->eval(QStringLiteral("rows.count")).toInt(), 0);
        root->setProperty("expanded", true);
        QVERIFY(!root->property("compactOnly").toBool());
        QVERIFY(feed(*harness));
        QVERIFY(rowsBecome(*harness, {100, 300}));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void runsProcessActionsWithQuotedArguments()
    {
        auto harness = started(Form::Planar);
        QVERIFY(harness);
        harness->eval(QStringLiteral("signalProc(200, 'TERM')"));
        QCOMPARE(harness->command(QStringLiteral("kill ")), QStringLiteral("kill -TERM 200"));
        harness->eval(QStringLiteral("forceKillAsRoot(200)"));
        QCOMPARE(harness->command(QStringLiteral("pkexec")), QStringLiteral("pkexec kill -9 200"));
        harness->eval(QStringLiteral("openJournal(\"it's\")"));
        QCOMPARE(harness->command(QStringLiteral("konsole")), QStringLiteral("konsole -e journalctl _COMM='it'\\''s' -e"));
        harness->eval(QStringLiteral("openLocation(200)"));
        QVERIFY(harness->command(QStringLiteral("sh -c")).contains(QStringLiteral("/proc/200/exe")));
        harness->eval(QStringLiteral("restartProc(200)"));
        QVERIFY(harness->command(QStringLiteral("bash -c")).startsWith(QStringLiteral("bash -c 'p=200; mapfile")));
        harness->eval(QStringLiteral("copyCmdline(200)"));
        QVERIFY(harness->reply(QStringLiteral("tr '\\0' ' ' < /proc/200/cmdline"), QStringLiteral("game --fullscreen \n")));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void formatsAndColoursValues()
    {
        auto harness = started(Form::Planar);
        QVERIFY(harness);
        QCOMPARE(harness
                     ->eval(QStringLiteral("[fmtValue(12.6, 'pct'), fmtValue(0, 'bytes'), fmtValue(2048, 'bytes'), fmtValue(0, 'rate'), "
                                           "fmtValue(2048, 'rate'), fmtValue(7, 'int')]"))
                     .toStringList(),
            (QStringList {QStringLiteral("13%"), QStringLiteral("—"), QStringLiteral("2 KB"), QStringLiteral("—"), QStringLiteral("2 KB/s"),
                QStringLiteral("7")}));
        QCOMPARE(harness
                     ->eval(QStringLiteral(
                         "[fmtCol({}, colOf('fps')), fmtCol({fps: 59.6}, colOf('fps')), fmtCol({cpu: 3, acpu: 9}, colOf('cpu'))]"))
                     .toStringList(),
            (QStringList {QStringLiteral("—"), QStringLiteral("60"), QStringLiteral("9%")}));
        harness->setConfig(QStringLiteral("aggregateChildren"), false);
        QCOMPARE(harness->eval(QStringLiteral("fmtCol({cpu: 3, acpu: 9}, colOf('cpu'))")).toString(), QStringLiteral("3%"));
        QVERIFY(harness
                ->eval(
                    QStringLiteral("fpsColor(60) === Kirigami.Theme.positiveTextColor && fpsColor(30) === Kirigami.Theme.neutralTextColor "
                                   "&& fpsColor(29) === Kirigami.Theme.negativeTextColor"))
                .toBool());
        QVERIFY(harness->eval(QStringLiteral("heatColor(0) === Kirigami.Theme.textColor && heatColor(50) !== Kirigami.Theme.textColor"))
                .toBool());
        harness->setConfig(QStringLiteral("colorizeUsage"), false);
        QVERIFY(harness->eval(QStringLiteral("heatColor(90) === Kirigami.Theme.textColor")).toBool());
        QCOMPARE(
            harness->eval(QStringLiteral("[graphMax('cpu'), graphMax('fps'), graphMax('threads')]")).toList(), (QVariantList {100, 0, 0}));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void choosesColumnsFromTheConfiguration()
    {
        auto harness = started(Form::Planar, {{QStringLiteral("showGpuColumn"), false}, {QStringLiteral("showPidColumn"), true}});
        QVERIFY(harness);
        QCOMPARE(harness->eval(QStringLiteral("columns.map(c => c.key)")).toStringList(),
            (QStringList {QStringLiteral("cpu"), QStringLiteral("ram"), QStringLiteral("fps"), QStringLiteral("pid")}));
        harness->setConfig(QStringLiteral("showFpsColumn"), false);
        QCOMPARE(harness->eval(QStringLiteral("columns.length")).toInt(), 3);
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void middleClickRevealsTheFocusedProcess()
    {
        auto harness = started(Form::Horizontal);
        QVERIFY(harness);
        harness->eval(QStringLiteral("middleClick()"));
        QVERIFY(!harness->root()->property("expanded").toBool());
        QObject *root = harness->root();
        QVERIFY(harness->deliver(
            runtime() + QStringLiteral("/panel/panel.json"), panel, [root] { return root->property("hasData").toBool(); }));
        harness->eval(QStringLiteral("middleClick()"));
        QVERIFY(root->property("expanded").toBool());
        QCOMPARE(root->property("searchText").toString(), QStringLiteral("200"));
        QCOMPARE(root->property("expandedPid").toInt(), 200);
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void overlayHostWatchesTheOverlaidProcesses()
    {
        const PlasmoidSpec host {QStringLiteral("process-monitor/plasmoids/org.devl0rd.procmon.overlay"),
            QStringLiteral("org.devl0rd.procmon.overlay"), QStringLiteral("utilities-system-monitor"), {}};
        auto harness = started(Form::Planar, {}, host);
        QVERIFY(harness);
        QObject *root = harness->root();
        QCOMPARE(harness->plasmoid()->status, 6);
        QVERIFY(!root->property("dataWanted").toBool());
        const QString state = harness->runtimePath(QStringLiteral("Konveyor-Monitor-Overlay/state.json"));
        QVERIFY(harness->deliver(state,
            R"({"targets": [{"key": "game", "pid": 200, "windowId": 3, "x": 0, "y": 0, "width": 900, "height": 600}]})",
            [root] { return root->property("overlayVisible").toBool(); }));
        QVERIFY(feed(*harness));
        QTRY_COMPARE(harness->eval(QStringLiteral("overlayProcByPid[200] ? overlayProcByPid[200].fps : 0")).toInt(), 144);
        QCOMPARE(harness->eval(QStringLiteral("frametimeWatchPids")).toList(), QVariantList {200});
        QVERIFY(harness->deliver(state, R"({"targets": []})", [root] { return !root->property("overlayVisible").toBool(); }));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }
};

QTEST_MAIN(TestPlasmoidProcmon)

#include "test_plasmoid_procmon.moc"
