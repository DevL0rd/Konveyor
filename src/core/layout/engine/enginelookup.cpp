#include "layout/engine/engineprivate.h"

#include "layout/common/geometry.h"

#include <algorithm>
#include <chrono>
#include <utility>

namespace Konveyor::Layout
{

OutputArea Engine::Private::areaFor(const OutputInfo &info) const
{
    OutputArea area;
    area.outputName = info.name;
    area.outputId = info.makeModelSerial.isEmpty() ? info.name : info.makeModelSerial;
    area.viewSize = info.geometry.size();
    area.workingArea = QRectF(info.workArea.topLeft() - info.geometry.topLeft(), info.workArea.size());
    area.scale = info.scale > 0.0 ? info.scale : 1.0;
    return area;
}

QPointF Engine::Private::originOf(const QString &outputName) const
{
    const auto it = outputInfos.constFind(outputName);
    return it == outputInfos.constEnd() ? QPointF() : it->geometry.topLeft();
}

Monitor *Engine::Private::activeMonitor()
{
    if (monitors.empty()) {
        return nullptr;
    }
    return &monitors[std::min(activeMonitorIndex, monitors.size() - 1)];
}

Workspace *Engine::Private::activeWorkspace()
{
    if (Monitor *monitor = activeMonitor()) {
        return &monitor->activeWorkspace();
    }
    return orphanWorkspaces.empty() ? nullptr : &orphanWorkspaces[0];
}

std::optional<std::size_t> Engine::Private::monitorIndexByName(const QString &name) const
{
    for (std::size_t idx = 0; idx < monitors.size(); ++idx) {
        if (monitors[idx].outputName().compare(name, Qt::CaseInsensitive) == 0) {
            return idx;
        }
    }
    return std::nullopt;
}

Monitor *Engine::Private::monitorByName(const QString &name)
{
    const auto idx = monitorIndexByName(name);
    return idx ? &monitors[*idx] : nullptr;
}

std::optional<std::size_t> Engine::Private::monitorIndexOf(WindowId id) const
{
    for (std::size_t idx = 0; idx < monitors.size(); ++idx) {
        if (monitors[idx].hasWindow(id)) {
            return idx;
        }
    }
    return std::nullopt;
}

Monitor *Engine::Private::monitorOf(WindowId id)
{
    const auto idx = monitorIndexOf(id);
    return idx ? &monitors[*idx] : nullptr;
}

std::vector<Workspace *> Engine::Private::allWorkspaces()
{
    std::vector<Workspace *> result;
    for (Monitor &monitor : monitors) {
        for (Workspace &workspace : monitor.workspaces()) {
            result.push_back(&workspace);
        }
    }
    for (Workspace &workspace : orphanWorkspaces) {
        result.push_back(&workspace);
    }
    return result;
}

Workspace *Engine::Private::workspaceOf(WindowId id)
{
    for (Workspace *workspace : allWorkspaces()) {
        if (workspace->hasWindow(id)) {
            return workspace;
        }
    }
    return nullptr;
}

Workspace *Engine::Private::workspaceById(WorkspaceId id)
{
    for (Workspace *workspace : allWorkspaces()) {
        if (workspace->id() == id) {
            return workspace;
        }
    }
    return nullptr;
}

std::optional<WindowId> Engine::Private::target(std::optional<WindowId> requested) const
{
    if (requested) {
        return requested;
    }
    return windowDrag && windowDrag->moving ? std::optional(windowDrag->window) : focused;
}

Workspace *Engine::Private::workspaceForTarget(std::optional<WindowId> requested)
{
    if (requested) {
        return workspaceOf(*requested);
    }
    return activeWorkspace();
}

std::optional<WindowId> Engine::windowAt(const QPointF &globalPos) const
{
    const std::optional<WindowHit> hit = hitAt(globalPos);
    return hit ? std::optional(hit->id) : std::nullopt;
}

std::optional<WindowHit> Engine::hitAt(const QPointF &globalPos) const
{
    for (const Monitor &monitor : d->monitors) {
        const QPointF origin = d->originOf(monitor.outputName());
        const QRectF geometry(origin, monitor.area().viewSize);
        if (!geometry.contains(globalPos)) {
            continue;
        }
        const QPointF local = globalPos - origin;
        const std::vector<double> offsets = monitor.workspaceRenderOffsets();
        for (std::size_t idx = 0; idx < monitor.workspaces().size(); ++idx) {
            const auto hit = monitor.workspaces()[idx].windowUnder(local - QPointF(0.0, offsets[idx]));
            if (hit) {
                return hit;
            }
        }
    }
    return std::nullopt;
}

}
