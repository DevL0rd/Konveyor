#include "configtesthelpers.h"

#include <QTest>

using namespace Konveyor::Config;
using namespace Konveyor::Config::Testing;

class TestConfigOutputs : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void parsesOutput();
    void parsesMonitorProfiles();
    void parsesWorkspaces();
    void rejectsDuplicateWorkspaceNames();
    void rejectsDisallowedWorkspaceLayoutNodes();
};

void TestConfigOutputs::parsesOutput()
{
    const LoadResult result = mustLoad(QStringLiteral(R"(
        layout {
            gaps 16
        }
        output "eDP-1" {
            layout {
                gaps 4
                border {
                    on
                }
            }
            hot-corners {
                off
            }
        }
        output "DP-2" {
        }
    )"));
    QCOMPARE(result.config.outputs.size(), 2);
    const OutputConfig &primary = result.config.outputs.first();
    QCOMPARE(primary.name, QStringLiteral("eDP-1"));
    QVERIFY(primary.layout.has_value());
    QCOMPARE(primary.layout->gaps, 4.0);
    QCOMPARE(primary.layout->border.enabled, true);
    QCOMPARE(primary.layout->focusRing.width, 4.0);
    QVERIFY(primary.hotCorners.has_value());
    QCOMPARE(primary.hotCorners->enabled, false);
    QCOMPARE(result.config.layout.gaps, 16.0);
    QVERIFY(!result.config.outputs.at(1).layout.has_value());
    QVERIFY(result.warnings.isEmpty());
    QVERIFY(mustFail(QStringLiteral("output \"eDP-1\" {\n scale 2\n}\n")).message.contains(QStringLiteral("unexpected node `scale`")));
}

void TestConfigOutputs::parsesMonitorProfiles()
{
    const Config config = parsed(QStringLiteral(R"(
        monitor-profile "ultrawide" {
            match aspect-ratio-above=2.0
            layout {
                default-column-width { proportion 0.25; }
            }
        }
        monitor-profile "standard" {
            layout {
                default-column-width { proportion 0.5; }
            }
        }
    )"));
    QCOMPARE(config.monitorProfiles.size(), 2);
    const MonitorProfile &wide = config.monitorProfiles.at(0);
    QCOMPARE(wide.name, QStringLiteral("ultrawide"));
    QCOMPARE(wide.matches.size(), 1);
    QCOMPARE(wide.matches.at(0).aspectRatioAbove, std::optional(2.0));
    QVERIFY(wide.layout.has_value());
    QCOMPARE(std::get<Proportion>(*wide.layout->defaultColumnWidth).value, 0.25);
    const MonitorProfile &standard = config.monitorProfiles.at(1);
    QVERIFY(standard.matches.isEmpty());
    QCOMPARE(std::get<Proportion>(*standard.layout->defaultColumnWidth).value, 0.5);
}

void TestConfigOutputs::parsesWorkspaces()
{
    const Config config = parsed(QStringLiteral(R"(
        workspace "chat" {
            open-on-output "eDP-1"
            layout {
                gaps 2
            }
        }
        workspace "code"
    )"));
    QCOMPARE(config.workspaces.size(), 2);
    QCOMPARE(config.workspaces.first().name, QStringLiteral("chat"));
    QCOMPARE(config.workspaces.first().openOnOutput, std::optional {QStringLiteral("eDP-1")});
    QVERIFY(config.workspaces.first().layout.has_value());
    QCOMPARE(config.workspaces.first().layout->gaps, 2.0);
    QCOMPARE(config.workspaces.at(1).name, QStringLiteral("code"));
    QVERIFY(!config.workspaces.at(1).layout.has_value());
}

void TestConfigOutputs::rejectsDuplicateWorkspaceNames()
{
    QCOMPARE(
        mustFail(QStringLiteral("workspace \"chat\"\nworkspace \"CHAT\"\n")).message, QStringLiteral("duplicate named workspace: CHAT"));
}

void TestConfigOutputs::rejectsDisallowedWorkspaceLayoutNodes()
{
    QVERIFY(mustFail(QStringLiteral("workspace \"a\" {\n layout {\n insert-hint {\n }\n }\n}\n"))
            .message.contains(QStringLiteral("not allowed inside `workspace.layout`")));
    QVERIFY(mustFail(QStringLiteral("workspace \"a\" {\n layout {\n empty-workspace-above-first\n }\n}\n"))
            .message.contains(QStringLiteral("not allowed inside `workspace.layout`")));
}

QTEST_MAIN(TestConfigOutputs)
#include "test_config_outputs.moc"
