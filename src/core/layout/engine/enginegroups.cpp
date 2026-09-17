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

bool Engine::Private::placeInAppGroup(Tile &tile, const NewWindowPlan &plan, Workspace &workspace, MonitorAddRequest &request)
{
    if (plan.isFloating || plan.parent || plan.workspace || plan.fillsWidth || plan.wantsFullscreen || plan.wantsMaximized) {
        return false;
    }
    const Config::GroupAppWindows mode = plan.rules.groupAppWindows.value_or(config.layout.groupAppWindows);
    const std::vector<std::size_t> appColumns = workspace.scrolling().appColumnIndices(tile.window().properties().appId);
    if (mode == Config::GroupAppWindows::Off || appColumns.empty()) {
        return false;
    }
    const bool activate = resolveActivation(plan.activate, !workspace.activeTileWantsFullscreen());
    if (mode == Config::GroupAppWindows::Beside) {
        request.target = MonitorAddTarget::besideWindow(workspace.scrolling().columns()[appColumns.back()].tiles.back().id());
        request.activate = activate ? Activation::Always : Activation::Never;
        return false;
    }
    const auto maxRows = static_cast<std::size_t>(std::max(1, plan.rules.maxRowsPerColumn.value_or(config.layout.maxRowsPerColumn)));
    if (plan.monitorIndex < monitors.size()) {
        Monitor &monitor = monitors[plan.monitorIndex];
        if (const auto index = monitor.indexOfWorkspace(workspace.id())) {
            monitor.addToAppStack(*index, appColumns, std::move(tile), activate, maxRows);
            return true;
        }
    }
    workspace.addToAppStack(appColumns, std::move(tile), activate, maxRows);
    return true;
}

}
