#include "layout/engine/engineprivate.h"

#include <algorithm>

namespace Konveyor::Layout
{

namespace
{

// Index of the card that is treated as active; the active workspace can be hidden.
int activeCardIndex(const Monitor &monitor, const std::vector<std::size_t> &wsIndices)
{
    const auto activeIt = std::find(wsIndices.begin(), wsIndices.end(), monitor.activeWorkspaceIndex());
    if (activeIt != wsIndices.end()) {
        return static_cast<int>(std::distance(wsIndices.begin(), activeIt));
    }
    if (wsIndices.empty()) {
        return 0;
    }
    // The active workspace is hidden (an idle empty edge workspace); keep the
    // nearest visible card active so the camera and selection stay put.
    return monitor.activeWorkspaceIndex() < wsIndices.front() ? 0 : static_cast<int>(wsIndices.size()) - 1;
}

int cardIndexFor(const std::vector<std::size_t> &wsIndices, std::size_t workspaceIndex)
{
    const auto it = std::find(wsIndices.begin(), wsIndices.end(), workspaceIndex);
    return it == wsIndices.end() ? -1 : static_cast<int>(std::distance(wsIndices.begin(), it));
}

QList<int> cardIndices(const std::vector<std::size_t> &wsIndices)
{
    QList<int> indices;
    indices.reserve(wsIndices.size());
    for (const std::size_t index : wsIndices) {
        indices.append(static_cast<int>(index));
    }
    return indices;
}

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
        if (dynamic && !dragging && !isOccupiedOrNamed) {
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
    const QList<QList<WindowId>> &ids, const QHash<WorkspaceId, QList<WindowState>> &byWorkspace, const QPointF &offset) const
{
    for (qsizetype cardIndex = 0; cardIndex < geometry.windows.size(); ++cardIndex) {
        const auto realIdx = static_cast<std::size_t>(view.workspaceIndices.at(cardIndex));
        const QList<QRectF> &frames = geometry.windows.at(cardIndex);
        const QList<WindowId> &workspaceIds = ids.at(cardIndex);
        const QList<WindowState> states = byWorkspace.value(monitor.workspaces()[realIdx].id());
        for (qsizetype windowIndex = 0; windowIndex < frames.size(); ++windowIndex) {
            OverviewWindow window;
            window.id = workspaceIds.at(windowIndex);
            window.workspaceIndex = static_cast<int>(realIdx) + 1;
            window.rect = frames.at(windowIndex).translated(offset);
            window.normalRect = windowIndex < states.size() ? std::optional(states.at(windowIndex).renderFrame) : std::nullopt;
            window.selected = overview.selected == window.id;
            window.focused = windowIndex < states.size() && states.at(windowIndex).isFocused;
            view.windows.append(window);
        }
    }
}

void Engine::Private::applyOverviewCamera(OverviewView &view, const OverviewGeometryInput &input, const OverviewGeometry &geometry) const
{
    // A running pan moves between two camera positions that were valid when it
    // started. Re-clamping in between would snap the board, because activating a
    // window can change the active workspace and with it the clamp bounds.
    const auto panIt = overview.cameraPans.constFind(view.output);
    if (panIt != overview.cameraPans.constEnd()) {
        view.camera = panIt->position();
        return;
    }
    const auto camIt = overview.cameras.constFind(view.output);
    if (camIt == overview.cameras.constEnd()) {
        view.camera = geometry.camera;
    } else {
        const QRectF bounds = geometry.contentBounds.united(QRectF(geometry.camera, input.viewSize));
        view.camera = clampOverviewCamera(bounds, input.viewSize, *camIt);
    }
    const auto animIt = overview.cameraYAnimations.constFind(view.output);
    if (animIt != overview.cameraYAnimations.constEnd()) {
        view.camera.setY(animIt->valueWithoutOvershoot());
    }
}

void Engine::Private::applyOverviewDropState(
    OverviewView &view, const Monitor &monitor, const OverviewGeometry &geometry, const std::vector<std::size_t> &wsIndices) const
{
    if (overview.drop && overview.dragOutput == view.output) {
        if (const auto index = monitor.indexOfWorkspace(overview.drop->workspace)) {
            view.dropWorkspace = cardIndexFor(wsIndices, *index);
        }
    }
    if (overview.reorderOutput == view.output && overview.reorderFrom >= 0) {
        view.draggedWorkspace = overview.reorderFrom;
        view.dropWorkspace = overview.reorderTo >= 0 ? overview.reorderTo : overview.reorderFrom;
    }
    if (overview.drop && overview.dragOutput == view.output) {
        view.dropHintRect = overviewDropHintRect(monitor, view.workspaceRects, view.dropWorkspace, geometry.scale);
    }
}

std::optional<QRectF> Engine::Private::overviewDropHintRect(
    const Monitor &monitor, const QList<QRectF> &cards, int dropWorkspace, double scale) const
{
    if (dropWorkspace < 0 || dropWorkspace >= cards.size()) {
        return std::nullopt;
    }
    if (!monitor.dropHint || !monitor.dropHint->workspace.existing) {
        return std::nullopt;
    }
    const auto index = monitor.indexOfWorkspace(monitor.dropHint->workspace.id);
    if (!index) {
        return std::nullopt;
    }
    const auto slot = monitor.workspaces()[*index].dropSlotRect(monitor.dropHint->position);
    if (!slot) {
        return std::nullopt;
    }
    const QRectF cardRect = cards.at(dropWorkspace);
    return QRectF(cardRect.topLeft() + slot->topLeft() * scale, slot->size() * scale);
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

    const std::vector<std::size_t> wsIndices = visibleOverviewWorkspaces(monitor, overview.dragWindow.has_value());
    view.workspaceIndices = cardIndices(wsIndices);
    view.activeWorkspace = activeCardIndex(monitor, wsIndices);

    QList<QList<WindowId>> ids;
    const OverviewGeometryInput input = overviewGeometryInput(monitor, view.activeWorkspace, byWorkspace, origin, ids, wsIndices);
    const OverviewGeometry geometry = computeOverviewGeometry(input);
    view.scale = geometry.scale;
    view.contentBounds = geometry.contentBounds;
    applyOverviewCamera(view, input, geometry);
    view.offscreen = overviewOffscreenSides(geometry.contentBounds, overviewViewport(view.camera, input.viewSize));
    for (const QRectF &card : std::as_const(geometry.cards)) {
        view.workspaceRects.append(card.translated(origin - view.camera));
    }
    appendOverviewWindows(view, monitor, geometry, ids, byWorkspace, origin - view.camera);
    applyOverviewDropState(view, monitor, geometry, wsIndices);
    if (draggedState && overview.dragOutput == view.output) {
        appendOverviewDragWindow(view, *draggedState, geometry.scale);
    }
    return view;
}

}
