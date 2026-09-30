#include "configtesthelpers.h"

#include <QTest>

using namespace Konveyor::Config;
using namespace Konveyor::Config::Testing;

namespace
{

HotCorners globalCorners(const QString &body)
{
    return parsed(QStringLiteral("gestures { hot-corners { %1 }; }").arg(body)).gestures.hotCorners;
}

HotCorners outputCorners(const QString &body)
{
    return *parsed(QStringLiteral("output \"DP-1\" { hot-corners { %1 }; }").arg(body)).outputs.first().hotCorners;
}

HotCorners cornersOf(bool enabled, bool topLeft, bool topRight, bool bottomLeft, bool bottomRight)
{
    return HotCorners {enabled, topLeft, topRight, bottomLeft, bottomRight};
}

}

class TestConfigMonitors : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void hotCorners_data();
    void hotCorners();
    void outputHotCornersStandAlone();
    void hotCornersRejectUnknownNodes();
    void matchesByNamePattern();
    void matchesBySize();
    void keepsEveryMatchNode();
    void rejectsBadMatches_data();
    void rejectsBadMatches();
    void rejectsDuplicateOutputNames();
    void rejectsDuplicateProfileNames();
    void rejectsDuplicateWorkspaceNamesAcrossCase();
};

void TestConfigMonitors::hotCorners_data()
{
    QTest::addColumn<QString>("body");
    QTest::addColumn<HotCorners>("expected");

    QTest::newRow("empty implies top left") << QString() << cornersOf(true, true, false, false, false);
    QTest::newRow("off keeps implied corner") << QStringLiteral("off;") << cornersOf(false, true, false, false, false);
    QTest::newRow("off false") << QStringLiteral("off false;") << cornersOf(true, true, false, false, false);
    QTest::newRow("top left") << QStringLiteral("top-left;") << cornersOf(true, true, false, false, false);
    QTest::newRow("top left false") << QStringLiteral("top-left false;") << cornersOf(true, false, false, false, false);
    QTest::newRow("top right replaces top left") << QStringLiteral("top-right;") << cornersOf(true, false, true, false, false);
    QTest::newRow("bottom corners") << QStringLiteral("bottom-left; bottom-right true;") << cornersOf(true, false, false, true, true);
    QTest::newRow("all corners") << QStringLiteral("top-left; top-right; bottom-left; bottom-right;")
                                 << cornersOf(true, true, true, true, true);
    QTest::newRow("corner false and true") << QStringLiteral("top-right false; bottom-left;") << cornersOf(true, false, false, true, false);
    QTest::newRow("off with corners") << QStringLiteral("off; bottom-right;") << cornersOf(false, false, false, false, true);
}

void TestConfigMonitors::hotCorners()
{
    QFETCH(QString, body);
    QFETCH(HotCorners, expected);
    QCOMPARE(globalCorners(body), expected);
    QCOMPARE(outputCorners(body), expected);
}

