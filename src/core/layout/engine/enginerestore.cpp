#include "layout/engine/engineprivate.h"

#include <algorithm>

namespace Konveyor::Layout
{

void Engine::addWindow(WindowId id, const WindowProperties &properties, const QString &preferredOutput, ActivationPolicy policy,
    const std::optional<RestorePlacement> &restore)
{
    if (hasWindow(id)) {
        return;
    }
    NewWindowPlan plan = d->planNewWindow(properties, preferredOutput, policy);
    const bool restoring = restore && d->workspaceById(restore->workspace);
    if (restoring) {
        plan.isFloating = restore->isFloating;
        for (std::size_t idx = 0; idx < d->monitors.size(); ++idx) {
            if (d->monitors[idx].indexOfWorkspace(restore->workspace)) {
                plan.monitorIndex = idx;
            }
        }
    }
    d->placeNewWindow(id, properties, plan, restoring ? restore : std::nullopt);
    if (restoring && restore->isFloating) {
        setFloatingFrame(id, restore->floatingFrame);
    }
    d->refresh();
}

void Engine::removeWindow(WindowId id)
{
    if (d->windowDrag && d->windowDrag->window == id) {
        d->windowDrag.reset();
    }
    for (Monitor &monitor : d->monitors) {
        for (Workspace &workspace : monitor.workspaces()) {
            if (!workspace.hasWindow(id)) {
                continue;
            }
            workspace.removeTile(id);
            monitor.pruneWorkspaces();
            d->focusOrder.remove(id);
            d->refresh();
            return;
        }
    }
    for (Workspace &workspace : d->orphanWorkspaces) {
        if (workspace.hasWindow(id)) {
            workspace.removeTile(id);
            d->focusOrder.remove(id);
            d->refresh();
            return;
        }
    }
}

std::optional<RestorePlacement> Engine::placementOf(WindowId id) const
{
    const Workspace *workspace = const_cast<Private &>(*d).workspaceOf(id);
    if (!workspace) {
        return std::nullopt;
    }
    RestorePlacement placement;
    placement.workspace = workspace->id();
    if (workspace->isFloating(id)) {
        placement.isFloating = true;
        placement.floatingFrame = windowState(id).value_or(WindowState()).targetFrame;
        return placement;
    }
    const std::vector<Column> &columns = workspace->scrolling().columns();
    for (std::size_t c = 0; c < columns.size(); ++c) {
        const auto position = columns[c].position(id);
        if (!position) {
            continue;
        }
        placement.columnIndex = c;
        placement.width = columns[c].widthSetting;
        if (columns[c].tiles.size() > 1) {
            placement.tileIndex = position;
        }
        return placement;
    }
    return std::nullopt;
}

bool Engine::Private::placeRestored(Tile &tile, const NewWindowPlan &plan, const RestorePlacement &restore, MonitorAddRequest &request)
{
    Workspace *workspace = workspaceById(restore.workspace);
    request.target = MonitorAddTarget::onWorkspace(restore.workspace);
    if (!workspace || restore.isFloating) {
        return false;
    }
    const std::size_t columnCount = workspace->scrolling().columns().size();
    request.width = restore.width;
    if (!restore.tileIndex || restore.columnIndex >= columnCount) {
        request.target = MonitorAddTarget::onWorkspace(restore.workspace, std::min(restore.columnIndex, columnCount));
        return false;
    }
    const std::size_t tileCount = workspace->scrolling().columns()[restore.columnIndex].tiles.size();
    const std::size_t tileIndex = std::min(*restore.tileIndex, tileCount);
    const bool activate = resolveActivation(plan.activate, !workspace->activeTileWantsFullscreen());
    for (Monitor &monitor : monitors) {
        if (const auto index = monitor.indexOfWorkspace(restore.workspace)) {
            monitor.insertIntoColumn(*index, restore.columnIndex, tileIndex, std::move(tile), activate, true);
            return true;
        }
    }
    workspace->insertIntoColumn(restore.columnIndex, tileIndex, std::move(tile), activate);
    return true;
}

}
