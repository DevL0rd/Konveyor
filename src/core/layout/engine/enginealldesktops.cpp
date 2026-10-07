#include "layout/engine/engineprivate.h"

#include <algorithm>
#include <utility>

namespace Konveyor::Layout
{

namespace
{

std::vector<WindowId> windowsOnAllDesktopsAway(Monitor &monitor)
{
    std::vector<WindowId> found;
    const WorkspaceId active = monitor.activeWorkspace().id();
    for (Workspace &workspace : monitor.workspaces()) {
        if (workspace.id() == active) {
            continue;
        }
        for (const TileRef &ref : workspace.renderedTilesMut(false)) {
            if (ref.tile->window().properties().onAllDesktops) {
                found.push_back(ref.tile->id());
            }
        }
    }
    return found;
}

}

void Engine::Private::followWindowsOnAllDesktops()
{
    for (Monitor &monitor : monitors) {
        const std::vector<WindowId> away = windowsOnAllDesktopsAway(monitor);
        for (const WindowId id : away) {
            Workspace *workspace = workspaceOf(id);
            const std::optional<std::size_t> column = workspace->tiledColumnIndex(id);
            DetachedTile removed = workspace->removeTile(id);
            Workspace &active = monitor.activeWorkspace();
            MonitorAddRequest request;
            const std::size_t columns = active.scrolling().columns().size();
            request.target = MonitorAddTarget::onWorkspace(active.id(), column ? std::optional(std::min(*column, columns)) : std::nullopt);
            request.activate = Activation::Never;
            request.allowActivateWorkspace = false;
            request.width = removed.width;
            request.fillsWidth = removed.fillsWidth;
            request.isFloating = removed.isFloating;
            monitor.addTile(std::move(removed.tile), request);
        }
        if (!away.empty()) {
            monitor.pruneWorkspaces();
        }
    }
}

}
