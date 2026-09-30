#include "procmonfixture.h"

using Procmon::feed;
using Procmon::rowPids;
using Procmon::rowsBecome;
using Procmon::runtime;
using Procmon::started;

namespace
{

QByteArray manyProcesses(int count)
{
    QByteArray procs;
    for (int pid = 1000; pid < 1000 + count; ++pid) {
        procs += (procs.isEmpty() ? "" : ",") + QByteArray("{\"pid\": ") + QByteArray::number(pid) + ", \"ppid\": 1, \"name\": \"worker"
            + QByteArray::number(pid) + "\", \"cpu\": " + QByteArray::number(pid - 999) + "}";
    }
    return "{\"ts\": 2, \"ncpu\": 4, \"mem_total\": 1, \"vram_total\": 0, \"procs\": [" + procs + "]}";
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

class TestPlasmoidProcmonRows : public QObject
{
    Q_OBJECT

public:
    static void initMain() { PlasmoidHarness::prepareEnvironment(); }

private Q_SLOTS:
    void cleanup() { QFile::remove(runtime() + QStringLiteral("/data.json")); }

    void listsProcessesForEachConfiguration_data()
    {
        QTest::addColumn<QVariantMap>("config");
        QTest::addColumn<QVariantList>("pids");
        const auto key = [](const char *name) { return QString::fromLatin1(name); };
        QTest::newRow("tree") << QVariantMap() << QVariantList {100, 300};
        QTest::newRow("flat") << QVariantMap {{key("treeView"), false}} << QVariantList {100, 200, 300};
        QTest::newRow("kernel threads") << QVariantMap {{key("treeView"), false}, {key("showKernelThreads"), true}}
                                        << QVariantList {100, 200, 300, 2};
        QTest::newRow("systemd") << QVariantMap {{key("treeView"), false}, {key("hideSystemd"), false}} << QVariantList {1, 100, 200, 300};
        QTest::newRow("systemd tree") << QVariantMap {{key("hideSystemd"), false}} << QVariantList {1};
        QTest::newRow("own usage") << QVariantMap {{key("treeView"), false}, {key("aggregateChildren"), false}}
                                   << QVariantList {200, 100, 300};
        QTest::newRow("gpu filter") << QVariantMap {{key("processFilter"), key("gpu")}} << QVariantList {100, 200};
        QTest::newRow("apps filter") << QVariantMap {{key("processFilter"), key("apps")}} << QVariantList {100, 200};
        QTest::newRow("disk filter") << QVariantMap {{key("processFilter"), key("disk")}} << QVariantList {200};
        QTest::newRow("ascending") << QVariantMap {{key("sortDescending"), false}} << QVariantList {300, 100};
        QTest::newRow("by pid") << QVariantMap {{key("sortColumn"), key("pid")}} << QVariantList {300, 100};
        QTest::newRow("by name") << QVariantMap {{key("sortColumn"), key("name")}, {key("sortDescending"), false}}
                                 << QVariantList {300, 100};
    }

    void listsProcessesForEachConfiguration()
    {
        QFETCH(QVariantMap, config);
        QFETCH(QVariantList, pids);
        auto harness = fed(config);
        QVERIFY(harness);
        QVERIFY2(rowsBecome(*harness, pids), qPrintable(QVariant(rowPids(*harness)).toStringList().join(QLatin1Char(','))));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void followsConfigurationChangesLive()
    {
        auto harness = fed();
        QVERIFY(harness);
        QVERIFY(rowsBecome(*harness, {100, 300}));
        harness->setConfig(QStringLiteral("treeView"), false);
        QVERIFY(rowsBecome(*harness, {100, 200, 300}));
        harness->setConfig(QStringLiteral("hideSystemd"), false);
        QVERIFY(rowsBecome(*harness, {1, 100, 200, 300}));
        harness->setConfig(QStringLiteral("showKernelThreads"), true);
        QVERIFY(rowsBecome(*harness, {1, 100, 200, 300, 2}));
        harness->setConfig(QStringLiteral("processFilter"), QStringLiteral("disk"));
        QVERIFY(rowsBecome(*harness, {200}));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void expandsAndCollapsesTheTree()
    {
        auto harness = fed();
        QVERIFY(harness);
        QVERIFY(rowsBecome(*harness, {100, 300}));
        harness->eval(QStringLiteral("toggleTree(100)"));
        QVERIFY(rowsBecome(*harness, {100, 200, 300}));
        QCOMPARE(
            harness->eval(QStringLiteral("[rows.get(0).hasChildren, rows.get(0).expanded, rows.get(1).depth, rows.get(2).hasChildren]"))
                .toList(),
            (QVariantList {true, true, 1, false}));
        harness->eval(QStringLiteral("toggleTree(100)"));
        QVERIFY(rowsBecome(*harness, {100, 300}));
        harness->eval(QStringLiteral("toggleTree(100)"));
        QVERIFY(rowsBecome(*harness, {100, 200, 300}));
        harness->eval(QStringLiteral("collapseAll()"));
        QVERIFY(rowsBecome(*harness, {100, 300}));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void searchesByNameAndPid_data()
    {
        QTest::addColumn<QString>("query");
        QTest::addColumn<QVariantList>("pids");
        QTest::newRow("name") << QStringLiteral("ame") << QVariantList {200};
        QTest::newRow("case") << QStringLiteral("SHELL") << QVariantList {100};
        QTest::newRow("pid") << QStringLiteral("300") << QVariantList {300};
        QTest::newRow("padded") << QStringLiteral("  game ") << QVariantList {200};
        QTest::newRow("hidden systemd") << QStringLiteral("systemd") << QVariantList {1};
        QTest::newRow("kernel") << QStringLiteral("kthreadd") << QVariantList {};
        QTest::newRow("nothing") << QStringLiteral("zzz") << QVariantList {};
    }

    void searchesByNameAndPid()
    {
        QFETCH(QString, query);
        QFETCH(QVariantList, pids);
        auto harness = fed();
        QVERIFY(harness);
        QVERIFY(rowsBecome(*harness, {100, 300}));
        harness->root()->setProperty("searchText", query);
        QVERIFY2(rowsBecome(*harness, pids), qPrintable(QVariant(rowPids(*harness)).toStringList().join(QLatin1Char(','))));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void sortsFromTheColumnHeaders()
    {
        auto harness = fed({{QStringLiteral("treeView"), false}});
        QVERIFY(harness);
        QVERIFY(rowsBecome(*harness, {100, 200, 300}));
        harness->eval(QStringLiteral("headerSort('cpu')"));
        QCOMPARE(harness->config(QStringLiteral("sortDescending")).toBool(), false);
        QVERIFY(rowsBecome(*harness, {300, 200, 100}));
        harness->eval(QStringLiteral("headerSort('name')"));
        QCOMPARE(harness->config(QStringLiteral("sortColumn")).toString(), QStringLiteral("name"));
        QVERIFY(rowsBecome(*harness, {200, 300, 100}));
        harness->eval(QStringLiteral("headerSort('name')"));
        QVERIFY(rowsBecome(*harness, {100, 300, 200}));
        harness->eval(QStringLiteral("headerSort('ram')"));
        QCOMPARE(harness->config(QStringLiteral("sortDescending")).toBool(), true);
        QVERIFY(rowsBecome(*harness, {100, 200, 300}));
        QVERIFY(!harness->root()->property("sortSyncPending").toBool());
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void keepsRowDetailsForTheVisibleWindowOnly()
    {
        auto harness = started(Form::Planar, {{QStringLiteral("treeView"), false}});
        QVERIFY(harness);
        QVERIFY(feed(*harness, manyProcesses(150)));
        QTRY_COMPARE(harness->eval(QStringLiteral("rows.count")).toInt(), 150);
        QCOMPARE(harness->eval(QStringLiteral("rows.get(0).pid")).toInt(), 1149);
        QTRY_COMPARE(harness->eval(QStringLiteral("Object.keys(procByPid).length")).toInt(), 81);
        QVERIFY(harness->eval(QStringLiteral("procByPid[1149] !== undefined && procByPid[1000] === undefined")).toBool());
        QCOMPARE(harness->eval(QStringLiteral("summary.count")).toInt(), 150);
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void drawsTheProcessRows()
    {
        auto harness = fed();
        QVERIFY(harness);
        QVERIFY(rowsBecome(*harness, {100, 300}));
        QTRY_VERIFY(visibleTexts(harness->scene()).contains(QStringLiteral("plasmashell")));
        QVERIFY(harness->root()->property("rowsView").value<QObject *>());
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }
};

QTEST_MAIN(TestPlasmoidProcmonRows)

#include "test_plasmoid_procmon_rows.moc"
