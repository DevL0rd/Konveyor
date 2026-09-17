#include "layout/engine/engineprivate.h"

#include "layout/common/geometry.h"

#include <algorithm>

namespace Konveyor::Layout
{

bool Engine::Private::appHasWindow(const QString &appId) const
{
    if (appId.isEmpty()) {
        return false;
    }
    const auto ownsWindow = [&appId](const Workspace &workspace) {
        const auto isApp = [&appId](const Tile &tile) { return tile.window().properties().appId == appId; };
        return std::ranges::any_of(workspace.floating().tiles(), isApp)
            || std::ranges::any_of(
                workspace.scrolling().columns(), [&isApp](const Column &column) { return std::ranges::any_of(column.tiles, isApp); });
    };
    for (const Monitor &monitor : monitors) {
        if (std::ranges::any_of(monitor.workspaces(), ownsWindow)) {
            return true;
        }
    }
    return std::ranges::any_of(orphanWorkspaces, ownsWindow);
}

const Config::Layout &Engine::Private::layoutForMonitor(std::size_t monitorIndex) const
{
    if (monitorIndex < monitors.size() && monitors[monitorIndex].layoutOverride()) {
        return *monitors[monitorIndex].layoutOverride();
    }
    return config.layout;
}

namespace
{

std::vector<std::size_t> stackTargets(const ColumnStrip &strip, const QString &appId, Config::GroupAppWindows mode, bool stackPlacement)
{
    std::vector<std::size_t> columns = mode == Config::GroupAppWindows::Off ? std::vector<std::size_t> {} : strip.appColumnIndices(appId);
    if (mode == Config::GroupAppWindows::Beside && !columns.empty()) {
        return {columns.back()};
    }
    if (!columns.empty() || !stackPlacement || strip.activeColumnIndex() >= strip.columns().size()) {
        return columns;
    }
    const Column &active = strip.columns()[strip.activeColumnIndex()];
    if (active.fillsWidth || active.requestedMode() != WindowMode::Normal) {
        return {};
    }
    return {strip.activeColumnIndex()};
}

}

namespace
{
bool appHasColumn(const Workspace &workspace, const Tile &tile)
{
    return !workspace.scrolling().appColumnIndices(tile.window().properties().appId).empty();
}
}

bool Engine::Private::placeInAppGroup(Tile &tile, const NewWindowPlan &plan, Workspace &workspace, MonitorAddRequest &request)
{
    if (plan.isFloating || plan.parent || plan.workspace || plan.fillsWidth || plan.wantsFullscreen || plan.wantsMaximized) {
        return false;
    }
    const Config::Layout &layout = layoutForMonitor(plan.monitorIndex);
    const Config::GroupAppWindows mode = plan.rules.groupAppWindows.value_or(layout.groupAppWindows);
    const bool stackPlacement = plan.rules.newWindowPlacement.value_or(layout.newWindowPlacement) == Config::NewWindowPlacement::Stack;
    const std::vector<std::size_t> columns = stackTargets(workspace.scrolling(), tile.window().properties().appId, mode, stackPlacement);
    if (columns.empty()) {
        return false;
    }
    const bool activate = resolveActivation(plan.activate, !workspace.activeTileWantsFullscreen());
    if (mode != Config::GroupAppWindows::Stack && !stackPlacement) {
        request.target = MonitorAddTarget::besideWindow(workspace.scrolling().columns()[columns.back()].tiles.back().id());
        request.activate = activate ? Activation::Always : Activation::Never;
        return false;
    }
    const AppStackRequest stack {
        .activate = activate,
        .maxRows = static_cast<std::size_t>(std::max(1, plan.rules.maxRowsPerColumn.value_or(layout.maxRowsPerColumn))),
        .byPlacement = stackPlacement && (mode != Config::GroupAppWindows::Stack || !appHasColumn(workspace, tile)),
    };
    stackIntoColumns(plan.monitorIndex, workspace, columns, std::move(tile), stack);
    return true;
}

void Engine::Private::stackIntoColumns(
    std::size_t monitorIndex, Workspace &workspace, const std::vector<std::size_t> &columns, Tile tile, const AppStackRequest &stack)
{
    const WindowId window = tile.id();
    const WorkspaceId workspaceId = workspace.id();
    if (monitorIndex < monitors.size()) {
        Monitor &monitor = monitors[monitorIndex];
        if (const auto index = monitor.indexOfWorkspace(workspaceId)) {
            monitor.addToAppStack(*index, columns, std::move(tile), stack.activate, stack.maxRows);
            if (stack.byPlacement) {
                monitor.workspaces()[*monitor.indexOfWorkspace(workspaceId)].scrolling().markPlacementStack(window);
            }
            return;
        }
    }
    workspace.addToAppStack(columns, std::move(tile), stack.activate, stack.maxRows);
    if (stack.byPlacement) {
        workspace.scrolling().markPlacementStack(window);
    }
}

void Engine::Private::reflowMonitorLayout(Monitor &monitor, const Config::Layout &previous)
{
    const Config::Layout &current = monitor.layoutOverride() ? *monitor.layoutOverride() : config.layout;
    if (previous.newWindowPlacement == current.newWindowPlacement && previous.defaultColumnWidth == current.defaultColumnWidth
        && previous.maxRowsPerColumn == current.maxRowsPerColumn) {
        return;
    }
    for (Workspace &workspace : monitor.workspaces()) {
        workspace.scrolling().reflowForLayout(previous, current);
    }
}

}
