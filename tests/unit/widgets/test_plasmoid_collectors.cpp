#include "procmonfixture.h"
#include "routerfixture.h"

#include <QElapsedTimer>

namespace
{

const QString procmonRestart = QStringLiteral("$HOME/.local/bin/procmon-collect --restart");
const QString routerRestart = QStringLiteral("$HOME/.local/bin/routermon-collect --restart");

qsizetype restarts(const QString &command)
{
    return DataSourceDouble::commands().count(command);
}

QString status(PlasmoidHarness &harness)
{
    QObject *shell = harness.findAll("PopupShell").value(0);
    return shell ? shell->property("statusText").toString() : QString();
}

void keepWriting(PlasmoidHarness &harness, const QString &path, const QByteArray &json, int forMs)
{
    QElapsedTimer elapsed;
    elapsed.start();
    while (elapsed.elapsed() < forMs) {
        harness.writeFile(path, json);
        QTest::qWait(300);
    }
}

}

class TestPlasmoidCollectors : public QObject
{
    Q_OBJECT

public:
    static void initMain() { PlasmoidHarness::prepareEnvironment(); }

private Q_SLOTS:
    void cleanup()
    {
        QFile::remove(Procmon::runtime() + QStringLiteral("/data.json"));
        QFile::remove(Procmon::runtime() + QStringLiteral("/panel/panel.json"));
        QFile::remove(Router::runtime() + QStringLiteral("/data.json"));
    }

    void procmonRestartsAStalledCollectorOncePerStall()
    {
        auto harness = Procmon::started(Form::Planar, {{QStringLiteral("updateInterval"), 500}});
        QVERIFY(harness);
        QObject *root = harness->root();
        QVERIFY(Procmon::feed(*harness));
        QVERIFY(!root->property("collectorStale").toBool());
        QCOMPARE(status(*harness), QStringLiteral("Live"));
        const qsizetype before = restarts(procmonRestart);
        QTRY_VERIFY_WITH_TIMEOUT(root->property("collectorStale").toBool(), 5000);
        QCOMPARE(status(*harness), QStringLiteral("Collector stopped"));
        QCOMPARE(harness->eval(QStringLiteral("tooltipText()")).toString(), QStringLiteral("Collector stopped"));
        QCOMPARE(restarts(procmonRestart), before + 1);
        QTest::qWait(2500);
        QCOMPARE(restarts(procmonRestart), before + 1);
        QVERIFY(harness->reply(procmonRestart, QString()));
        QVERIFY(Procmon::feed(*harness));
        QVERIFY(!root->property("collectorStale").toBool());
        QCOMPARE(status(*harness), QStringLiteral("Live"));
        QTRY_COMPARE_WITH_TIMEOUT(restarts(procmonRestart), before + 2, 5000);
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void procmonRestartsACollectorThatNeverWrote()
    {
        auto harness = Procmon::started(Form::Planar, {{QStringLiteral("updateInterval"), 500}});
        QVERIFY(harness);
        const qsizetype before = restarts(procmonRestart);
        QTRY_VERIFY_WITH_TIMEOUT(harness->root()->property("collectorStale").toBool(), 5000);
        QCOMPARE(status(*harness), QStringLiteral("Collector stopped"));
        QCOMPARE(restarts(procmonRestart), before + 1);
        QVERIFY(harness->reply(procmonRestart, QString()));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void procmonPanelHearsTheFullSnapshotBetweenPanelChanges()
    {
        auto harness = Procmon::started(Form::Horizontal, {{QStringLiteral("updateInterval"), 500}});
        QVERIFY(harness);
        QObject *root = harness->root();
        QVERIFY(root->property("compactOnly").toBool());
        QQuickItem *compact = harness->show("compactRepresentation");
        QVERIFY(compact);
        QVERIFY(harness->deliver(Procmon::runtime() + QStringLiteral("/panel/panel.json"), Procmon::panel,
            [root] { return root->property("hasData").toBool(); }));
        keepWriting(*harness, Procmon::runtime() + QStringLiteral("/data.json"), Procmon::snapshot, 3500);
        QVERIFY(!root->property("collectorStale").toBool());
        QCOMPARE(compact->opacity(), 1.0);
        QTRY_VERIFY_WITH_TIMEOUT(root->property("collectorStale").toBool(), 5000);
        QCOMPARE(compact->opacity(), 0.5);
        QVERIFY(harness->reply(procmonRestart, QString()));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void anOverlayHostWithNothingToShowDoesNotWatch()
    {
        const PlasmoidSpec overlay {QStringLiteral("process-monitor/plasmoids/org.devl0rd.procmon.overlay"),
            QStringLiteral("org.devl0rd.procmon.overlay"), QStringLiteral("utilities-system-monitor"), {}};
        auto harness = Procmon::started(Form::Planar, {{QStringLiteral("updateInterval"), 500}}, overlay);
        QVERIFY(harness);
        const qsizetype before = restarts(procmonRestart);
        QTest::qWait(3000);
        QVERIFY(!harness->root()->property("collectorStale").toBool());
        QCOMPARE(restarts(procmonRestart), before);
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void routerMarksAStalledCollectorAndRestartsIt()
    {
        auto harness = Router::started(Form::Horizontal);
        QVERIFY(harness);
        QObject *root = harness->root();
        QCOMPARE(harness->eval(QStringLiteral("routerData.staleAfter")).toInt(), 60000);
        harness->eval(QStringLiteral("routerData.staleAfter = 1500"));
        QVERIFY(Router::feed(*harness));
        QCOMPARE(root->property("routerState").toString(), QStringLiteral("ok"));
        const qsizetype before = restarts(routerRestart);
        QTRY_COMPARE_WITH_TIMEOUT(root->property("routerState").toString(), QStringLiteral("stale"), 5000);
        QCOMPARE(root->property("stateText").toString(), QStringLiteral("Collector stopped"));
        QCOMPARE(harness->eval(QStringLiteral("tooltipText()")).toString(), QStringLiteral("Collector stopped"));
        QCOMPARE(restarts(routerRestart), before + 1);
        QVERIFY(harness->reply(routerRestart, QString()));
        QVERIFY(Router::feedState(*harness, Router::changed({{QStringLiteral("paused"), true}}), "paused", true));
        QCOMPARE(root->property("routerState").toString(), QStringLiteral("paused"));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }
};

QTEST_MAIN(TestPlasmoidCollectors)

#include "test_plasmoid_collectors.moc"
