#include "striphelpers.h"

using namespace LayoutTest;

namespace
{

Config::Config withEmptyAbove()
{
    Config::Config config = instantConfig();
    config.layout.emptyWorkspaceAboveFirst = true;
    return config;
}

Config::Config withFixed(int count)
{
    Config::Config config = instantConfig();
    config.layout.fixedWorkspaceCount = count;
    return config;
}

}

class TestLayoutMonitorEdges : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void edgeScrollingSwitchesToTheWorkspaceBeyondTheGesture()
    {
        MonitorKit kit;
        kit.add(0);
        kit.monitor.switchWorkspace(1);
        kit.add(1);
        kit.monitor.switchWorkspace(1);
        kit.monitor.overviewOpen = true;
        kit.monitor.beginEdgeScroll();
        kit.monitor.switchWorkspaceUp();
        QCOMPARE(kit.monitor.activeWorkspaceIndex(), std::size_t(0));
        kit.monitor.switchWorkspaceDown();
        QCOMPARE(kit.monitor.activeWorkspaceIndex(), std::size_t(1));
        kit.monitor.endEdgeScroll();
    }

    void edgeScrollingReactsToThePointerAtTheEdges()
    {
        MonitorKit kit;
        kit.add(0);
        kit.monitor.overviewOpen = true;
        QVERIFY(!kit.monitor.edgeScrollBy(QPointF(100, 0), 1000.0));
        kit.monitor.beginEdgeScroll();
        const double height = kit.area.workingArea.height();
        for (const QPointF pos : {QPointF(100, 0), QPointF(100, height), QPointF(100, height / 2), QPointF(-5, 0), QPointF(5000, 0)}) {
            kit.clock.setRawNow(kit.clock.rawNow() + std::chrono::seconds(1));
            kit.monitor.edgeScrollBy(pos, 1000.0);
            kit.clock.setRawNow(kit.clock.rawNow() + std::chrono::seconds(1));
            kit.monitor.edgeScrollBy(pos, 1000.0);
        }
        QVERIFY(!kit.monitor.updateWorkspaceSwipe(10.0, kit.clock.rawNow(), true).has_value());
        kit.monitor.endEdgeScroll();
        kit.monitor.endEdgeScroll();
    }

    void workspaceSwipesIgnoreUpdatesDuringEdgeScrolling()
    {
        MonitorKit kit;
        kit.add(0);
        kit.monitor.overviewOpen = true;
        kit.monitor.beginEdgeScroll();
        QVERIFY(!kit.monitor.updateWorkspaceSwipe(10.0, kit.clock.rawNow(), false).has_value());
        kit.monitor.endEdgeScroll();
        kit.monitor.beginWorkspaceSwipe(false);
        QVERIFY(kit.monitor.updateWorkspaceSwipe(40.0, kit.clock.rawNow(), false).has_value());
        QVERIFY(kit.monitor.endWorkspaceSwipe(std::nullopt));
    }

    void insertingIntoColumnsMayKeepTheWorkspace()
    {
        MonitorKit kit;
        kit.add(0);
        kit.monitor.switchWorkspace(1);
        kit.monitor.insertIntoColumn(0, 0, std::nullopt, kit.makeTile(), true, false);
        QCOMPARE(kit.monitor.activeWorkspaceIndex(), std::size_t(1));
        kit.monitor.insertIntoColumn(0, 0, std::nullopt, kit.makeTile(), false, true);
        QCOMPARE(kit.monitor.activeWorkspaceIndex(), std::size_t(1));
        kit.monitor.insertIntoColumn(0, 0, std::nullopt, kit.makeTile(), true, true);
        QCOMPARE(kit.monitor.activeWorkspaceIndex(), std::size_t(0));
    }

    void movingAWindowOfAnUnknownIdDoesNothing()
    {
        MonitorKit kit;
        kit.add(0);
        kit.monitor.moveToWorkspace(999, 1, Layout::Activation::Always);
        QCOMPARE(kit.monitor.workspaces().size(), std::size_t(2));
        QCOMPARE(kit.monitor.checkConsistency(), QString());
    }

    void movingTheOnlyWindowAwayDropsItsWorkspace()
    {
        MonitorKit kit;
        const Layout::WindowId id = kit.add(0);
        kit.monitor.switchWorkspace(1);
        kit.monitor.switchWorkspace(0);
        kit.monitor.moveToWorkspace(id, 1, Layout::Activation::Never);
        kit.monitor.tickAnimations();
        QVERIFY(kit.monitor.hasWindow(id));
        QCOMPARE(kit.monitor.checkConsistency(), QString());
    }

    void movingColumnsNeedsAnActiveColumn()
    {
        MonitorKit kit;
        kit.add(0);
        kit.monitor.switchWorkspace(1);
        kit.monitor.moveColumnToWorkspace(0, true);
        QCOMPARE(kit.monitor.checkConsistency(), QString());
        QVERIFY(kit.monitor.workspaceOfWindow(1).has_value());
    }

    void movingWorkspacesKeepsAnEmptyOneAbove()
    {
        MonitorKit kit(withEmptyAbove());
        kit.add(0);
        kit.monitor.switchWorkspace(0);
        kit.monitor.moveWorkspaceDown();
        QCOMPARE(kit.monitor.checkConsistency(), QString());
        kit.monitor.moveWorkspaceUp();
        QCOMPARE(kit.monitor.checkConsistency(), QString());
    }

    void movingWorkspacesByIndexKeepsTheActiveOne()
    {
        MonitorKit kit;
        for (std::size_t i = 0; i < 4; ++i) {
            kit.add(i);
        }
        kit.monitor.switchWorkspace(2);
        const Layout::WorkspaceId active = kit.monitor.activeWorkspace().id();
        kit.monitor.moveWorkspaceToIndex(99, 0);
        kit.monitor.moveWorkspaceToIndex(1, 1);
        kit.monitor.moveWorkspaceToIndex(0, 3);
        QCOMPARE(kit.monitor.activeWorkspace().id(), active);
        kit.monitor.moveWorkspaceToIndex(3, 0);
        QCOMPARE(kit.monitor.activeWorkspace().id(), active);
        kit.monitor.moveWorkspaceToIndex(2, 0);
        QCOMPARE(kit.monitor.activeWorkspace().id(), active);
        QCOMPARE(kit.monitor.checkConsistency(), QString());
    }

    void movingWorkspacesWithAnEmptyOneAboveKeepsItFirst()
    {
        MonitorKit kit(withEmptyAbove());
        kit.add(0);
        kit.monitor.switchWorkspace(1);
        kit.add(1);
        kit.monitor.moveWorkspaceToIndex(1, 2);
        QCOMPARE(kit.monitor.checkConsistency(), QString());
        kit.monitor.moveWorkspaceToIndex(2, 1);
        QCOMPARE(kit.monitor.checkConsistency(), QString());
        kit.monitor.moveWorkspaceToIndex(0, 2);
        QCOMPARE(kit.monitor.checkConsistency(), QString());
    }

    void dropTargetsCoverTheSpaceAroundWorkspaces()
    {
        MonitorKit kit;
        kit.add(0);
        const std::vector<double> offsets = kit.monitor.workspaceRenderOffsets();
        const Layout::Monitor::InsertTarget above = kit.monitor.insertTargetAt(QPointF(10, offsets.front() - 50));
        QVERIFY(!above.workspace.existing);
        QCOMPARE(above.workspace.newAt, std::size_t(0));
        const Layout::Monitor::InsertTarget below = kit.monitor.insertTargetAt(QPointF(10, offsets.back() + 5000));
        QVERIFY(!below.workspace.existing);
        QVERIFY(kit.monitor.insertTargetAt(QPointF(10, 10), false).workspace.existing);
    }

    void dropTargetsOfFixedWorkspacesAreAlwaysExisting()
    {
        MonitorKit kit(withFixed(3));
        const std::vector<double> offsets = kit.monitor.workspaceRenderOffsets();
        const Layout::Monitor::InsertTarget above = kit.monitor.insertTargetAt(QPointF(10, offsets.front() - 50));
        QVERIFY(above.workspace.existing);
        QCOMPARE(above.workspace.id, kit.monitor.workspaces().front().id());
        const Layout::Monitor::InsertTarget below = kit.monitor.insertTargetAt(QPointF(10, offsets.back() + 5000));
        QVERIFY(below.workspace.existing);
        QCOMPARE(below.workspace.id, kit.monitor.workspaces().back().id());
        const double gapY = offsets.at(1) - 1.0;
        QVERIFY(kit.monitor.insertTargetAt(QPointF(10, gapY)).workspace.existing);
    }

    void dropTargetsBetweenWorkspacesInsertANewOne()
    {
        MonitorKit kit;
        kit.add(0);
        kit.monitor.switchWorkspace(1);
        kit.add(1);
        const std::vector<double> offsets = kit.monitor.workspaceRenderOffsets();
        const double height = kit.monitor.workspaceHeightWithGap();
        const Layout::Monitor::InsertTarget between = kit.monitor.insertTargetAt(QPointF(10, offsets.at(1) - 1.0));
        if (height > kit.area.viewSize.height()) {
            QVERIFY(!between.workspace.existing);
        }
        QCOMPARE(kit.monitor.checkConsistency(), QString());
    }

    void removingFixedWorkspacesBeforeTheActiveOneShiftsIt()
    {
        MonitorKit kit(withFixed(3));
        kit.add(2);
        kit.monitor.switchWorkspace(2);
        Layout::Workspace extra(kit.area, kit.clock, kit.options, std::nullopt);
        kit.monitor.insertWorkspace(std::move(extra), 3, false);
        QCOMPARE(kit.monitor.checkConsistency(), QString());
    }

    void takingEveryFixedWorkspaceLeavesAnEmptyOne()
    {
        MonitorKit kit(withFixed(1));
        Layout::OutputArea other = kit.area;
        other.outputName = QStringLiteral("DP-2");
        other.outputId = QStringLiteral("DP-2");
        Layout::WorkspaceId homed = 0;
        {
            Layout::Workspace workspace(kit.area, kit.clock, kit.options, namedWorkspace(QStringLiteral("away"), QStringLiteral("DP-2")));
            homed = workspace.id();
            kit.monitor.insertWorkspace(std::move(workspace), 0, false);
        }
        QVERIFY(kit.monitor.indexOfWorkspace(homed).has_value());
        const std::vector<Layout::Workspace> taken = kit.monitor.takeWorkspacesForOutput(other);
        QCOMPARE(taken.size(), std::size_t(1));
        QCOMPARE(taken.front().id(), homed);
        QVERIFY(!kit.monitor.workspaces().empty());
        QCOMPARE(kit.monitor.checkConsistency(), QString());
    }

    void clearingTheNameOfAnUnknownWorkspaceFails()
    {
        MonitorKit kit;
        QVERIFY(!kit.monitor.clearWorkspaceName(9999));
        QVERIFY(kit.monitor.clearWorkspaceName(kit.monitor.workspaces().front().id()));
    }

    void emptyAboveFirstPrunesEmptyPairs()
    {
        MonitorKit kit(withEmptyAbove());
        kit.add(0);
        kit.monitor.switchWorkspace(0);
        const Layout::WorkspaceId occupied = kit.monitor.workspaces().at(1).id();
        QVERIFY(kit.monitor.indexOfWorkspace(occupied).has_value());
        kit.monitor.pruneWorkspaces();
        QCOMPARE(kit.monitor.checkConsistency(), QString());
    }
};

QTEST_GUILESS_MAIN(TestLayoutMonitorEdges)
#include "test_layout_monitoredges.moc"
