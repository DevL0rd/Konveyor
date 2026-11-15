#include "layout/engine/engineprivate.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace Konveyor::Layout
{

namespace
{

const OverviewView *viewAt(const QList<OverviewView> &views, const QString &output)
{
    const auto found = std::ranges::find(views, output, &OverviewView::output);
    return found == views.end() ? nullptr : &*found;
}

OverviewTarget targetAt(const OverviewWindow &window, const QRectF &outputGeometry)
{
    return OverviewTarget {window.rect.center() - outputGeometry.topLeft()};
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
    d->overview.cameras.insert(output, clampOverviewCamera(bounds, view->outputGeometry.size(), moved));
}

std::optional<WindowId> Engine::Private::centeredOverviewWindow(const OverviewView &view) const
{
    const QPointF center = view.outputGeometry.center();
    const int activeWorkspace = view.activeWorkspace + 1;
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
    for (const OverviewWindow &window : view.windows) {
        const QPointF diff = window.rect.center() - center;
        const double dist = diff.x() * diff.x() + diff.y() * diff.y();
        if (dist < bestDist) {
            bestDist = dist;
            best = window.id;
        }
    }
    return best;
}

void Engine::overviewScroll(const QString &output, Qt::Orientation orientation, double delta, qint32 deltaV120, bool shift)
{
    if (!d->builtinOverviewOpen()) {
        return;
    }
    Monitor *monitor = d->monitorByName(output);
    if (!monitor) {
        return;
    }
    const double units = deltaV120 != 0 ? static_cast<double>(deltaV120) : delta * 8.0;
    if (units == 0.0) {
        return;
    }
    const bool horizontal = orientation == Qt::Horizontal || shift;
    bool changed = false;
    bool workspaceChanged = false;
    double beforeCameraY = 0.0;
    if (!horizontal) {
        if (d->config.animations.enabled) {
            if (const std::optional<OverviewView> beforeView = d->overviewView(output)) {
                beforeCameraY = beforeView->camera.y();
            }
        }
        d->overview.verticalScrollAccum += units;
        while (std::abs(d->overview.verticalScrollAccum) >= 120.0) {
            const int step = d->overview.verticalScrollAccum > 0.0 ? 1 : -1;
            d->overview.verticalScrollAccum -= step * 120.0;
            const std::size_t before = monitor->activeWorkspaceIndex();
            if (step > 0) {
                monitor->switchWorkspaceDown();
            } else {
                monitor->switchWorkspaceUp();
            }
            if (monitor->activeWorkspaceIndex() != before) {
                workspaceChanged = true;
                changed = true;
            }
        }
    } else if (d->config.animations.enabled) {
        constexpr double kHorizontalPanPerNotch = 140.0;
        const double panDelta = (units / 120.0) * kHorizontalPanPerNotch;
        changed = monitor->activeWorkspace().panView(panDelta);
    } else {
        d->overview.horizontalScrollAccum += units;
        while (std::abs(d->overview.horizontalScrollAccum) >= 120.0) {
            const int step = d->overview.horizontalScrollAccum > 0.0 ? 1 : -1;
            d->overview.horizontalScrollAccum -= step * 120.0;
            Workspace &workspace = monitor->activeWorkspace();
            const bool moved
                = step > 0 ? workspace.focusRight(d->config.layout.wrapColumns) : workspace.focusLeft(d->config.layout.wrapColumns);
            changed = changed || moved;
        }
    }
    if (!changed) {
        return;
    }
    d->overview.cameras.remove(output);
    const std::optional<OverviewView> afterView
        = (workspaceChanged && d->config.animations.enabled) || (horizontal && d->config.animations.enabled)
            || !monitor->activeWorkspace().activeWindow()
        ? d->overviewView(output)
        : std::nullopt;
    if (workspaceChanged) {
        d->overview.cameraYAnimations.remove(output);
        if (d->config.animations.enabled && afterView) {
            const double afterCameraY = afterView->camera.y();
            if (!qFuzzyCompare(beforeCameraY, afterCameraY)) {
                d->overview.cameraYAnimations.insert(
                    output, Anim::Animation(d->clock, beforeCameraY, afterCameraY, 0.0, d->config.animations.workspaceSwitch));
            }
        }
    }
    std::optional<WindowId> next = horizontal && d->config.animations.enabled
        ? (afterView ? d->centeredOverviewWindow(*afterView) : std::nullopt)
        : monitor->activeWorkspace().activeWindow();
    if (!next && afterView) {
        next = d->centeredOverviewWindow(*afterView);
    }
    d->overview.selected = next;
    if (const auto index = d->monitorIndexByName(output)) {
        d->activeMonitorIndex = *index;
    }
    d->refresh();
}

std::optional<WindowId> Engine::overviewSelectedWindow() const
{
    if (!d->overview.selected || !windowState(*d->overview.selected)) {
        return std::nullopt;
    }
    return d->overview.selected;
}

void Engine::overviewSelectWindow(std::optional<WindowId> id)
{
    if (id && !windowState(*id)) {
        return;
    }
    d->overview.selected = id;
    if (id) {
        d->revealOverviewSelection();
    }
}

bool Engine::overviewActivateSelected()
{
    const std::optional<WindowId> id = overviewSelectedWindow();
    if (!id || !d->focusWindow(*id)) {
        return false;
    }
    if (d->overviewOpen) {
        setOverviewOpen(false);
    }
    return true;
}

void Engine::Private::revealOverviewSelection()
{
    if (!overview.selected) {
        return;
    }
    const QList<OverviewView> views = overviewViews();
    for (const OverviewView &view : views) {
        for (const OverviewWindow &window : view.windows) {
            if (window.id != *overview.selected) {
                continue;
            }
            const QRectF local = window.rect.translated(-view.outputGeometry.topLeft());
            const QRectF target = local.translated(view.camera);
            const QRectF bounds = view.contentBounds.united(QRectF(view.camera, view.outputGeometry.size()));
            overview.cameras.insert(view.output, overviewRevealCamera(bounds, view.outputGeometry.size(), view.camera, target));
            return;
        }
    }
}

namespace
{

const OverviewView *viewOfSelection(const QList<OverviewView> &views, std::optional<WindowId> selected)
{
    for (const OverviewView &view : views) {
        if (std::ranges::any_of(view.windows, [selected](const OverviewWindow &window) { return window.id == selected; })) {
            return &view;
        }
    }
    return nullptr;
}

}

void Engine::overviewMoveSelection(OverviewDirection direction)
{
    const QList<OverviewView> views = overviewViews();
    if (views.isEmpty()) {
        return;
    }
    const OverviewView *view = viewOfSelection(views, overviewSelectedWindow());
    if (!view) {
        if (const Monitor *monitor = d->activeMonitor()) {
            view = viewAt(views, monitor->outputName());
        }
    }
    if (!view) {
        view = &views.first();
    }

    QList<OverviewTarget> targets;
    int current = -1;
    for (qsizetype index = 0; index < view->windows.size(); ++index) {
        targets.append(targetAt(view->windows.at(index), view->outputGeometry));
        if (view->windows.at(index).id == overviewSelectedWindow()) {
            current = static_cast<int>(index);
        }
    }
    const std::optional<int> next = overviewNextTarget(targets, current, direction);
    if (next) {
        overviewSelectWindow(view->windows.at(*next).id);
    }
}

void Engine::overviewCycleSelection(bool forward)
{
    const QList<OverviewView> views = overviewViews();
    const OverviewView *view = viewOfSelection(views, overviewSelectedWindow());
    if (!view && !views.isEmpty()) {
        view = &views.first();
    }
    if (!view) {
        return;
    }
    int current = -1;
    for (qsizetype index = 0; index < view->windows.size(); ++index) {
        if (view->windows.at(index).id == overviewSelectedWindow()) {
            current = static_cast<int>(index);
        }
    }
    if (const auto next = overviewCycledTarget(current, static_cast<int>(view->windows.size()), forward)) {
        overviewSelectWindow(view->windows.at(*next).id);
    }
}

void Engine::Private::startOverviewAnimation(bool open)
{
    const double target = open ? 1.0 : 0.0;
    if (!config.animations.enabled || qFuzzyCompare(overview.progress, target)) {
        overview.progress = target;
        overview.progressAnimation.reset();
        return;
    }
    overview.progressAnimation.emplace(clock, overview.progress, target, 0.0, config.animations.workspaceSwitch);
}

void Engine::Private::tickOverview()
{
    for (auto it = overview.cameraYAnimations.begin(); it != overview.cameraYAnimations.end();) {
        if (it->isFinished()) {
            it = overview.cameraYAnimations.erase(it);
        } else {
            ++it;
        }
    }
    if (!overview.progressAnimation) {
        return;
    }
    if (overview.progressAnimation->isFinished()) {
        overview.progress = overview.progressAnimation->to();
        overview.progressAnimation.reset();
        if (!overviewOpen) {
            overview.selected.reset();
            overview.dragWindow.reset();
            overview.reorderFrom = -1;
            overview.reorderOutput.clear();
            overview.cameraYAnimations.clear();
        }
        return;
    }
    overview.progress = std::clamp(overview.progressAnimation->valueWithoutOvershoot(), 0.0, 1.0);
}

void Engine::Private::seedOverviewCamera()
{
    overview.cameras.clear();
    overview.cameraYAnimations.clear();
    const Workspace *workspace = activeWorkspace();
    overview.selected = focused ? focused : (workspace ? workspace->activeWindow() : std::nullopt);
}

QList<OverviewView> Engine::Private::overviewViews() const
{
    QList<OverviewView> views;
    if (!config.overview.enabled || overview.progress <= 0.0) {
        return views;
    }

    QHash<WorkspaceId, QList<WindowState>> byWorkspace;
    std::optional<WindowState> draggedState;
    for (const WindowState &state : windowStates()) {
        if (overview.dragWindow && state.id == *overview.dragWindow) {
            draggedState = state;
            continue;
        }
        byWorkspace[state.workspace].append(state);
    }

    for (const Monitor &monitor : monitors) {
        views.append(overviewView(monitor, byWorkspace, draggedState));
    }
    return views;
}

std::optional<OverviewView> Engine::Private::overviewView(const QString &output) const
{
    if (!config.overview.enabled || overview.progress <= 0.0) {
        return std::nullopt;
    }
    const auto monitorIt = std::ranges::find(monitors, output, &Monitor::outputName);
    if (monitorIt == monitors.end()) {
        return std::nullopt;
    }

    QHash<WorkspaceId, QList<WindowState>> byWorkspace;
    std::optional<WindowState> draggedState;
    for (const WindowState &state : windowStates()) {
        if (overview.dragWindow && state.id == *overview.dragWindow) {
            draggedState = state;
            continue;
        }
        if (state.output == output) {
            byWorkspace[state.workspace].append(state);
        }
    }
    return overviewView(*monitorIt, byWorkspace, draggedState);
}

std::vector<std::size_t> visibleOverviewWorkspaces(const Monitor &monitor, bool dragging)
{
    std::vector<std::size_t> indices;
    const std::size_t count = monitor.workspaces().size();
    if (count == 0) {
        return indices;
    }
    const bool dynamic = monitor.options()->layout.fixedWorkspaceCount == 0;
    const bool emptyAbove = monitor.options()->layout.emptyWorkspaceAboveFirst;
    const std::size_t active = monitor.activeWorkspaceIndex();

    for (std::size_t idx = 0; idx < count; ++idx) {
        const Workspace &ws = monitor.workspaces()[idx];
        const bool isOccupiedOrNamed = ws.isOccupiedOrNamed();
        if (dynamic && !dragging && !isOccupiedOrNamed && idx != active) {
            if (emptyAbove && idx == 0) {
                continue;
            }
            if (idx == count - 1) {
                continue;
            }
        }
        indices.push_back(idx);
    }
    if (indices.empty()) {
        indices.push_back(active < count ? active : 0);
    }
    return indices;
}

OverviewGeometryInput Engine::Private::overviewGeometryInput(const Monitor &monitor, int activeWorkspace,
    const QHash<WorkspaceId, QList<WindowState>> &byWorkspace, const QPointF &origin, QList<QList<WindowId>> &ids,
    const std::vector<std::size_t> &wsIndices) const
{
    const std::vector<double> offsets = workspaceRenderOffsets(monitor);
    OverviewGeometryInput input;
    input.viewSize = monitor.area().viewSize;
    input.reserved = overview.reserved.value(monitor.outputName());
    input.cardGap = config.overview.cardGap;
    input.minimumScale = config.overview.minimumScale;
    input.activeWorkspace = activeWorkspace;
    for (std::size_t cardIdx = 0; cardIdx < wsIndices.size(); ++cardIdx) {
        const std::size_t idx = wsIndices[cardIdx];
        const Workspace &workspace = monitor.workspaces()[idx];
        const QList<WindowState> states = byWorkspace.value(workspace.id());
        QList<QRectF> frames;
        QList<WindowId> workspaceIds;
        frames.reserve(states.size());
        workspaceIds.reserve(states.size());
        for (const WindowState &state : states) {
            const QRectF frame = state.renderFrame.translated(-origin).translated(0.0, -offsets[idx]);
            frames.append(frame);
            workspaceIds.append(state.id);
            if (overview.selected == state.id && static_cast<int>(cardIdx) == input.activeWorkspace) {
                input.focus = frame.center();
            }
        }
        input.windowFrames.append(frames);
        ids.append(workspaceIds);
    }
    return input;
}

void Engine::Private::appendOverviewWindows(OverviewView &view, const Monitor &monitor, const OverviewGeometry &geometry,
    const QList<QList<WindowId>> &ids, const QHash<WorkspaceId, QList<WindowState>> &byWorkspace, const QPointF &origin,
    const std::vector<std::size_t> &wsIndices) const
{
    for (qsizetype cardIndex = 0; cardIndex < geometry.windows.size(); ++cardIndex) {
        const QList<QRectF> &frames = geometry.windows.at(cardIndex);
        const QList<WindowId> &workspaceIds = ids.at(cardIndex);
        const std::size_t realIdx = wsIndices[cardIndex];
        const QList<WindowState> states = byWorkspace.value(monitor.workspaces()[realIdx].id());
        for (qsizetype windowIndex = 0; windowIndex < frames.size(); ++windowIndex) {
            OverviewWindow window;
            window.id = workspaceIds.at(windowIndex);
            window.workspaceIndex = static_cast<int>(realIdx) + 1;
            window.rect = frames.at(windowIndex).translated(origin - view.camera);
            window.normalRect = windowIndex < states.size() ? std::optional(states.at(windowIndex).renderFrame) : std::nullopt;
            window.selected = overview.selected == window.id;
            window.focused = windowIndex < states.size() && states.at(windowIndex).isFocused;
            view.windows.append(window);
        }
    }
}

void Engine::Private::appendOverviewDragWindow(OverviewView &view, const WindowState &draggedState, double scale) const
{
    const QSizeF size = draggedState.targetFrame.size() * scale;
    OverviewWindow window;
    window.id = draggedState.id;
    window.workspaceIndex = view.dropWorkspace >= 0 ? view.dropWorkspace + 1 : draggedState.workspaceIndex;
    window.rect = QRectF(
        overview.dragPos - QPointF(overview.dragPointerRatio.x() * size.width(), overview.dragPointerRatio.y() * size.height()), size);
    window.normalRect = draggedState.renderFrame;
    window.selected = true;
    window.focused = draggedState.isFocused;
    view.draggedWindow = window.id;
    view.windows.append(window);
}

OverviewView Engine::Private::overviewView(
    const Monitor &monitor, const QHash<WorkspaceId, QList<WindowState>> &byWorkspace, const std::optional<WindowState> &draggedState) const
{
    OverviewView view;
    view.output = monitor.outputName();
    const OutputInfo info = outputInfos.value(view.output);
    view.outputGeometry = info.geometry.isValid() ? info.geometry : QRectF(QPointF(), monitor.area().viewSize);
    view.reserved = overview.reserved.value(view.output);
    const QPointF origin = view.outputGeometry.topLeft();

    const bool isDraggingWindow = overview.dragWindow.has_value();
    const std::vector<std::size_t> wsIndices = visibleOverviewWorkspaces(monitor, isDraggingWindow);
    for (const std::size_t idx : wsIndices) {
        view.workspaceIndices.append(static_cast<int>(idx));
    }
    const auto activeIt = std::find(wsIndices.begin(), wsIndices.end(), monitor.activeWorkspaceIndex());
    view.activeWorkspace = activeIt != wsIndices.end() ? static_cast<int>(std::distance(wsIndices.begin(), activeIt)) : 0;

    QList<QList<WindowId>> ids;
    const OverviewGeometryInput input = overviewGeometryInput(monitor, view.activeWorkspace, byWorkspace, origin, ids, wsIndices);
    const OverviewGeometry geometry = computeOverviewGeometry(input);
    view.scale = geometry.scale;
    view.contentBounds = geometry.contentBounds;
    const auto camIt = overview.cameras.constFind(view.output);
    if (camIt != overview.cameras.constEnd()) {
        const QRectF bounds = geometry.contentBounds.united(QRectF(geometry.camera, input.viewSize));
        view.camera = clampOverviewCamera(bounds, input.viewSize, *camIt);
    } else {
        view.camera = geometry.camera;
    }
    const auto animIt = overview.cameraYAnimations.constFind(view.output);
    if (animIt != overview.cameraYAnimations.constEnd()) {
        view.camera.setY(animIt->valueWithoutOvershoot());
    }
    view.offscreen = overviewOffscreenSides(geometry.contentBounds, overviewViewport(view.camera, input.viewSize));
    for (const QRectF &card : std::as_const(geometry.cards)) {
        view.workspaceRects.append(card.translated(origin - view.camera));
    }
    appendOverviewWindows(view, monitor, geometry, ids, byWorkspace, origin, wsIndices);

    if (overview.drop && overview.dragOutput == view.output) {
        if (const auto index = monitor.indexOfWorkspace(overview.drop->workspace)) {
            const auto it = std::find(wsIndices.begin(), wsIndices.end(), *index);
            if (it != wsIndices.end()) {
                view.dropWorkspace = static_cast<int>(std::distance(wsIndices.begin(), it));
            }
        }
    }
    if (overview.reorderOutput == view.output && overview.reorderFrom >= 0) {
        view.draggedWorkspace = overview.reorderFrom;
        view.dropWorkspace = overview.reorderTo >= 0 ? overview.reorderTo : overview.reorderFrom;
    }
    if (overview.drop && overview.dragOutput == view.output && view.dropWorkspace >= 0
        && view.dropWorkspace < view.workspaceRects.size()) {
        if (monitor.dropHint && monitor.dropHint->workspace.existing) {
            if (const auto idx = monitor.indexOfWorkspace(monitor.dropHint->workspace.id)) {
                if (const auto slot = monitor.workspaces()[*idx].dropSlotRect(monitor.dropHint->position)) {
                    const QRectF cardRect = view.workspaceRects.at(view.dropWorkspace);
                    view.dropHintRect = QRectF(cardRect.topLeft() + slot->topLeft() * geometry.scale, slot->size() * geometry.scale);
                }
            }
        }
    }
    if (draggedState && overview.dragOutput == view.output) {
        appendOverviewDragWindow(view, *draggedState, geometry.scale);
    }
    return view;
}

}
