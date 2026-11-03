#include "striphelpers.h"

using namespace LayoutTest;

namespace
{

Layout::AddTileRequest tileRequest(Layout::AddTarget target, Layout::Activation activate, bool floating)
{
    Layout::AddTileRequest request;
    request.target = target;
    request.activate = activate;
    request.width = Layout::ColumnWidth::proportion(0.5);
    request.isFloating = floating;
    return request;
}

std::pair<Layout::WindowId, Layout::WindowId> addTiledAndFloating(MonitorKit &kit, Layout::Workspace &workspace)
{
    Layout::Tile tiled = kit.makeTile();
    const Layout::WindowId tiledId = tiled.id();
    workspace.addTile(std::move(tiled), tileRequest(Layout::AddTarget(), Layout::Activation::Always, false));
    Layout::Tile floating = kit.makeTile();
    const Layout::WindowId floatingId = floating.id();
    workspace.addTile(std::move(floating), tileRequest(Layout::AddTarget(), Layout::Activation::Always, true));
    return {tiledId, floatingId};
}

}

class TestLayoutWorkspaceEdges : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void floatingTilesNextToUnknownWindowsStillLand()
    {
        MonitorKit kit;
        Layout::Workspace &workspace = kit.monitor.workspaces().front();
        workspace.addTile(kit.makeTile(), tileRequest(Layout::AddTarget::besideWindow(999), Layout::Activation::Always, true));
        QCOMPARE(workspace.floating().tiles().size(), std::size_t(1));
        QCOMPARE(workspace.checkConsistency(), QString());
    }

    void floatingTilesOpenBesideTiledWindows()
    {
        MonitorKit kit;
        Layout::Workspace &workspace = kit.monitor.workspaces().front();
        Layout::Tile first = kit.makeTile();
        const Layout::WindowId anchor = first.id();
        workspace.addTile(std::move(first), tileRequest(Layout::AddTarget(), Layout::Activation::Always, false));
        workspace.addTile(kit.makeTile(), tileRequest(Layout::AddTarget::besideWindow(anchor), Layout::Activation::Always, true));
        QCOMPARE(workspace.floating().tiles().size(), std::size_t(1));
        QVERIFY(workspace.isFloatingFocused());
        QCOMPARE(workspace.checkConsistency(), QString());
    }

    void columnsCanOpenWithoutTakingFocus()
    {
        MonitorKit kit;
        Layout::Workspace &workspace = kit.monitor.workspaces().front();
        workspace.addTile(kit.makeTile(), tileRequest(Layout::AddTarget(), Layout::Activation::Always, false));
        Layout::Tile second = kit.makeTile();
        const Layout::WindowId id = second.id();
        workspace.addTile(std::move(second), tileRequest(Layout::AddTarget::atColumn(0), Layout::Activation::Never, false));
        QVERIFY(workspace.activeWindow() != std::optional(id));
        QCOMPARE(workspace.scrolling().columns().size(), std::size_t(2));
    }

    void floatingFocusBlocksRemovingColumns()
    {
        MonitorKit kit;
        Layout::Workspace &workspace = kit.monitor.workspaces().front();
        workspace.addTile(kit.makeTile(), tileRequest(Layout::AddTarget(), Layout::Activation::Always, false));
        workspace.addTile(kit.makeTile(), tileRequest(Layout::AddTarget(), Layout::Activation::Always, true));
        QVERIFY(workspace.isFloatingFocused());
        QVERIFY(!workspace.removeActiveColumn().has_value());
        workspace.focusTiling();
        QVERIFY(workspace.removeActiveColumn().has_value());
    }

    void unknownWindowsHaveNoRenderPosition()
    {
        MonitorKit kit;
        Layout::Workspace &workspace = kit.monitor.workspaces().front();
        Layout::Tile tile = kit.makeTile();
        const Layout::WindowId id = tile.id();
        workspace.addTile(std::move(tile), tileRequest(Layout::AddTarget(), Layout::Activation::Always, false));
        QVERIFY(workspace.tileRenderPosition(id).has_value());
        QVERIFY(!workspace.tileRenderPosition(999).has_value());
        QVERIFY(!workspace.animateOpening(999));
        QVERIFY(workspace.animateOpening(id));
    }

    void floatingWindowsAreHitOnlyWhenShown()
    {
        MonitorKit kit;
        Layout::Workspace &workspace = kit.monitor.workspaces().front();
        const auto [tiledId, floatingId] = addTiledAndFloating(kit, workspace);
        const QPointF floatingAt = workspace.tileRenderPosition(floatingId).value() + QPointF(5, 5);
        QVERIFY(workspace.windowUnder(floatingAt).has_value());
        workspace.focusTiling();
        workspace.setFullscreen(tiledId, true);
        workspace.scrolling().tileFor(tiledId)->window().commit(QSizeF(1280, 720));
        workspace.updateWindow(tiledId);
        kit.clock.setRawNow(kit.clock.rawNow() + std::chrono::seconds(5));
        workspace.tickAnimations();
        QVERIFY(!workspace.showsFloating());
        QVERIFY(workspace.windowUnder(QPointF(-5, -5)) == std::nullopt);
    }

    void edgeScrollingWithoutATriggerStillReports()
    {
        Config::Config config = instantConfig();
        config.gestures.dndEdgeViewScroll.triggerSize = 0.0;
        MonitorKit kit(config);
        Layout::Workspace &workspace = kit.monitor.workspaces().front();
        workspace.addTile(kit.makeTile(), tileRequest(Layout::AddTarget(), Layout::Activation::Always, false));
        workspace.beginEdgeScroll();
        workspace.edgeScrollBy(QPointF(0, 0), 1.0);
        workspace.endEdgeScroll();
    }

    void resizeSessionsEndWithTheirWindowOrAll()
    {
        MonitorKit kit;
        Layout::Workspace &workspace = kit.monitor.workspaces().front();
        const auto [tiledId, floatingId] = addTiledAndFloating(kit, workspace);
        const quint8 right = static_cast<quint8>(Layout::ResizeEdge::Right);
        QVERIFY(workspace.beginResize(floatingId, right));
        QVERIFY(workspace.updateResize(floatingId, QPointF(10, 0)));
        workspace.endResize(floatingId);
        QVERIFY(workspace.beginResize(tiledId, right));
        QVERIFY(workspace.updateResize(tiledId, QPointF(10, 0)));
        workspace.endResize(tiledId);
        QVERIFY(workspace.beginResize(tiledId, right));
        workspace.endResize(std::nullopt);
        QVERIFY(workspace.beginResize(floatingId, right));
        workspace.endResize(std::nullopt);
        QVERIFY(workspace.beginResize(tiledId, right));
    }

    void floatingToggleNeedsAKnownWindow()
    {
        MonitorKit kit;
        Layout::Workspace &workspace = kit.monitor.workspaces().front();
        workspace.toggleWindowFloating(std::nullopt);
        workspace.toggleWindowFloating(999);
        workspace.placeWindowFloating(999, true);
        Layout::Tile tile = kit.makeTile();
        const Layout::WindowId id = tile.id();
        workspace.addTile(std::move(tile), tileRequest(Layout::AddTarget(), Layout::Activation::Always, false));
        workspace.setFullscreen(id, true);
        workspace.scrolling().tileFor(id)->coversLockedOutput = true;
        workspace.toggleWindowFloating(id);
        QVERIFY(workspace.isFloating(id));
        workspace.toggleWindowFloating(id);
        QVERIFY(!workspace.isFloating(id));
    }

    void sizingUnknownWindowsDoesNothing()
    {
        MonitorKit kit;
        Layout::Workspace &workspace = kit.monitor.workspaces().front();
        workspace.setFullscreen(999, false);
        workspace.setFullscreen(999, true);
        workspace.setMaximized(999, true);
        workspace.setMaximized(999, false);
        QVERIFY(workspace.scrolling().isEmpty());
    }

    void floatingWindowsReturnWhenLeavingFullscreen()
    {
        MonitorKit kit;
        Layout::Workspace &workspace = kit.monitor.workspaces().front();
        Layout::Tile floating = kit.makeTile();
        const Layout::WindowId id = floating.id();
        workspace.addTile(std::move(floating), tileRequest(Layout::AddTarget(), Layout::Activation::Always, true));
        workspace.setFullscreen(id, false);
        QVERIFY(workspace.isFloating(id));
        workspace.setFullscreen(id, true);
        QVERIFY(!workspace.isFloating(id));
        workspace.scrolling().tileFor(id)->window().commit(QSizeF(1280, 720));
        workspace.updateWindow(id);
        workspace.setFullscreen(id, false);
        QCOMPARE(workspace.checkConsistency(), QString());
    }
};

QTEST_GUILESS_MAIN(TestLayoutWorkspaceEdges)
#include "test_layout_workspaceedges.moc"
