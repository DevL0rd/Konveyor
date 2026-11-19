#include "layout/engine/engineprivate.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace Konveyor::Layout
{

namespace
{

double overviewScrollUnits(const Config::Overview &config, bool finger, bool horizontal, const OverviewScrollAmount &amount)
{
    const double speed = std::clamp(finger ? (horizontal ? config.touchpadScrollSpeedX : config.touchpadScrollSpeedY) : 1.0, 0.05, 2.0);
    return amount.units() * speed;
}

}

bool Engine::isOverviewEnabled() const
{
    return d->config.overview.enabled;
}

double Engine::overviewProgress() const
{
    return d->overview.progress;
}

QList<OverviewView> Engine::overviewViews() const
{
    return d->overviewViews();
}

std::optional<OverviewView> Engine::overviewView(const QString &output) const
{
    return d->overviewView(output);
}

void Engine::setOverviewReserved(const QString &output, const QMarginsF &reserved)
{
    if (d->overview.reserved.value(output) != reserved) {
        d->overview.reserved.insert(output, reserved);
    }
}

void Engine::overviewPan(const QString &output, const QPointF &delta)
{
    if (!d->builtinOverviewOpen() || delta.isNull()) {
        return;
    }
    const std::optional<OverviewView> view = d->overviewView(output);
    if (!view) {
        return;
    }
    const QRectF bounds = view->contentBounds.united(QRectF(view->camera, view->outputGeometry.size()));
    const QPointF moved = view->camera + delta;
    d->stopOverviewCameraPan(output);
    d->overview.cameras.insert(output, clampOverviewCamera(bounds, view->outputGeometry.size(), moved));
}

std::optional<WindowId> Engine::Private::centeredOverviewWindow(const OverviewView &view) const
{
    const QPointF center = view.outputGeometry.center();
    const int activeReal = view.activeWorkspace >= 0 && view.activeWorkspace < view.workspaceIndices.size()
        ? view.workspaceIndices.at(view.activeWorkspace)
        : view.activeWorkspace;
    const int activeWorkspace = activeReal + 1;
    std::optional<WindowId> best;
    double bestDist = std::numeric_limits<double>::max();
    for (const OverviewWindow &window : view.windows) {
        if (window.workspaceIndex != activeWorkspace) {
            continue;
        }
        const QPointF diff = window.rect.center() - center;
        const double dist = diff.x() * diff.x() + diff.y() * diff.y();
        if (dist < bestDist) {
            bestDist = dist;
            best = window.id;
        }
    }
    if (best) {
        return best;
    }
    return best;
}

void Engine::overviewScroll(const QString &output, Qt::Orientation orientation, const OverviewScrollAmount &amount, bool shift, bool finger,
    std::optional<QPointF> pointer)
{
    if (!d->builtinOverviewOpen()) {
        return;
    }
    Monitor *monitor = d->monitorByName(output);
    if (!monitor) {
        return;
    }
    const bool horizontal = orientation == Qt::Horizontal || shift;
    const double units = overviewScrollUnits(d->config.overview, finger, horizontal, amount);
    if (units == 0.0) {
        return;
    }
    if (horizontal) {
        Workspace *scrolled = d->overviewWorkspaceUnderPointer(*monitor, output, pointer);
        if (!d->stepOverviewHorizontal(*scrolled, units)) {
            return;
        }
        if (scrolled != &monitor->activeWorkspace()) {
            d->stopOverviewCameraPan(output);
            d->overview.cameras.remove(output);
            d->overview.selected = scrolled->activeWindow();
            d->focusOverviewMonitor(output);
            return;
        }
        d->settleOverviewAfterWorkspaceChange(output, *monitor, false, monitor->activeWorkspace().overviewPanned(), 0.0);
        return;
    }
    const double beforeCameraY = d->overviewCameraY(output).value_or(0.0);
    if (!d->stepOverviewVertical(*monitor, units)) {
        return;
    }
    d->settleOverviewAfterWorkspaceChange(output, *monitor, true, monitor->activeWorkspace().overviewPanned(), beforeCameraY);
}

bool Engine::Private::stepOverviewVertical(Monitor &monitor, double units)
{
    overview.verticalScrollAccum += units;
    bool changed = false;
    while (std::abs(overview.verticalScrollAccum) >= 120.0) {
        const int step = overview.verticalScrollAccum > 0.0 ? 1 : -1;
        overview.verticalScrollAccum -= step * 120.0;
        const std::size_t before = monitor.activeWorkspaceIndex();
        if (step > 0) {
            monitor.switchWorkspaceDown();
        } else {
            monitor.switchWorkspaceUp();
        }
        changed = changed || monitor.activeWorkspaceIndex() != before;
    }
    return changed;
}

bool Engine::Private::stepOverviewHorizontal(Workspace &workspace, double units)
{
    overview.horizontalScrollAccum += units;
    bool changed = false;
    while (std::abs(overview.horizontalScrollAccum) >= 120.0) {
        const int step = overview.horizontalScrollAccum > 0.0 ? 1 : -1;
        overview.horizontalScrollAccum -= step * 120.0;
        const bool moved = step > 0 ? workspace.focusRight(config.layout.wrapColumns) : workspace.focusLeft(config.layout.wrapColumns);
        changed = changed || moved;
    }
    return changed;
}

Workspace *Engine::Private::overviewWorkspaceUnderPointer(Monitor &monitor, const QString &output, const std::optional<QPointF> &pointer)
{
    Workspace *scrolled = &monitor.activeWorkspace();
    if (!pointer) {
        return scrolled;
    }
    const std::optional<OverviewView> view = overviewView(output);
    if (!view) {
        return scrolled;
    }
    for (qsizetype index = view->windows.size(); index > 0; --index) {
        const OverviewWindow &window = view->windows.at(index - 1);
        if (!window.rect.contains(*pointer)) {
            continue;
        }
        if (window.workspaceIndex > 0) {
            const auto workspaceIndex = static_cast<std::size_t>(window.workspaceIndex - 1);
            if (workspaceIndex < monitor.workspaces().size()) {
                scrolled = &monitor.workspaces()[workspaceIndex];
            }
        }
        break;
    }
    return scrolled;
}

std::optional<double> Engine::Private::overviewCameraY(const QString &output) const
{
    if (!config.animations.enabled) {
        return std::nullopt;
    }
    const std::optional<OverviewView> view = overviewView(output);
    if (!view) {
        return std::nullopt;
    }
    return view->camera.y();
}

void Engine::Private::settleOverviewAfterWorkspaceChange(
    const QString &output, const Monitor &monitor, bool workspaceChanged, bool panned, double beforeCameraY)
{
    stopOverviewCameraPan(output);
    overview.cameras.remove(output);
    const std::optional<OverviewView> afterView = (workspaceChanged && config.animations.enabled)
            || !monitor.activeWorkspace().activeWindow() || (workspaceChanged && monitor.activeWorkspace().overviewPanned())
        ? overviewView(output)
        : std::nullopt;
    if (workspaceChanged) {
        restartOverviewCameraY(output, afterView, beforeCameraY);
    }
    std::optional<WindowId> next = (workspaceChanged && panned) ? (afterView ? centeredOverviewWindow(*afterView) : std::nullopt)
                                                                : monitor.activeWorkspace().activeWindow();
    if (!next && afterView) {
        next = centeredOverviewWindow(*afterView);
    }
    overview.selected = next;
    focusOverviewMonitor(output);
}

void Engine::Private::restartOverviewCameraY(const QString &output, const std::optional<OverviewView> &afterView, double beforeCameraY)
{
    overview.cameraYAnimations.remove(output);
    if (!config.animations.enabled || !afterView) {
        return;
    }
    const double afterCameraY = afterView->camera.y();
    if (!qFuzzyCompare(beforeCameraY, afterCameraY)) {
        overview.cameraYAnimations.insert(
            output, Anim::Animation(clock, beforeCameraY, afterCameraY, 0.0, config.animations.workspaceSwitch));
    }
}

void Engine::Private::focusOverviewMonitor(const QString &output)
{
    if (const auto index = monitorIndexByName(output)) {
        activeMonitorIndex = *index;
    }
    refresh();
}

void Engine::overviewNavigate(OverviewDirection direction)
{
    if (!d->builtinOverviewOpen()) {
        return;
    }
    Monitor *monitor = d->activeMonitor();
    if (!monitor) {
        return;
    }
    const QString output = monitor->outputName();
    const bool vertical = direction == OverviewDirection::Up || direction == OverviewDirection::Down;
    const double beforeCameraY = vertical ? d->overviewCameraY(output).value_or(0.0) : 0.0;
    bool workspaceChanged = false;
    bool changed = false;
    if (vertical) {
        const std::size_t before = monitor->activeWorkspaceIndex();
        if (direction == OverviewDirection::Down) {
            monitor->switchWorkspaceDown();
        } else {
            monitor->switchWorkspaceUp();
        }
        workspaceChanged = monitor->activeWorkspaceIndex() != before;
        changed = workspaceChanged;
    } else {
        Workspace &workspace = monitor->activeWorkspace();
        changed = direction == OverviewDirection::Right ? workspace.focusRight(d->config.layout.wrapColumns)
                                                        : workspace.focusLeft(d->config.layout.wrapColumns);
    }
    if (!changed) {
        return;
    }
    d->settleOverviewAfterWorkspaceChange(output, *monitor, workspaceChanged, monitor->activeWorkspace().overviewPanned(), beforeCameraY);
}

}
