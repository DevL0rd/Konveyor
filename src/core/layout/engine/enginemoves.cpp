#include "layout/engine/engineprivate.h"

#include <algorithm>
#include <cmath>
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

struct Candidate
{
    std::optional<std::size_t> index;
    double primary = 0;
    double secondary = 0;

    bool isBetterThan(double otherPrimary, double otherSecondary) const
    {
        return !index || otherPrimary < primary || (otherPrimary == primary && otherSecondary < secondary);
    }
};

std::optional<std::size_t> nearestInDirection(const std::vector<QPointF> &centers, std::size_t active, DirectionAxis axis)
{
    Candidate best;
    for (std::size_t idx = 0; idx < centers.size(); ++idx) {
        const double delta = axisValue(centers[idx], axis.horizontal) - axisValue(centers[active], axis.horizontal);
        const double primary = axis.positive ? delta : -delta;
        if (idx == active || primary <= 0.0) {
            continue;
        }
        const double secondary = std::abs(axisValue(centers[idx], !axis.horizontal) - axisValue(centers[active], !axis.horizontal));
        if (best.isBetterThan(primary, secondary)) {
            best = {idx, primary, secondary};
        }
    }
    return best.index;
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

    std::vector<QPointF> centers;
    centers.reserve(count);
    for (const Monitor &monitor : monitors) {
        centers.push_back(outputInfos.value(monitor.outputName()).geometry.center());
    }
    return nearestInDirection(centers, active, *axis);
}

void Engine::Private::moveWindowToMonitor(std::optional<WindowId> window, std::size_t monitorIndex, bool activate)
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
    DetachedTile removed = workspace->removeTile(*id);

    MonitorAddRequest request;
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

void Engine::Private::moveColumnToMonitor(std::size_t monitorIndex, bool activate)
{
    if (monitorIndex >= monitors.size() || monitorIndex == activeMonitorIndex || monitors.empty()) {
        return;
    }
    const std::size_t sourceIndex = std::min(activeMonitorIndex, monitors.size() - 1);
    auto column = monitors[sourceIndex].activeWorkspace().removeActiveColumn();
    if (!column) {
        return;
    }
    const std::size_t targetWorkspace = monitors[monitorIndex].activeWorkspaceIndex();
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

}
