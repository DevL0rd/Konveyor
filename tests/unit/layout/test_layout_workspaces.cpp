#include "helpers.h"

using namespace LayoutTest;

namespace
{

int workspaceCount(Layout::Engine &engine, const QString &output)
{
    int count = 0;
    for (const Layout::WorkspaceState &state : engine.workspaceStates()) {
        if (state.output == output) {
            count += 1;
        }
    }
    return count;
}

Layout::WorkspaceState workspaceAt(Layout::Engine &engine, int index)
{
    for (const Layout::WorkspaceState &state : engine.workspaceStates()) {
        if (state.index == index) {
            return state;
        }
    }
    return {};
}

}

class TestLayoutWorkspaces : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void startsWithOneEmptyWorkspace()
    {
        Fixture fixture;
        QCOMPARE(workspaceCount(fixture.engine(), QStringLiteral("DP-1")), 1);
        QCOMPARE(fixture.engine().workspaceStates().first().index, 1);
        VERIFY_INVARIANTS(fixture);
    }

    void addingWindowCreatesTrailingEmptyWorkspace()
    {
        Fixture fixture;
        fixture.add();
        QCOMPARE(workspaceCount(fixture.engine(), QStringLiteral("DP-1")), 2);
        VERIFY_INVARIANTS(fixture);
    }

    void focusWorkspaceDownAndUp()
    {
        Fixture fixture;
        fixture.add();
        QVERIFY(fixture.perform(QStringLiteral("focus-workspace-down")).ok);
        QCOMPARE(workspaceAt(fixture.engine(), 2).isActive, true);
        QVERIFY(fixture.perform(QStringLiteral("focus-workspace-up")).ok);
        QCOMPARE(workspaceAt(fixture.engine(), 1).isActive, true);
        VERIFY_INVARIANTS(fixture);
    }

    void focusWorkspacePrevious()
    {
        Fixture fixture;
        fixture.add();
        fixture.perform(QStringLiteral("focus-workspace-down"));
        QVERIFY(fixture.perform(QStringLiteral("focus-workspace-previous")).ok);
        QCOMPARE(workspaceAt(fixture.engine(), 1).isActive, true);
        VERIFY_INVARIANTS(fixture);
    }

    void moveWindowToWorkspaceDownFollowsFocus()
    {
        Fixture fixture;
        const auto id = fixture.add();
        QVERIFY(fixture.perform(QStringLiteral("move-window-to-workspace-down")).ok);
        QCOMPARE(fixture.state(id).workspaceIndex, 2);
        QCOMPARE(fixture.focused(), id);
        VERIFY_INVARIANTS(fixture);
    }

    void moveWindowToWorkspaceDownWithoutFocus()
    {
        Fixture fixture;
        const auto first = fixture.add();
        const auto second = fixture.add();
        QVERIFY(
            fixture.perform(QStringLiteral("move-window-to-workspace-down"), {}, {{QStringLiteral("focus"), QStringLiteral("false")}}).ok);
        QCOMPARE(fixture.state(second).workspaceIndex, 2);
        QCOMPARE(fixture.focused(), first);
        VERIFY_INVARIANTS(fixture);
    }

    void moveColumnToWorkspaceByIndex()
    {
        Fixture fixture;
        const auto id = fixture.add();
        QVERIFY(fixture.perform(QStringLiteral("move-column-to-workspace"), {QStringLiteral("2")}).ok);
        QCOMPARE(fixture.state(id).workspaceIndex, 2);
        VERIFY_INVARIANTS(fixture);
    }

    void emptyWorkspacesAreCleanedUp()
    {
        Fixture fixture;
        const auto id = fixture.add();
        fixture.perform(QStringLiteral("move-window-to-workspace-down"));
        fixture.advance(1);
        QCOMPARE(workspaceCount(fixture.engine(), QStringLiteral("DP-1")), 2);
        QCOMPARE(fixture.state(id).workspaceIndex, 1);
        VERIFY_INVARIANTS(fixture);
    }

    void namedWorkspaceFromConfigExists()
    {
        Config::Config config = instantConfig();
        Config::NamedWorkspace named;
        named.name = QStringLiteral("browser");
        config.workspaces.append(named);
        Fixture fixture(config);
        QCOMPARE(workspaceCount(fixture.engine(), QStringLiteral("DP-1")), 2);
        QCOMPARE(workspaceAt(fixture.engine(), 1).name, QStringLiteral("browser"));
        VERIFY_INVARIANTS(fixture);
    }

    void namedWorkspaceSurvivesCleanup()
    {
        Config::Config config = instantConfig();
        Config::NamedWorkspace named;
        named.name = QStringLiteral("browser");
        config.workspaces.append(named);
        Fixture fixture(config);
        fixture.perform(QStringLiteral("focus-workspace-down"));
        QCOMPARE(workspaceCount(fixture.engine(), QStringLiteral("DP-1")), 2);
        VERIFY_INVARIANTS(fixture);
    }

    void focusWorkspaceByName()
    {
        Config::Config config = instantConfig();
        Config::NamedWorkspace named;
        named.name = QStringLiteral("chat");
        config.workspaces.append(named);
        Fixture fixture(config);
        fixture.perform(QStringLiteral("focus-workspace-down"));
        QVERIFY(fixture.perform(QStringLiteral("focus-workspace"), {QStringLiteral("chat")}).ok);
        QCOMPARE(workspaceAt(fixture.engine(), 1).isActive, true);
        VERIFY_INVARIANTS(fixture);
    }

    void setAndUnsetWorkspaceName()
    {
        Fixture fixture;
        fixture.add();
        QVERIFY(fixture.perform(QStringLiteral("set-workspace-name"), {QStringLiteral("work")}).ok);
        QCOMPARE(workspaceAt(fixture.engine(), 1).name, QStringLiteral("work"));
        QVERIFY(fixture.perform(QStringLiteral("unset-workspace-name")).ok);
        QCOMPARE(workspaceAt(fixture.engine(), 1).name, QString());
        VERIFY_INVARIANTS(fixture);
    }

    void emptyWorkspaceAboveFirst()
    {
        Config::Config config = instantConfig();
        config.layout.emptyWorkspaceAboveFirst = true;
        Fixture fixture(config);
        const auto id = fixture.add();
        QCOMPARE(workspaceCount(fixture.engine(), QStringLiteral("DP-1")), 3);
        QCOMPARE(fixture.state(id).workspaceIndex, 2);
        VERIFY_INVARIANTS(fixture);
    }

    void moveWorkspaceDownReordersWorkspaces()
    {
        Fixture fixture;
        const auto first = fixture.add();
        fixture.perform(QStringLiteral("focus-workspace-down"));
        const auto second = fixture.add();
        QCOMPARE(fixture.state(second).workspaceIndex, 2);
        QVERIFY(fixture.perform(QStringLiteral("move-workspace-up")).ok);
        QCOMPARE(fixture.state(second).workspaceIndex, 1);
        QCOMPARE(fixture.state(first).workspaceIndex, 2);
        VERIFY_INVARIANTS(fixture);
    }

    void moveWorkspaceToIndex()
    {
        Fixture fixture;
        const auto first = fixture.add();
        fixture.perform(QStringLiteral("focus-workspace-down"));
        const auto second = fixture.add();
        QVERIFY(fixture.perform(QStringLiteral("move-workspace-to-index"), {QStringLiteral("1")}).ok);
        QCOMPARE(fixture.state(second).workspaceIndex, 1);
        QCOMPARE(fixture.state(first).workspaceIndex, 2);
        VERIFY_INVARIANTS(fixture);
    }

    void secondOutputGetsItsOwnWorkspaces()
    {
        Fixture fixture;
        fixture.add();
        fixture.engine().addOutput(makeOutput(QStringLiteral("DP-2"), QRectF(1920, 0, 1280, 720)));
        QCOMPARE(workspaceCount(fixture.engine(), QStringLiteral("DP-2")), 1);
        QCOMPARE(workspaceCount(fixture.engine(), QStringLiteral("DP-1")), 2);
        VERIFY_INVARIANTS(fixture);
    }

    void removingOutputMigratesWindows()
    {
        Fixture fixture;
        fixture.engine().addOutput(makeOutput(QStringLiteral("DP-2"), QRectF(1920, 0, 1280, 720)));
        fixture.engine().focusOutput(QStringLiteral("DP-2"));
        const auto id = fixture.add();
        QCOMPARE(fixture.state(id).output, QStringLiteral("DP-2"));
        fixture.engine().removeOutput(QStringLiteral("DP-2"));
        fixture.settle();
        QCOMPARE(fixture.state(id).output, QStringLiteral("DP-1"));
        VERIFY_INVARIANTS(fixture);
    }

    void reAddingOutputRestoresWorkspaces()
    {
        Fixture fixture;
        fixture.engine().addOutput(makeOutput(QStringLiteral("DP-2"), QRectF(1920, 0, 1280, 720)));
        fixture.engine().focusOutput(QStringLiteral("DP-2"));
        const auto id = fixture.add();
        fixture.engine().removeOutput(QStringLiteral("DP-2"));
        fixture.settle();
        fixture.engine().addOutput(makeOutput(QStringLiteral("DP-2"), QRectF(1920, 0, 1280, 720)));
        fixture.settle();
        QCOMPARE(fixture.state(id).output, QStringLiteral("DP-2"));
        VERIFY_INVARIANTS(fixture);
    }

    void focusMonitorSwitchesOutput()
    {
        Fixture fixture;
        fixture.engine().addOutput(makeOutput(QStringLiteral("DP-2"), QRectF(1920, 0, 1280, 720)));
        QVERIFY(fixture.perform(QStringLiteral("focus-monitor-right")).ok);
        QCOMPARE(fixture.engine().focusedOutput(), QStringLiteral("DP-2"));
        QVERIFY(fixture.perform(QStringLiteral("focus-monitor-left")).ok);
        QCOMPARE(fixture.engine().focusedOutput(), QStringLiteral("DP-1"));
        VERIFY_INVARIANTS(fixture);
    }

    void moveWindowToMonitorByName()
    {
        Fixture fixture;
        fixture.engine().addOutput(makeOutput(QStringLiteral("DP-2"), QRectF(1920, 0, 1280, 720)));
        const auto id = fixture.add();
        QVERIFY(fixture.perform(QStringLiteral("move-window-to-monitor"), {QStringLiteral("DP-2")}).ok);
        fixture.settle();
        QCOMPARE(fixture.state(id).output, QStringLiteral("DP-2"));
        VERIFY_INVARIANTS(fixture);
    }

    void unknownMonitorNameFails()
    {
        Fixture fixture;
        const auto result = fixture.perform(QStringLiteral("focus-monitor"), {QStringLiteral("HDMI-9")});
        QVERIFY(!result.ok);
        QVERIFY(result.error.contains(QStringLiteral("HDMI-9")));
    }

    void workspaceAutoBackAndForth()
    {
        Config::Config config = instantConfig();
        config.input.workspaceAutoBackAndForth = true;
        Fixture fixture(config);
        fixture.add();
        fixture.perform(QStringLiteral("focus-workspace-down"));
        fixture.add();
        QVERIFY(fixture.perform(QStringLiteral("focus-workspace"), {QStringLiteral("2")}).ok);
        QCOMPARE(workspaceAt(fixture.engine(), 1).isActive, true);
        VERIFY_INVARIANTS(fixture);
    }
};

QTEST_GUILESS_MAIN(TestLayoutWorkspaces)
#include "test_layout_workspaces.moc"