void TestConfigMonitors::outputHotCornersStandAlone()
{
    const Config config = parsed(QStringLiteral(R"(
        gestures { hot-corners { bottom-right; }; }
        output "DP-1" { hot-corners { top-right; }; }
        output "DP-2" { layout { gaps 1; }; }
    )"));
    QCOMPARE(config.gestures.hotCorners, cornersOf(true, false, false, false, true));
    QCOMPARE(*config.outputs.at(0).hotCorners, cornersOf(true, false, true, false, false));
    QCOMPARE(config.outputs.at(1).hotCorners, std::nullopt);
    QCOMPARE(parsed(QString()).gestures.hotCorners, cornersOf(true, true, false, false, false));
}

void TestConfigMonitors::hotCornersRejectUnknownNodes()
{
    verifyFailure(QStringLiteral("gestures { hot-corners { »left; }; }"), QStringLiteral("unexpected node `left`"));
    verifyFailure(QStringLiteral("gestures { hot-corners { top-left; »top-left; }; }"),
        QStringLiteral("duplicate node `top-left`, single node expected"));
    verifyFailure(QStringLiteral("output \"DP-1\" { hot-corners »1 {}; }"), QStringLiteral("no arguments expected for this node"));
    verifyFailure(QStringLiteral("output \"DP-1\" { hot-corners {}; »hot-corners {}; }"),
        QStringLiteral("duplicate node `hot-corners`, single node expected"));
}

void TestConfigMonitors::matchesByNamePattern()
{
    const Config config = parsed(QStringLiteral(R"(monitor-profile "docked" { match name="^DP-[0-9]$"; })"));
    const MonitorMatch &match = config.monitorProfiles.first().matches.first();
    QVERIFY(match.name.has_value());
    QVERIFY(match.name->match(QStringLiteral("DP-3")).hasMatch());
    QVERIFY(!match.name->match(QStringLiteral("eDP-1")).hasMatch());
    QCOMPARE(match.aspectRatioAbove, std::nullopt);
    QCOMPARE(match.widthBelow, std::nullopt);
}

void TestConfigMonitors::matchesBySize()
{
    const Config config = parsed(QStringLiteral(R"(
        monitor-profile "big" {
            match width-above=2560 width-below=5121 height-above=1080.5 height-below=2161 aspect-ratio-above=1.5 aspect-ratio-below=2.5
        }
    )"));
    const MonitorMatch &match = config.monitorProfiles.first().matches.first();
    QCOMPARE(match.widthAbove, std::optional(2560.0));
    QCOMPARE(match.widthBelow, std::optional(5121.0));
    QCOMPARE(match.heightAbove, std::optional(1080.5));
    QCOMPARE(match.heightBelow, std::optional(2161.0));
    QCOMPARE(match.aspectRatioAbove, std::optional(1.5));
    QCOMPARE(match.aspectRatioBelow, std::optional(2.5));
    QCOMPARE(match.name, std::nullopt);
}

void TestConfigMonitors::keepsEveryMatchNode()
{
    const Config config = parsed(QStringLiteral(R"(
        monitor-profile "either" {
            match name="^HDMI"
            layout { gaps 3; }
            match width-below=1500
            match
        }
    )"));
    const MonitorProfile &profile = config.monitorProfiles.first();
    QCOMPARE(profile.matches.size(), 3);
    QVERIFY(profile.matches.at(0).name.has_value());
    QCOMPARE(profile.matches.at(1).widthBelow, std::optional(1500.0));
    QCOMPARE(profile.matches.at(2), MonitorMatch {});
    QCOMPARE(profile.layout->gaps, 3.0);
}

void TestConfigMonitors::rejectsBadMatches_data()
{
    QTest::addColumn<QString>("marked");
    QTest::addColumn<QString>("message");

    QTest::newRow("invalid regex") << QStringLiteral("monitor-profile \"p\" { match name=»\"(DP\"; }")
                                   << QStringLiteral("invalid regex: missing closing parenthesis");
    QTest::newRow("name number") << QStringLiteral("monitor-profile \"p\" { match name=»1; }") << QStringLiteral("expected a string");
    QTest::newRow("argument") << QStringLiteral("monitor-profile \"p\" { match »\"DP-1\"; }")
                              << QStringLiteral("no arguments expected for this node");
    QTest::newRow("children") << QStringLiteral("monitor-profile \"p\" { match { »name; }; }") << QStringLiteral("unexpected node `name`");
    QTest::newRow("unknown property") << QStringLiteral("monitor-profile \"p\" { match »scale=2; }")
                                      << QStringLiteral("unexpected property `scale`");
    QTest::newRow("string size") << QStringLiteral("monitor-profile \"p\" { match width-above=»\"2560\"; }")
                                 << QStringLiteral("unsupported value, only numbers are recognized");
    QTest::newRow("profile name number") << QStringLiteral("monitor-profile »1 {}") << QStringLiteral("expected a string");
    QTest::newRow("profile two names") << QStringLiteral("monitor-profile \"a\" »\"b\" {}") << QStringLiteral("unexpected argument");
    QTest::newRow("profile two layouts") << QStringLiteral("monitor-profile \"p\" { layout {}; »layout {}; }")
                                         << QStringLiteral("duplicate node `layout`, single node expected");
}

void TestConfigMonitors::rejectsBadMatches()
{
    QFETCH(QString, marked);
    QFETCH(QString, message);
    verifyFailure(marked, message);
}

void TestConfigMonitors::rejectsDuplicateOutputNames()
{
    verifyFailure(QStringLiteral("output \"DP-1\"\noutput »\"DP-1\"\n"), QStringLiteral("duplicate output: DP-1"));
    verifyFailure(QStringLiteral("output \"dp-1\" { layout { gaps 1; }; }\noutput »\"DP-1\" { hot-corners { off; }; }\n"),
        QStringLiteral("duplicate output: DP-1"));
    QCOMPARE(parsed(QStringLiteral("output \"DP-1\"\noutput \"DP-2\"\n")).outputs.size(), 2);
}

void TestConfigMonitors::rejectsDuplicateProfileNames()
{
    verifyFailure(
        QStringLiteral("monitor-profile \"wide\" {}\nmonitor-profile »\"wide\" {}\n"), QStringLiteral("duplicate monitor profile: wide"));
    const Config config = parsed(QStringLiteral("monitor-profile \"wide\" {}\nmonitor-profile \"Wide\" {}\n"));
    QCOMPARE(config.monitorProfiles.size(), 2);
    QCOMPARE(config.monitorProfiles.at(1).name, QStringLiteral("Wide"));
}

void TestConfigMonitors::rejectsDuplicateWorkspaceNamesAcrossCase()
{
    verifyFailure(QStringLiteral("workspace \"Chat\" {}\nworkspace »\"chat\"\n"), QStringLiteral("duplicate named workspace: chat"));
    QCOMPARE(parsed(QStringLiteral("workspace \"a\"\nworkspace \"b\"\n")).workspaces.size(), 2);
}

QTEST_MAIN(TestConfigMonitors)
#include "test_config_monitors.moc"
