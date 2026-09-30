#include "logmonfixture.h"

#include <QRegularExpression>
#include <QTest>

using Logmon::feed;
using Logmon::logPath;
using Logmon::rowField;
using Logmon::snapshot;
using Logmon::started;

namespace
{

const QString journalPrefix = QStringLiteral("journalctl -o json");

QString cursorFile(const QString &command)
{
    return QRegularExpression(QStringLiteral("--cursor-file=([^\\s;]+)")).match(command).captured(1);
}

QString record(qint64 micros, const QString &cursor, const QString &message)
{
    return QStringLiteral("{\"__CURSOR\": \"%1\", \"__REALTIME_TIMESTAMP\": \"%2\", \"PRIORITY\": \"3\", \"MESSAGE\": \"%3\"}\n")
        .arg(cursor, QString::number(micros), message);
}

}

class TestPlasmoidLogmon : public QObject
{
    Q_OBJECT

public:
    static void initMain() { PlasmoidHarness::prepareEnvironment(); }

private Q_SLOTS:
    void cleanup() { QFile::remove(logPath()); }

    void showsTheJournalOnTheDesktop()
    {
        auto harness = started(Form::Planar);
        QVERIFY(harness);
        QVERIFY(feed(*harness));
        QObject *root = harness->root();
        QCOMPARE(root->property("stateKey").toString(), QStringLiteral("live"));
        QCOMPARE(rowField(*harness, "app"),
            (QStringList {QStringLiteral("kernel"), QStringLiteral("sshd"), QStringLiteral("kwin_wayland"), QStringLiteral("kded6")}));
        QCOMPARE(rowField(*harness, "msg").constLast(), QStringLiteral("red text"));
        QCOMPARE(root->property("toolTipSubText").toString(), QStringLiteral("No new errors or warnings since you last looked"));
        QCOMPARE(harness->plasmoid()->icon, QStringLiteral("utilities-log-viewer"));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void countsNewProblemsInThePanelUntilOpened()
    {
        auto harness = started(Form::Horizontal);
        QVERIFY(harness);
        QObject *root = harness->root();
        const QByteArray first = snapshot();
        QVERIFY(feed(*harness, first));
        QCOMPARE(root->property("newErrors").toInt(), 1);
        QCOMPARE(root->property("newWarnings").toInt(), 1);
        QCOMPARE(root->property("toolTipSubText").toString(), QStringLiteral("Since you last looked: 1 error, 1 warning"));
        root->setProperty("expanded", true);
        QCOMPARE(root->property("newErrors").toInt(), 0);
        QTRY_COMPARE(rowField(*harness, "app").size(), 4);
        root->setProperty("expanded", false);
        QCOMPARE(harness->config(QStringLiteral("lastSeen")).toString(), QString::number(root->property("countedT").toDouble(), 'g', 16));
        QVERIFY(feed(*harness, first));
        QCOMPARE(root->property("newErrors").toInt(), 0);
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void keepsCountingNewProblemsAfterTheClockIsSetBack()
    {
        auto harness = started(Form::Horizontal);
        QVERIFY(harness);
        QObject *root = harness->root();
        const QJsonObject before = Logmon::line(-3600000, 6, QStringLiteral("kernel"), QStringLiteral("before the clock went back"));
        QVERIFY(feed(*harness, Logmon::journalOf({before})));
        QCOMPARE(root->property("newErrors").toInt(), 0);
        QVERIFY(feed(*harness,
            Logmon::journalOf({before, Logmon::line(1000, 3, QStringLiteral("sshd"), QStringLiteral("after the clock went back"))})));
        QCOMPARE(root->property("newErrors").toInt(), 1);
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void summarizesSourcesAndActivity()
    {
        auto harness = started(Form::Planar);
        QVERIFY(harness);
        QVERIFY(feed(*harness));
        harness->eval(QStringLiteral("refreshSummary()"));
        QObject *root = harness->root();
        QCOMPARE(root->property("warningCount").toInt(), 2);
        QCOMPARE(root->property("errorCount").toInt(), 1);
        QCOMPARE(root->property("linesPerMinute").toInt(), 3);
        QCOMPARE(harness->eval(QStringLiteral("activity.reduce((a, b) => a + b, 0)")).toInt(), 4);
        QCOMPARE(harness->eval(QStringLiteral("activityAlerts.reduce((a, b) => a + b, 0)")).toInt(), 2);
        QCOMPARE(harness->eval(QStringLiteral("topSources.count")).toInt(), 4);
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void mutesAndUnmutesApps()
    {
        auto harness = started(Form::Planar);
        QVERIFY(harness);
        QVERIFY(feed(*harness));
        harness->eval(QStringLiteral("muteApp('sshd')"));
        QCOMPARE(harness->config(QStringLiteral("mutedApps")).toString(), QStringLiteral("sshd"));
        QCOMPARE(harness->plasmoid()->configuration()->writes, 1);
        QTRY_COMPARE(
            rowField(*harness, "app"), (QStringList {QStringLiteral("kernel"), QStringLiteral("kwin_wayland"), QStringLiteral("kded6")}));
        QCOMPARE(harness->root()->property("errorCount").toInt(), 0);
        harness->eval(QStringLiteral("muteApp('KDED')"));
        QCOMPARE(harness->config(QStringLiteral("mutedApps")).toString(), QStringLiteral("sshd, KDED"));
        QTRY_COMPARE(rowField(*harness, "app"), (QStringList {QStringLiteral("kernel"), QStringLiteral("kwin_wayland")}));
        harness->eval(QStringLiteral("unmuteApp('sshd')"));
        QTRY_COMPARE(rowField(*harness, "app").size(), 3);
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void searchesTheJournalForALevel()
    {
        auto harness = started(Form::Planar);
        QVERIFY(harness);
        QVERIFY(feed(*harness));
        harness->root()->setProperty("level", 3);
        QTRY_VERIFY(harness->command(journalPrefix).contains(QLatin1String(" -p 3 $(test -e ")));
        QVERIFY(harness->command(journalPrefix).contains(QLatin1String("printf -- '-n %s' 5000)")));
        QVERIFY(harness->root()->property("querying").toBool());
        const QString records
            = QStringLiteral("{\"__CURSOR\": \"c2000000\", \"__REALTIME_TIMESTAMP\": \"2000000\", \"PRIORITY\": \"3\", "
                             "\"SYSLOG_IDENTIFIER\": \"sshd\", \"MESSAGE\": \"bad\", \"_PID\": "
                             "\"7\"}\n"
                             "{\"__CURSOR\": \"c1000000\", \"__REALTIME_TIMESTAMP\": \"1000000\", \"PRIORITY\": \"2\", \"_TRANSPORT\": "
                             "\"kernel\", \"MESSAGE\": [104, 105]}\n"
                             "{\"__CURSOR\": \"c3000000\", \"__REALTIME_TIMESTAMP\": \"3000000\", \"_SYSTEMD_UNIT\": \"foo.service\", "
                             "\"MESSAGE\": \"unit\"}\n"
                             "{\"__CURSOR\": \"c4000000\", \"__REALTIME_TIMESTAMP\": \"4000000\", \"MESSAGE\": \"nobody\"}\nnot json\n");
        QVERIFY(harness->reply(journalPrefix, records));
        QCOMPARE(rowField(*harness, "app"),
            (QStringList {QStringLiteral("kernel"), QStringLiteral("sshd"), QStringLiteral("foo"), QStringLiteral("?")}));
        QCOMPARE(rowField(*harness, "msg").first(), QStringLiteral("hi"));
        QCOMPARE(
            rowField(*harness, "prio"), (QStringList {QStringLiteral("2"), QStringLiteral("3"), QStringLiteral("6"), QStringLiteral("6")}));
        QCOMPARE(rowField(*harness, "time").first(), QStringLiteral("00:00:01"));
        QVERIFY(!harness->root()->property("querying").toBool());
        harness->root()->setProperty("level", 2);
        QTRY_VERIFY(harness->command(journalPrefix).contains(QLatin1String(" -p 4 ")));
        const QString encoded
            = QStringLiteral("{\"__CURSOR\": \"c5000000\", \"__REALTIME_TIMESTAMP\": \"5000000\", \"PRIORITY\": \"3\", \"MESSAGE\": "
                             "[226, 156, 147, 32, 111, 108, 195, 169, 32, 27, 91, 49, 109, 98, 27, 91, 48, 109, 32, 255, 33]}\n");
        QVERIFY(harness->reply(journalPrefix, encoded));
        QCOMPARE(rowField(*harness, "msg").constLast(), QStringLiteral("\u2713 ol\u00e9 b \ufffd!"));
        harness->root()->setProperty("level", 3);
        QTRY_VERIFY(harness->command(journalPrefix).contains(QLatin1String(" -p 3 ")));
        QVERIFY(harness->reply(journalPrefix,
            QStringLiteral("{\"__CURSOR\": \"c6\", \"__REALTIME_TIMESTAMP\": \"6000000\", \"PRIORITY\": [\"3\", \"6\"], "
                           "\"SYSLOG_IDENTIFIER\": [\"app\", \"helper\"], \"_PID\": [\"7\", \"8\"], "
                           "\"MESSAGE\": [\"first\", [115, 195, 182]]}\n")));
        QCOMPARE(rowField(*harness, "msg").constLast(), QStringLiteral("first\ns\u00f6"));
        QCOMPARE(rowField(*harness, "app").constLast(), QStringLiteral("app, helper"));
        QCOMPARE(rowField(*harness, "pid").constLast(), QStringLiteral("7, 8"));
        QCOMPARE(rowField(*harness, "prio").constLast(), QStringLiteral("3"));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void searchKeepsShowingNewEntriesAfterTheClockIsSetBack()
    {
        auto harness = started(Form::Planar);
        QVERIFY(harness);
        QVERIFY(feed(*harness));
        harness->root()->setProperty("level", 3);
        QTRY_VERIFY(!harness->command(journalPrefix).isEmpty());
        const QString first = harness->command(journalPrefix);
        QVERIFY(!cursorFile(first).isEmpty());
        QVERIFY(harness->reply(journalPrefix, record(9000000000000000, QStringLiteral("s=1;i=1"), QStringLiteral("before"))));
        harness->eval(QStringLiteral("runSearch(false)"));
        const QString refresh = harness->command(journalPrefix);
        QVERIFY(!refresh.contains(QLatin1String("--since")));
        QCOMPARE(cursorFile(refresh), cursorFile(first));
        QVERIFY(harness->reply(journalPrefix,
            record(1000000, QStringLiteral("s=1;i=2"), QStringLiteral("after"))
                + record(1000000, QStringLiteral("s=1;i=2"), QStringLiteral("after"))));
        QCOMPARE(rowField(*harness, "msg"), (QStringList {QStringLiteral("before"), QStringLiteral("after")}));
        harness->root()->setProperty("level", 2);
        QTRY_VERIFY(harness->command(journalPrefix).contains(QLatin1String(" -p 4 ")));
        QVERIFY(cursorFile(harness->command(journalPrefix)) != cursorFile(first));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void searchesTheJournalForText()
    {
        auto harness = started(Form::Planar);
        QVERIFY(harness);
        QVERIFY(feed(*harness));
        harness->root()->setProperty("search", QStringLiteral("it's a.b"));
        QTRY_VERIFY(harness->command(QStringLiteral("TIDS=")).contains(QLatin1String("--grep 'it'\\''s a\\.b' $(test -e ")));
        QVERIFY(harness->command(QStringLiteral("TIDS=")).contains(QLatin1String("grep -iF -- 'it'\\''s a.b'")));
        QVERIFY(harness->reply(QStringLiteral("TIDS="), QString()));
        QCOMPARE(harness->eval(QStringLiteral("rows.count")).toInt(), 0);
        harness->root()->setProperty("search", QString());
        QTRY_COMPARE(rowField(*harness, "app").size(), 4);
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void showsAStoppedCollector_data()
    {
        QTest::addColumn<qint64>("age");
        QTest::addColumn<bool>("alive");
        QTest::newRow("stale") << qint64(60) << true;
        QTest::newRow("dead") << qint64(0) << false;
    }

    void showsAStoppedCollector()
    {
        QFETCH(qint64, age);
        QFETCH(bool, alive);
        auto harness = started(Form::Horizontal);
        QVERIFY(harness);
        QVERIFY(feed(*harness, snapshot(age, alive)));
        QCOMPARE(harness->root()->property("stateKey").toString(), QStringLiteral("offline"));
        QCOMPARE(harness->root()->property("toolTipSubText").toString(), QStringLiteral("Collector not running"));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void noticesWhenHeartbeatsStop()
    {
        auto harness = started(Form::Horizontal);
        QVERIFY(harness);
        QVERIFY(feed(*harness, snapshot(12)));
        QTRY_COMPARE_WITH_TIMEOUT(harness->root()->property("stateKey").toString(), QStringLiteral("offline"), 10000);
        QVERIFY(feed(*harness));
        QCOMPARE(harness->root()->property("stateKey").toString(), QStringLiteral("live"));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void pausesOnMiddleClick()
    {
        auto harness = started(Form::Planar);
        QVERIFY(harness);
        QVERIFY(feed(*harness));
        harness->eval(QStringLiteral("middleClick()"));
        QCOMPARE(harness->root()->property("stateKey").toString(), QStringLiteral("paused"));
        QVERIFY(harness->eval(QStringLiteral("logData.paused")).toBool());
        harness->setConfig(QStringLiteral("middleClickPause"), false);
        harness->eval(QStringLiteral("middleClick()"));
        QVERIFY(harness->root()->property("paused").toBool());
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void keepsOnlyTheNewestRows()
    {
        auto harness = started(Form::Planar, {{QStringLiteral("maxRows"), 2}});
        QVERIFY(harness);
        QVERIFY(feed(*harness));
        QCOMPARE(rowField(*harness, "app"), (QStringList {QStringLiteral("kwin_wayland"), QStringLiteral("kded6")}));
        harness->eval(QStringLiteral("clearLog()"));
        QCOMPARE(harness->eval(QStringLiteral("rows.count")).toInt(), 0);
        QVERIFY(feed(*harness));
        QCOMPARE(rowField(*harness, "app"), QStringList {QStringLiteral("kded6")});
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }

    void copiesExpandsAndAsks()
    {
        auto harness = started(Form::Planar);
        QVERIFY(harness);
        QVERIFY(feed(*harness));
        QCOMPARE(harness->eval(QStringLiteral("lineText(rows.get(1))")).toString().section(QLatin1Char(' '), 2),
            QStringLiteral("sshd[12]: Failed password\nsecond line"));
        harness->eval(QStringLiteral("toggleExpand(1)"));
        QVERIFY(harness->eval(QStringLiteral("rows.get(1).expanded")).toBool());
        harness->eval(QStringLiteral("toggleExpand(99)"));
        harness->eval(QStringLiteral("askClaude('10:00', 'sshd', '12', \"it's bad\", 3)"));
        const QString command = harness->command(QStringLiteral("konsole"));
        QVERIFY(command.startsWith(QLatin1String("konsole --workdir \"$HOME\" -e claude 'I saw this entry")));
        QVERIFY(command.contains(QLatin1String("message: it'\\''s bad")));
        QVERIFY2(PlasmoidHarness::messages().isEmpty(), qPrintable(PlasmoidHarness::report()));
    }
};

QTEST_MAIN(TestPlasmoidLogmon)

#include "test_plasmoid_logmon.moc"
