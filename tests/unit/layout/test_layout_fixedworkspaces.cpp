#include "helpers.h"

using namespace LayoutTest;

namespace
{

Config::Config fixedConfig(int count = 4)
{
    Config::Config config = instantConfig();
    config.layout.fixedWorkspaceCount = count;
    return config;
}

int countOn(Fixture &fixture, const QString &output = QStringLiteral("DP-1"))
{
    int count = 0;
    for (const auto &workspace : fixture.engine().workspaceStates()) {
        count += workspace.output == output ? 1 : 0;
    }
    return count;
}

}

class TestLayoutFixedWorkspaces : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void emptySlotsKeepTheirIdentity()
    {
        Fixture fixture(fixedConfig());
        const auto before = fixture.engine().workspaceStates();
        QCOMPARE(before.size(), 4);
        fixture.perform(QStringLiteral("focus-workspace"), {QStringLiteral("4")});
        const auto id = fixture.add();
        QCOMPARE(fixture.state(id).workspaceIndex, 4);
        QCOMPARE(countOn(fixture), 4);
        fixture.remove(id);
        fixture.perform(QStringLiteral("focus-workspace"), {QStringLiteral("1")});
        const auto after = fixture.engine().workspaceStates();
        for (qsizetype i = 0; i < before.size(); ++i) {
            QCOMPARE(after[i].id, before[i].id);
        }
        VERIFY_INVARIANTS(fixture);
    }

    void windowsAndColumnsClampAtLastSlot()
    {
        Fixture fixture(fixedConfig(2));
        const auto id = fixture.add();
        fixture.perform(QStringLiteral("move-window-to-workspace"), {QStringLiteral("99")});
        QCOMPARE(fixture.state(id).workspaceIndex, 2);
        fixture.perform(QStringLiteral("move-window-to-workspace-down"));
        fixture.perform(QStringLiteral("move-column-to-workspace-down"));
        QCOMPARE(fixture.state(id).workspaceIndex, 2);
        QCOMPARE(countOn(fixture), 2);
        VERIFY_INVARIANTS(fixture);
    }

    void workspaceReorderingNeverAddsATail()
    {
        Fixture fixture(fixedConfig(2));
        const auto id = fixture.add();
        fixture.perform(QStringLiteral("move-workspace-down"));
        QCOMPARE(fixture.state(id).workspaceIndex, 2);
        fixture.perform(QStringLiteral("move-workspace-up"));
        QCOMPARE(fixture.state(id).workspaceIndex, 1);
        fixture.perform(QStringLiteral("move-workspace-to-index"), {QStringLiteral("2")});
        QCOMPARE(fixture.state(id).workspaceIndex, 2);
        QCOMPARE(countOn(fixture), 2);
        VERIFY_INVARIANTS(fixture);
    }

    void gapDropsUseAnExistingFixedSlot()
    {
        Fixture fixture(fixedConfig(2));
        const auto id = fixture.add();
        fixture.engine().setOverviewOpen(true);
        QVERIFY(startMove(fixture, id, QPointF(300, 1134)));
        fixture.engine().endWindowDrag();
        fixture.settle();
        QCOMPARE(fixture.state(id).workspaceIndex, 2);
        QCOMPARE(countOn(fixture), 2);
        VERIFY_INVARIANTS(fixture);
    }

    void fixedModeIgnoresEmptyAboveAndRestoresItLive()
    {
        Config::Config config = fixedConfig(1);
        config.layout.emptyWorkspaceAboveFirst = true;
        Fixture fixture(config);
        const auto id = fixture.add();
        QCOMPARE(fixture.state(id).workspaceIndex, 1);
        QCOMPARE(countOn(fixture), 1);
        config.layout.fixedWorkspaceCount = 0;
        fixture.setConfig(config);
        QCOMPARE(fixture.state(id).workspaceIndex, 2);
        QCOMPARE(countOn(fixture), 3);
        config.layout.fixedWorkspaceCount = 1;
        fixture.setConfig(config);
        QCOMPARE(fixture.state(id).workspaceIndex, 1);
        QCOMPARE(countOn(fixture), 1);
        VERIFY_INVARIANTS(fixture);
    }

    void reductionPreservesStackedColumnsFullscreenAndFloating()
    {
        Config::Config config = fixedConfig(4);
        Fixture fixture(config);
        const auto first = fixture.add();
        fixture.perform(QStringLiteral("focus-workspace"), {QStringLiteral("4")});
        const auto [top, bottom] = addStackedPair(fixture);
        fixture.perform(QStringLiteral("toggle-column-tabbed-display"));
        const auto game = fixture.add();
        fixture.perform(QStringLiteral("fullscreen-window"));
        const auto floating = fixture.add();
        fixture.perform(QStringLiteral("toggle-window-floating"));
        config.layout.fixedWorkspaceCount = 1;
        fixture.setConfig(config);
        QCOMPARE(countOn(fixture), 1);
        for (const auto id : {first, top, bottom, game, floating}) {
            QCOMPARE(fixture.state(id).workspaceIndex, 1);
        }
        QCOMPARE(fixture.state(top).columnIndex, fixture.state(bottom).columnIndex);
        QVERIFY(fixture.engine().workspaceStates().first().columnDisplays.contains(Config::ColumnDisplay::Tabbed));
        QCOMPARE(fixture.state(game).sizingMode, Layout::WindowMode::Fullscreen);
        QVERIFY(fixture.state(floating).isFloating);
        QCOMPARE(fixture.focused(), floating);
        VERIFY_INVARIANTS(fixture);
    }

    void countIncreasesLiveAndReloadKeepsSlots()
    {
        Config::Config config = fixedConfig(1);
        Fixture fixture(config);
        const auto id = fixture.add();
        config.layout.fixedWorkspaceCount = 5;
        fixture.setConfig(config);
        QCOMPARE(countOn(fixture), 5);
        const auto before = fixture.engine().workspaceStates();
        fixture.setConfig(config);
        const auto after = fixture.engine().workspaceStates();
        for (qsizetype i = 0; i < before.size(); ++i) {
            QCOMPARE(after[i].id, before[i].id);
        }
        QCOMPARE(fixture.state(id).workspaceIndex, 1);
        VERIFY_INVARIANTS(fixture);
    }

    void namedWorkspacesConsumeSlotsAndSetMinimum()
    {
        Config::Config config = fixedConfig(2);
        config.workspaces = {namedWorkspace(QStringLiteral("web")), namedWorkspace(QStringLiteral("chat"))};
        Fixture fixture(config);
        QCOMPARE(countOn(fixture), 2);
        QCOMPARE(fixture.workspaceNamed(QStringLiteral("web")).index, 1);
        QCOMPARE(fixture.workspaceNamed(QStringLiteral("chat")).index, 2);
        config.workspaces.append(namedWorkspace(QStringLiteral("extra")));
        fixture.setConfig(config);
        QCOMPARE(countOn(fixture), 3);
        QCOMPARE(fixture.workspaceNamed(QStringLiteral("extra")).index, 3);
        config.workspaces.removeLast();
        fixture.setConfig(config);
        QCOMPARE(countOn(fixture), 2);
        QVERIFY(fixture.workspaceNamed(QStringLiteral("web")).id != 0);
        QVERIFY(fixture.workspaceNamed(QStringLiteral("chat")).id != 0);
        VERIFY_INVARIANTS(fixture);
    }

    void monitorWorkspaceMovesReplaceEmptySourceSlot()
    {
        Fixture fixture(fixedConfig(2));
        fixture.addOutput(QStringLiteral("DP-2"), QRectF(1920, 0, 1920, 1080));
        const auto id = fixture.add();
        fixture.perform(QStringLiteral("move-workspace-to-monitor"), {QStringLiteral("DP-2")});
        QCOMPARE(fixture.state(id).output, QStringLiteral("DP-2"));
        QCOMPARE(countOn(fixture), 2);
        QCOMPARE(countOn(fixture, QStringLiteral("DP-2")), 2);
        VERIFY_INVARIANTS(fixture);
    }

    void disconnectMergesWindowsAndReconnectKeepsCap()
    {
        Fixture fixture(fixedConfig(1));
        fixture.addOutput(QStringLiteral("DP-2"), QRectF(1920, 0, 1920, 1080));
        const auto first = fixture.add();
        fixture.engine().focusOutput(QStringLiteral("DP-2"));
        const auto second = fixture.add();
        fixture.removeOutput(QStringLiteral("DP-2"));
        QCOMPARE(countOn(fixture), 1);
        QCOMPARE(fixture.state(first).workspaceIndex, 1);
        QCOMPARE(fixture.state(second).workspaceIndex, 1);
        fixture.addOutput(QStringLiteral("DP-2"), QRectF(1920, 0, 1920, 1080));
        QCOMPARE(countOn(fixture), 1);
        QCOMPARE(countOn(fixture, QStringLiteral("DP-2")), 1);
        QCOMPARE(fixture.state(second).output, QStringLiteral("DP-1"));
        VERIFY_INVARIANTS(fixture);
    }

    void namedHomesReturnAfterDisconnect()
    {
        Config::Config config = fixedConfig(1);
        config.layout.emptyWorkspaceAboveFirst = true;
        config.workspaces = {
            namedWorkspace(QStringLiteral("web"), QStringLiteral("DP-1")), namedWorkspace(QStringLiteral("chat"), QStringLiteral("DP-2"))};
        Fixture fixture(config);
        fixture.addOutput(QStringLiteral("DP-2"), QRectF(1920, 0, 1920, 1080));
        QCOMPARE(countOn(fixture), 1);
        QCOMPARE(countOn(fixture, QStringLiteral("DP-2")), 1);
        fixture.removeOutput(QStringLiteral("DP-2"));
        QCOMPARE(countOn(fixture), 2);
        fixture.perform(QStringLiteral("focus-workspace"), {QStringLiteral("chat")});
        fixture.addOutput(QStringLiteral("DP-2"), QRectF(1920, 0, 1920, 1080));
        QCOMPARE(countOn(fixture), 1);
        QCOMPARE(countOn(fixture, QStringLiteral("DP-2")), 1);
        QCOMPARE(fixture.workspaceNamed(QStringLiteral("chat")).output, QStringLiteral("DP-2"));
        VERIFY_INVARIANTS(fixture);
    }
};

QTEST_GUILESS_MAIN(TestLayoutFixedWorkspaces)
#include "test_layout_fixedworkspaces.moc"
