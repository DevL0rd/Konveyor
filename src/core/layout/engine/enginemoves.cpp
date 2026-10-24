#include "layout/engine/engineprivate.h"

#include <algorithm>
#include <utility>

namespace Konveyor::Layout
{

namespace
{

struct DirectionAxis
{
    bool horizontal = true;
    bool positive = true;
};

std::optional<DirectionAxis> axisFor(const QString &direction)
{
    if (direction == QLatin1String("left")) {
        return DirectionAxis {true, false};
    }
    if (direction == QLatin1String("right")) {
        return DirectionAxis {true, true};
    }
    if (direction == QLatin1String("up")) {
        return DirectionAxis {false, false};
    }
    if (direction == QLatin1String("down")) {
        return DirectionAxis {false, true};
    }
    return std::nullopt;
}

double axisValue(QPointF point, bool horizontal)
{
    return horizontal ? point.x() : point.y();
}

bool overlapsAcross(const QRectF &candidate, const QRectF &active, bool horizontal)
{
    const double start = horizontal ? candidate.top() : candidate.left();
    const double end = horizontal ? candidate.bottom() : candidate.right();
    const double activeStart = horizontal ? active.top() : active.left();
    const double activeEnd = horizontal ? active.bottom() : active.right();
    return start < activeEnd && activeStart < end;
}

std::optional<std::size_t> nearestInDirection(const std::vector<QRectF> &geometries, std::size_t active, DirectionAxis axis)
{
    std::optional<std::size_t> best;
    double bestDistance = 0.0;
    const QPointF activeCenter = geometries[active].center();
    for (std::size_t idx = 0; idx < geometries.size(); ++idx) {
        const double delta = axisValue(geometries[idx].center(), axis.horizontal) - axisValue(activeCenter, axis.horizontal);
        const double distance = axis.positive ? delta : -delta;
        if (idx == active || distance <= 0.0 || !overlapsAcross(geometries[idx], geometries[active], axis.horizontal)) {
            continue;
        }
        if (!best || distance < bestDistance) {
            best = idx;
            bestDistance = distance;
        }
    }
    return best;
}

}

std::optional<std::size_t> Engine::Private::monitorInDirection(const QString &direction) const
{
    const std::size_t count = monitors.size();
    if (count == 0) {
        return std::nullopt;
    }
    const std::size_t active = std::min(activeMonitorIndex, count - 1);
    if (direction == QLatin1String("previous")) {
        return (active + count - 1) % count;
    }
    if (direction == QLatin1String("next")) {
        return (active + 1) % count;
    }
    const auto axis = axisFor(direction);
    if (!axis || count < 2) {
        return std::nullopt;
    }

    std::vector<QRectF> geometries;
    geometries.reserve(count);
    for (const Monitor &monitor : monitors) {
        geometries.push_back(outputInfos.value(monitor.outputName()).geometry);
    }
    return nearestInDirection(geometries, active, *axis);
}

void Engine::Private::moveWindowToMonitor(
    std::optional<WindowId> window, std::size_t monitorIndex, bool activate, std::optional<std::size_t> workspaceIndex)
{
    const auto id = target(window);
    if (!id || monitorIndex >= monitors.size()) {
        return;
    }
    const auto sourceIndex = monitorIndexOf(*id);
    if (!sourceIndex || *sourceIndex == monitorIndex) {
        return;
    }
    Workspace *workspace = workspaceOf(*id);
    if (!workspace) {
        return;
    }
    if (workspace->tileFor(*id)->isWindowedFullscreenFallback) {
        workspace->setFullscreen(*id, false);
    }
    DetachedTile removed = workspace->removeTile(*id);

    MonitorAddRequest request;
    if (workspaceIndex) {
        request.target = MonitorAddTarget::onWorkspace(monitors[monitorIndex].workspaces()[*workspaceIndex].id());
    }
    request.activate = activate ? Activation::Always : Activation::Never;
    request.width = removed.width;
    request.fillsWidth = removed.fillsWidth;
    request.isFloating = removed.isFloating;
    monitors[monitorIndex].addTile(std::move(removed.tile), request);
    monitors[*sourceIndex].pruneWorkspaces();
    if (activate) {
        activeMonitorIndex = monitorIndex;
    }
}

void Engine::Private::moveColumnToMonitor(std::size_t monitorIndex, bool activate, std::optional<std::size_t> workspaceIndex)
{
    if (monitorIndex >= monitors.size() || monitorIndex == activeMonitorIndex || monitors.empty()) {
        return;
    }
    const std::size_t sourceIndex = std::min(activeMonitorIndex, monitors.size() - 1);
    Workspace &source = monitors[sourceIndex].activeWorkspace();
    if (source.isFloatingFocused()) {
        moveWindowToMonitor(source.activeWindow(), monitorIndex, activate, workspaceIndex);
        return;
    }
    if (const Column *column = source.scrolling().activeColumn()) {
        for (const Tile &tile : column->tiles) {
            if (tile.isWindowedFullscreenFallback) {
                source.setFullscreen(tile.id(), false);
                break;
            }
        }
    }
    auto column = source.removeActiveColumn();
    if (!column) {
        return;
    }
    const std::size_t targetWorkspace = workspaceIndex.value_or(monitors[monitorIndex].activeWorkspaceIndex());
    monitors[monitorIndex].addColumn(targetWorkspace, std::move(*column), activate, std::nullopt);
    monitors[sourceIndex].pruneWorkspaces();
    if (activate) {
        activeMonitorIndex = monitorIndex;
    }
}

void Engine::Private::moveWorkspaceToMonitor(std::size_t monitorIndex)
{
    if (monitorIndex >= monitors.size() || monitorIndex == activeMonitorIndex || monitors.empty()) {
        return;
    }
    const std::size_t sourceIndex = std::min(activeMonitorIndex, monitors.size() - 1);
    Workspace workspace = monitors[sourceIndex].detachWorkspaceAt(monitors[sourceIndex].activeWorkspaceIndex());
    const std::size_t insertAt = monitors[monitorIndex].workspaces().size();
    monitors[monitorIndex].insertWorkspace(std::move(workspace), insertAt, true);
    activeMonitorIndex = monitorIndex;
}

std::vector<std::pair<std::size_t, WorkspaceId>> Engine::Private::occupiedWorkspacesOutside(std::size_t monitorIndex) const
{
    std::vector<std::pair<std::size_t, WorkspaceId>> result;
    for (std::size_t source = 0; source < monitors.size(); ++source) {
        if (source == monitorIndex) {
            continue;
        }
        for (const Workspace &workspace : monitors[source].workspaces()) {
            if (workspace.isOccupiedOrNamed()) {
                result.emplace_back(source, workspace.id());
            }
        }
    }
    return result;
}

bool Engine::Private::activateOnMonitor(std::size_t monitorIndex, const Workspace &workspace)
{
    const auto idx = monitors[monitorIndex].indexOfWorkspace(workspace.id());
    if (!idx) {
        return false;
    }
    monitors[monitorIndex].activateWorkspace(*idx);
    return true;
}

void Engine::Private::revealWindowOnMonitor(std::size_t monitorIndex, WindowId window, WorkspaceId fallback)
{
    Workspace *workspace = workspaceOf(window);
    if (!workspace) {
        workspace = workspaceById(fallback);
    }
    if (workspace && activateOnMonitor(monitorIndex, *workspace) && workspace->hasWindow(window)) {
        workspace->activateWindow(window);
    }
}

void Engine::Private::moveAllWindowsToMonitor(std::size_t monitorIndex)
{
    if (monitorIndex >= monitors.size() || monitors.empty()) {
        return;
    }

    const std::optional<WindowId> focusedWindow = focused;
    const bool focusedWasOnTarget = focusedWindow.has_value() && monitors[monitorIndex].hasWindow(*focusedWindow);
    const std::size_t previousActiveMonitor = std::min(activeMonitorIndex, monitors.size() - 1);
    const WorkspaceId targetWorkspace = monitors[monitorIndex].activeWorkspace().id();

    std::vector<Workspace> moved;
    for (const auto &[sourceIndex, id] : occupiedWorkspacesOutside(monitorIndex)) {
        if (const auto idx = monitors[sourceIndex].indexOfWorkspace(id)) {
            moved.push_back(monitors[sourceIndex].detachWorkspaceAt(*idx));
        }
    }
    monitors[monitorIndex].appendWorkspaces(std::move(moved));

    activeMonitorIndex = monitorIndex;
    if (focusedWindow && !focusedWasOnTarget) {
        revealWindowOnMonitor(monitorIndex, *focusedWindow, targetWorkspace);
    } else if (!focusedWindow && previousActiveMonitor == monitorIndex) {
        if (const Workspace *workspace = workspaceById(targetWorkspace)) {
            activateOnMonitor(monitorIndex, *workspace);
        }
    }
}

}
