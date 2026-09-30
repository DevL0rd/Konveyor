#include "helpers.h"

using namespace LayoutTest;

namespace
{

QList<Layout::WorkspaceState> workspaces(Fixture &fixture)
{
    return fixture.engine().workspaceStates();
}

int activeIndex(Fixture &fixture)
{
    for (const Layout::WorkspaceState &state : workspaces(fixture)) {
        if (state.isActive) {
            return state.index;
        }
    }
    return 0;
}

Config::Config stackingConfig(int maxRows)
{
    Config::Config config = instantConfig();
    config.layout.newWindowPlacement = Config::NewWindowPlacement::Stack;
    config.layout.maxRowsPerColumn = maxRows;
    return config;
}

}

class TestLayoutLiveSettings : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void newColumnsOpenLeftOfTheFirstColumn()
    {
        Config::Config config = instantConfig();
        config.layout.newColumnPosition = Config::NewColumnPosition::Left;
        Fixture fixture(config);
        const auto a = fixture.add(QStringLiteral("a"));
        const auto b = fixture.add(QStringLiteral("b"));
        const auto c = fixture.add(QStringLiteral("c"));
        QCOMPARE(fixture.state(c).columnIndex, 0);
        QCOMPARE(fixture.state(b).columnIndex, 1);
        QCOMPARE(fixture.state(a).columnIndex, 2);
        QCOMPARE(fixture.frame(c).x(), 16.0);
        VERIFY_INVARIANTS(fixture);
    }

    void newColumnPositionChangesLive()
    {
        Config::Config config = instantConfig();
        Fixture fixture(config);
        const auto a = fixture.add(QStringLiteral("a"));
        const auto b = fixture.add(QStringLiteral("b"));
        config.layout.newColumnPosition = Config::NewColumnPosition::Left;
        fixture.setConfig(config);
        QCOMPARE(fixture.state(b).columnIndex, 1);
        const auto c = fixture.add(QStringLiteral("c"));
        QCOMPARE(fixture.state(a).columnIndex, 0);
        QCOMPARE(fixture.state(c).columnIndex, 1);
        QCOMPARE(fixture.state(b).columnIndex, 2);
    }

    void maxRowsOfOneGivesEveryWindowAColumn()
    {
        Fixture fixture(stackingConfig(1));
        const auto a = fixture.add(QStringLiteral("a"));
        const auto b = fixture.add(QStringLiteral("b"));
        QVERIFY(fixture.state(a).columnIndex != fixture.state(b).columnIndex);
    }

    void maxRowsChangeAppliesToNewWindowsOnly()
    {
        Config::Config config = stackingConfig(3);
        Fixture fixture(config);
        const auto a = fixture.add(QStringLiteral("a"));
        const auto b = fixture.add(QStringLiteral("b"));
        const auto c = fixture.add(QStringLiteral("c"));
        QCOMPARE(fixture.state(c).columnIndex, fixture.state(a).columnIndex);
        config.layout.maxRowsPerColumn = 2;
        fixture.setConfig(config);
        QCOMPARE(fixture.state(c).columnIndex, fixture.state(a).columnIndex);
        QCOMPARE(fixture.state(b).columnIndex, fixture.state(a).columnIndex);
        const auto d = fixture.add(QStringLiteral("d"));
        QVERIFY(fixture.state(d).columnIndex != fixture.state(a).columnIndex);
        VERIFY_INVARIANTS(fixture);
    }

    void stackPlacementSkipsAnExpandedColumn()
    {
        Fixture fixture(stackingConfig(3));
        const auto a = fixture.add(QStringLiteral("a"));
        fixture.perform(QStringLiteral("maximize-column"));
        const auto b = fixture.add(QStringLiteral("b"));
        QVERIFY(fixture.state(b).columnIndex != fixture.state(a).columnIndex);
        fixture.perform(QStringLiteral("fullscreen-window"));
        const auto c = fixture.add(QStringLiteral("c"));
        QVERIFY(fixture.state(c).columnIndex != fixture.state(b).columnIndex);
        VERIFY_INVARIANTS(fixture);
    }

    void placementSwitchedLiveStacksAndUnstacks()
    {
        Config::Config config = instantConfig();
        Fixture fixture(config);
        const auto a = fixture.add(QStringLiteral("a"));
        const auto b = fixture.add(QStringLiteral("b"));
        fixture.setConfig(stackingConfig(3));
        QCOMPARE(fixture.state(b).columnIndex, fixture.state(a).columnIndex);
        QCOMPARE(fixture.focused(), std::optional(b));
        fixture.setConfig(config);
        QVERIFY(fixture.state(b).columnIndex != fixture.state(a).columnIndex);
        QCOMPARE(fixture.frame(a).width(), 936.0);
        QCOMPARE(fixture.frame(b).width(), 936.0);
        VERIFY_INVARIANTS(fixture);
    }

    void groupingSwitchedLiveAppliesToTheNextWindow()
    {
        Config::Config config = instantConfig();
        config.layout.groupAppWindows = Config::GroupAppWindows::Off;
        Fixture fixture(config);
        const auto first = fixture.add(QStringLiteral("app"));
        fixture.add(QStringLiteral("other"));
        const auto apart = fixture.add(QStringLiteral("app"));
        QCOMPARE(fixture.state(apart).columnIndex, 2);
        config.layout.groupAppWindows = Config::GroupAppWindows::Stack;
        fixture.setConfig(config);
        const auto grouped = fixture.add(QStringLiteral("app"));
        QVERIFY(fixture.state(grouped).columnIndex == fixture.state(first).columnIndex
            || fixture.state(grouped).columnIndex == fixture.state(apart).columnIndex);
        VERIFY_INVARIANTS(fixture);
    }

    void defaultColumnDisplaySwitchedLiveLeavesOpenColumnsAlone()
    {
        Config::Config config = instantConfig();
        Fixture fixture(config);
        const auto [top, bottom] = addStackedPair(fixture);
        config.layout.defaultColumnDisplay = Config::ColumnDisplay::Tabbed;
        fixture.setConfig(config);
        QVERIFY(fixture.state(top).visible);
        QVERIFY(fixture.state(bottom).visible);
        const auto first = fixture.add(QStringLiteral("x"));
        fixture.add(QStringLiteral("y"));
        fixture.perform(QStringLiteral("consume-or-expel-window-left"));
        QVERIFY(!fixture.state(first).visible);
    }

    void floatChildWindowsSwitchedLive()
    {
        Config::Config config = instantConfig();
        Fixture fixture(config);
        fixture.add(QStringLiteral("app"));
        QVERIFY(!fixture.state(fixture.add(QStringLiteral("app"))).isFloating);
        config.layout.floatChildWindows = true;
        fixture.setConfig(config);
        QVERIFY(fixture.state(fixture.add(QStringLiteral("app"))).isFloating);
        QVERIFY(!fixture.state(fixture.add(QStringLiteral("fresh"))).isFloating);
        VERIFY_INVARIANTS(fixture);
    }

    void emptyWorkspaceAboveFirstSwitchedLive()
    {
        Config::Config config = instantConfig();
        Fixture fixture(config);
        const auto id = fixture.add(QStringLiteral("a"));
        QCOMPARE(workspaces(fixture).size(), 2);
        config.layout.emptyWorkspaceAboveFirst = true;
        fixture.setConfig(config);
        QCOMPARE(workspaces(fixture).size(), 3);
        QCOMPARE(fixture.state(id).workspaceIndex, 2);
        QCOMPARE(activeIndex(fixture), 2);
        QVERIFY(fixture.state(id).onActiveWorkspace);
        config.layout.emptyWorkspaceAboveFirst = false;
        fixture.setConfig(config);
        QCOMPARE(workspaces(fixture).size(), 2);
        QCOMPARE(fixture.state(id).workspaceIndex, 1);
        QCOMPARE(activeIndex(fixture), 1);
        VERIFY_INVARIANTS(fixture);
    }

    void emptyWorkspaceAboveFirstStaysWhileItIsFocused()
    {
        Config::Config config = instantConfig();
        config.layout.emptyWorkspaceAboveFirst = true;
        Fixture fixture(config);
        fixture.add(QStringLiteral("a"));
        fixture.perform(QStringLiteral("focus-workspace-up"));
        fixture.advance(1);
        QCOMPARE(activeIndex(fixture), 1);
        config.layout.emptyWorkspaceAboveFirst = false;
        fixture.setConfig(config);
        QCOMPARE(activeIndex(fixture), 1);
        fixture.perform(QStringLiteral("focus-workspace-down"));
        fixture.advance(1);
        QCOMPARE(workspaces(fixture).size(), 2);
        QCOMPARE(activeIndex(fixture), 1);
        VERIFY_INVARIANTS(fixture);
    }

    void outputColoursReachTheOutputState()
    {
        Config::Config config = instantConfig();
        config.layout.backgroundColor = QColor(10, 20, 30);
        config.layout.insertHint.paint.color = QColor(1, 2, 3, 4);
        Fixture fixture(config);
        const Layout::OutputState state = fixture.engine().outputStates().first();
        QCOMPARE(state.backgroundColor, QColor(10, 20, 30));
        QCOMPARE(state.dropHintPaint.color, QColor(1, 2, 3, 4));
    }
};

QTEST_GUILESS_MAIN(TestLayoutLiveSettings)
#include "test_layout_livesettings.moc"
