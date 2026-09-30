#include "fakekonveyor.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>

#include <memory>

using Konveyor::Test::FakeKonveyor;
using Konveyor::Test::PrivateSession;
using Konveyor::Test::ProgramResult;

namespace
{

const QString windowsJson = QStringLiteral(
    R"([{"id":7,"title":"Konsole","app_id":"org.kde.konsole","pid":42,"workspace_id":3,"is_focused":true,"is_floating":false,)"
    R"("is_urgent":false,"layout":{"pos_in_scrolling_layout":[2,1],"tile_size":[960,1080]}}])");

}

class TestCliMsg : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        m_session = std::make_unique<PrivateSession>();
        QVERIFY(m_session->start());
    }

    void init()
    {
        m_konveyor = std::make_unique<FakeKonveyor>();
        QVERIFY(m_konveyor->start(m_session->address()));
    }

    void cleanup() { m_konveyor.reset(); }

    void cleanupTestCase() { m_session.reset(); }

    void queriesPrintPlainText_data()
    {
        QTest::addColumn<QString>("request");
        QTest::addColumn<QString>("method");
        QTest::addColumn<QString>("reply");
        QTest::addColumn<QString>("expected");
        QTest::newRow("windows") << "windows" << "Windows" << windowsJson
                                 << "Window ID 7: (focused)\n  Title: \"Konsole\"\n  App ID: \"org.kde.konsole\"\n  PID: 42\n"
                                    "  Workspace ID: 3\n  Scrolling position: column 2, tile 1\n  Tile size: 960 x 1080\n";
        QTest::newRow("workspaces") << "workspaces" << "Workspaces" << R"([{"idx":1,"output":"DP-1","is_focused":true,"name":"web"}])"
                                    << "Output \"DP-1\":\n * 1 \"web\"\n";
        QTest::newRow("outputs")
            << "outputs" << "Outputs"
            << R"([{"name":"DP-1","description":"Dell","logical":{"x":0,"y":0,"width":1920,"height":1080,"scale":1.5}}])"
            << "Output \"DP-1\" (Dell)\n  Logical position: 0, 0\n  Logical size: 1920x1080\n  Scale: 1.5\n";
        QTest::newRow("focused-output")
            << "focused-output" << "FocusedOutput"
            << R"([{"name":"HDMI-A-1","description":"TV","logical":{"x":1920,"y":0,"width":3840,"height":2160,"scale":2}}])"
            << "Output \"HDMI-A-1\" (TV)\n  Logical position: 1920, 0\n  Logical size: 3840x2160\n  Scale: 2\n";
        QTest::newRow("no focused output") << "focused-output" << "FocusedOutput" << "[]" << "No output is focused.\n";
        QTest::newRow("focused-window") << "focused-window" << "FocusedWindow" << windowsJson.mid(1, windowsJson.size() - 2)
                                        << "Window ID 7: (focused)\n  Title: \"Konsole\"\n  App ID: \"org.kde.konsole\"\n  PID: 42\n"
                                           "  Workspace ID: 3\n  Scrolling position: column 2, tile 1\n  Tile size: 960 x 1080\n";
        QTest::newRow("no focused window") << "focused-window" << "FocusedWindow" << "" << "No window is focused.\n";
        QTest::newRow("binds") << "binds" << "Binds" << R"([{"key":"Mod+H","action":{"name":"focus-column-left"}}])"
                               << QStringLiteral("Mod+H").leftJustified(32) + QStringLiteral("  focus-column-left\n");
        QTest::newRow("floating windows")
            << "windows" << "Windows"
            << R"([{"id":1,"title":"a","app_id":"x","pid":1,"workspace_id":1,"is_floating":true,"is_urgent":true,)"
               R"("layout":{"pos_in_scrolling_layout":null,"tile_size":[300,200.5]}},{"id":2,"title":"b","app_id":"y",)"
               R"("pid":2,"workspace_id":2,"layout":{}}])"
            << "Window ID 1: (floating, urgent)\n  Title: \"a\"\n  App ID: \"x\"\n  PID: 1\n  Workspace ID: 1\n"
               "  Tile size: 300 x 200.5\n\nWindow ID 2:\n  Title: \"b\"\n  App ID: \"y\"\n  PID: 2\n"
               "  Workspace ID: 2\n";
        QTest::newRow("workspaces on two outputs")
            << "workspaces" << "Workspaces"
            << R"([{"idx":1,"output":"DP-1","is_active":true,"name":null},{"idx":2,"output":"DP-1"},)"
               R"({"idx":1,"output":"DP-2","is_active":true,"is_focused":true}])"
            << "Output \"DP-1\":\n - 1\n   2\nOutput \"DP-2\":\n * 1\n";
        QTest::newRow("binds with titles and arguments")
            << "binds" << "Binds"
            << R"([{"key":"Mod+1","action":{"name":"focus-workspace","arguments":["1"]}},)"
               R"({"key":"Mod+T","title":"Terminal","action":{"name":"spawn","arguments":["konsole"]}}])"
            << QStringLiteral("Mod+1").leftJustified(32) + QStringLiteral("  focus-workspace 1\n")
                + QStringLiteral("Mod+T").leftJustified(32) + QStringLiteral("  Terminal\n");
        QTest::newRow("nothing open") << "windows" << "Windows" << "[]" << "\n";
        QTest::newRow("version") << "version" << "Version" << R"({"version":"1.2.3"})" << "1.2.3\n";
    }

    void queriesPrintPlainText()
    {
        QFETCH(QString, request);
        QFETCH(QString, method);
        QFETCH(QString, reply);
        QFETCH(QString, expected);
        m_konveyor->reply(method, reply);
        const ProgramResult result = konveyor({QStringLiteral("msg"), request});
        QCOMPARE(result.err, QString());
        QCOMPARE(result.exitCode, 0);
        QCOMPARE(result.out, expected);
        QCOMPARE(m_konveyor->calls(method).size(), 1);
    }

    void jsonPrintsTheReplyCompactly()
    {
        m_konveyor->reply(QStringLiteral("Windows"), QString::fromUtf8(QJsonDocument::fromJson(windowsJson.toUtf8()).toJson()));
        const ProgramResult result = konveyor({QStringLiteral("msg"), QStringLiteral("--json"), QStringLiteral("windows")});
        QCOMPARE(result.exitCode, 0);
        QCOMPARE(QJsonDocument::fromJson(result.out.toUtf8()), QJsonDocument::fromJson(windowsJson.toUtf8()));
        QCOMPARE(result.out.count(QLatin1Char('\n')), 1);

        m_konveyor->reply(QStringLiteral("Version"), QStringLiteral(R"({"version":"1.2.3"})"));
        QCOMPARE(konveyor({QStringLiteral("msg"), QStringLiteral("--json"), QStringLiteral("version")}).out,
            QStringLiteral("{\"version\":\"1.2.3\"}\n"));

        m_konveyor->reply(QStringLiteral("FocusedWindow"), QString());
        QCOMPARE(
            konveyor({QStringLiteral("msg"), QStringLiteral("--json"), QStringLiteral("focused-window")}).out, QStringLiteral("null\n"));
    }

    void invalidJsonFromKonveyorFails()
    {
        m_konveyor->reply(QStringLiteral("Windows"), QStringLiteral("[{"));
        const ProgramResult result = konveyor({QStringLiteral("msg"), QStringLiteral("windows")});
        QCOMPARE(result.exitCode, 1);
        QVERIFY(result.err.startsWith(QStringLiteral("Error: Konveyor sent invalid JSON")));
    }

    void actionSendsTheRequestAsJson()
    {
        m_konveyor->reply(QStringLiteral("Action"), QString());
        const ProgramResult result = konveyor({QStringLiteral("msg"), QStringLiteral("action"), QStringLiteral("move-column-to-workspace"),
            QStringLiteral("2"), QStringLiteral("--focus"), QStringLiteral("false"), QStringLiteral("--id"), QStringLiteral("9")});
        QCOMPARE(result.err, QString());
        QCOMPARE(result.exitCode, 0);
        const QList<QDBusMessage> calls = m_konveyor->calls(QStringLiteral("Action"));
        QCOMPARE(calls.size(), 1);
        const QJsonObject sent = QJsonDocument::fromJson(calls.first().arguments().value(0).toString().toUtf8()).object();
        QCOMPARE(sent.value(QStringLiteral("name")).toString(), QStringLiteral("move-column-to-workspace"));
        QCOMPARE(sent.value(QStringLiteral("arguments")).toArray().first().toString(), QStringLiteral("2"));
        QCOMPARE(sent.value(QStringLiteral("properties")).toObject().value(QStringLiteral("focus")).toString(), QStringLiteral("false"));
        QCOMPARE(sent.value(QStringLiteral("id")).toInteger(), 9);
    }

    void actionReportsKonveyorsError()
    {
        m_konveyor->reply(QStringLiteral("Action"), QStringLiteral("unknown action: fly"));
        const ProgramResult result = konveyor({QStringLiteral("msg"), QStringLiteral("action"), QStringLiteral("fly")});
        QCOMPARE(result.exitCode, 1);
        QCOMPARE(result.err, QStringLiteral("Error: unknown action: fly\n"));
    }

    void badActionArgumentsFailBeforeCalling()
    {
        m_konveyor->reply(QStringLiteral("Action"), QString());
        const ProgramResult missing = konveyor({QStringLiteral("msg"), QStringLiteral("action")});
        QCOMPARE(missing.exitCode, 1);
        QVERIFY(missing.err.startsWith(QStringLiteral("Error: ")));
        const ProgramResult badId = konveyor(
            {QStringLiteral("msg"), QStringLiteral("action"), QStringLiteral("close-window"), QStringLiteral("--id"), QStringLiteral("x")});
        QCOMPARE(badId.exitCode, 1);
        QCOMPARE(badId.err, QStringLiteral("Error: invalid window id: x\n"));
        QVERIFY(m_konveyor->calls(QStringLiteral("Action")).isEmpty());
    }

    void loadConfigFilePassesThePath()
    {
        m_konveyor->reply(QStringLiteral("LoadConfigFile"), QString());
        QCOMPARE(konveyor({QStringLiteral("msg"), QStringLiteral("load-config-file")}).exitCode, 0);
        QCOMPARE(konveyor({QStringLiteral("msg"), QStringLiteral("load-config-file"), QStringLiteral("/tmp/other.kdl")}).exitCode, 0);
        const QList<QDBusMessage> calls = m_konveyor->calls(QStringLiteral("LoadConfigFile"));
        QCOMPARE(calls.size(), 2);
        QCOMPARE(calls.at(0).arguments().value(0).toString(), QString());
        QCOMPARE(calls.at(1).arguments().value(0).toString(), QStringLiteral("/tmp/other.kdl"));

        m_konveyor->reply(QStringLiteral("LoadConfigFile"), QStringLiteral("config.kdl:3:1: unexpected token"));
        const ProgramResult failed = konveyor({QStringLiteral("msg"), QStringLiteral("load-config-file")});
        QCOMPARE(failed.exitCode, 1);
        QCOMPARE(failed.err, QStringLiteral("Error: config.kdl:3:1: unexpected token\n"));
    }

    void unknownRequestFails()
    {
        const ProgramResult result = konveyor({QStringLiteral("msg"), QStringLiteral("dance")});
        QCOMPARE(result.exitCode, 1);
        QCOMPARE(result.err, QStringLiteral("Error: unknown request: dance\n"));
    }

    void unknownOptionFails()
    {
        const ProgramResult result = konveyor({QStringLiteral("msg"), QStringLiteral("--yaml"), QStringLiteral("windows")});
        QCOMPARE(result.exitCode, 1);
        QVERIFY(result.err.contains(QStringLiteral("yaml")));
    }

    void msgWithoutARequestPrintsUsage()
    {
        const ProgramResult result = konveyor({QStringLiteral("msg")});
        QCOMPARE(result.exitCode, 0);
        QVERIFY(result.out.startsWith(QStringLiteral("Usage: konveyor <command>")));
    }

    void failsWhenKonveyorIsNotRunning()
    {
        m_konveyor.reset();
        for (const QString &request : {QStringLiteral("windows"), QStringLiteral("version"), QStringLiteral("load-config-file")}) {
            const ProgramResult result = konveyor({QStringLiteral("msg"), request});
            QCOMPARE(result.exitCode, 1);
            QVERIFY2(
                result.err.startsWith(QStringLiteral("Error: could not reach Konveyor (is the effect enabled?)")), qPrintable(result.err));
            QCOMPARE(result.out, QString());
        }
    }

private:
    ProgramResult konveyor(const QStringList &arguments) const { return m_session->run(QStringLiteral(KONVEYOR_CLI), arguments); }

    std::unique_ptr<PrivateSession> m_session;
    std::unique_ptr<FakeKonveyor> m_konveyor;
};

QTEST_GUILESS_MAIN(TestCliMsg)
#include "test_cli_msg.moc"
