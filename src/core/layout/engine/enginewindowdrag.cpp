#include "layout/engine/engineprivate.h"

#include "layout/common/geometry.h"

#include <algorithm>
#include <utility>

namespace Konveyor::Layout
{

namespace
{

constexpr double DragDetachDistanceSq = 256.0 * 256.0;
constexpr double DraggedWindowOpacity = 0.75;
constexpr ElasticLimit DragDetachResistance {1.0, 0.5};

}

QPointF WindowDrag::renderLocation() const
{
    if (!tile) {
        return pointerPos;
    }
    const QSizeF size = tile->windowSize();
    const QPointF offset(pointerRatio.x() * size.width(), pointerRatio.y() * size.height());
    return pointerPos - offset - tile->windowOffset();
}

bool Engine::beginWindowDrag(WindowId id, const QPointF &pointer)
{
    if (d->windowDrag) {
        return false;
    }
    Monitor *monitor = d->monitorOf(id);
    Workspace *workspace = d->workspaceOf(id);
    if (!monitor || !workspace) {
        return false;
    }
    const Tile *tile = workspace->tileFor(id);
    const auto tilePos = workspace->tileRenderPosition(id);
    const std::optional<RestorePlacement> origin = placementOf(id);
    if (!tile || !tilePos || !origin) {
        return false;
    }

    WindowDrag move;
    move.window = id;
    move.origin = {*origin, monitor->outputName(), *monitor->indexOfWorkspace(workspace->id()), tile->window().requestedMode()};
    move.output = monitor->outputName();
    move.pointerPos = pointer - d->originOf(move.output);
    move.lastPointer = pointer;
    move.isFloating = workspace->isFloating(id);
    const QPointF within = move.pointerPos - *tilePos - tile->windowOffset();
    const QSizeF size = tile->windowSize();
    move.pointerRatio = QPointF(
        std::clamp(within.x() / std::max(size.width(), 1.0), 0.0, 1.0), std::clamp(within.y() / std::max(size.height(), 1.0), 0.0, 1.0));
    d->windowDrag = move;

    for (Monitor &each : d->monitors) {
        each.beginEdgeScroll();
        if (!move.isFloating) {
            for (Workspace &ws : each.workspaces()) {
                ws.beginEdgeScroll();
            }
        }
    }
    return true;
}

void Engine::Private::beginInteractiveMoving(const QString &output)
{
    WindowDrag &move = *windowDrag;
    Workspace *workspace = workspaceOf(move.window);
    if (!workspace) {
        windowDrag.reset();
        return;
    }
    workspace->setFullscreen(move.window, false);
    workspace->setMaximized(move.window, false);
    workspace = workspaceOf(move.window);
    if (!workspace) {
        windowDrag.reset();
        return;
    }

    DetachedTile removed = workspace->removeTile(move.window);
    removed.tile.stopSlides();
    removed.tile.dragOffset = QPointF();
    move.width = removed.width;
    move.fillsWidth = removed.fillsWidth;
    move.isFloating = removed.isFloating;
    if (!move.isFloating) {
        removed.tile.fadeOpacity(1.0, DraggedWindowOpacity, options->animations.windowMovement);
        removed.tile.keepFadeAfterFinish();
    }
    move.tile = std::move(removed.tile);
    move.moving = true;
    move.output = output;
    for (Monitor &monitor : monitors) {
        monitor.pruneWorkspaces();
    }
}

void Engine::Private::updateDropHint()
{
    for (Monitor &monitor : monitors) {
        monitor.dropHint.reset();
    }
    if (!windowDrag || !windowDrag->moving) {
        return;
    }
    Monitor *monitor = monitorByName(windowDrag->output);
    if (!monitor) {
        return;
    }
    const QPointF local = windowDrag->pointerPos;
    const Monitor::InsertTarget insertTarget = monitor->insertTargetAt(local);
    DropHint hint;
    hint.workspace = insertTarget.workspace;
    if (windowDrag->isFloating) {
        hint.position = {DropSlot::Kind::Floating, 0, 0};
    } else if (insertTarget.workspace.existing) {
        const auto idx = monitor->indexOfWorkspace(insertTarget.workspace.id);
        hint.position = idx ? monitor->workspaces()[*idx].tiledDropSlotAt(
                                  local - QPointF(0.0, insertTarget.workspaceY), windowDrag->tile->window().rules().columnPosition)
                            : DropSlot();
    }
    monitor->dropHint = hint;
}

void Engine::updateWindowDrag(const QPointF &pointer, const QString &output)
{
    if (!d->windowDrag) {
        return;
    }
    WindowDrag &move = *d->windowDrag;
    const QPointF delta = pointer - move.lastPointer;
    move.lastPointer = pointer;

    if (!move.moving) {
        move.pointerDelta += delta;
        const double squared = QPointF::dotProduct(move.pointerDelta, move.pointerDelta);
        const double factor = DragDetachResistance.band(squared / DragDetachDistanceSq);
        if (Workspace *workspace = d->workspaceOf(move.window)) {
            if (Tile *tile = workspace->tileFor(move.window)) {
                tile->dragOffset = move.pointerDelta * factor;
            }
        }
        if (move.isFloating || squared >= DragDetachDistanceSq) {
            d->beginInteractiveMoving(output);
        }
    } else if (move.output != output && d->monitorByName(output)) {
        move.output = output;
        if (const auto idx = d->monitorIndexByName(output)) {
            d->activeMonitorIndex = *idx;
        }
    }
    move.pointerPos = pointer - d->originOf(move.output);
    d->scrollDragEdges();
    d->updateDropHint();
}

void Engine::Private::scrollDragEdges()
{
    if (!windowDrag->moving || windowDrag->isFloating) {
        return;
    }
    if (Monitor *monitor = monitorByName(windowDrag->output)) {
        edgeScrollAt(*monitor, windowDrag->pointerPos);
    }
}

void Engine::Private::moveDragToActiveOutput()
{
    const Monitor *monitor = activeMonitor();
    const QSizeF size = monitor->area().viewSize;
    windowDrag->output = monitor->outputName();
    windowDrag->pointerPos
        = QPointF(std::clamp(windowDrag->pointerPos.x(), 0.0, size.width()), std::clamp(windowDrag->pointerPos.y(), 0.0, size.height()));
    updateDropHint();
}

void Engine::toggleWindowDragFloating()
{
    if (!d->windowDrag || !d->windowDrag->moving || !d->windowDrag->tile) {
        return;
    }
    WindowDrag &move = *d->windowDrag;
    move.isFloating = !move.isFloating;
    if (move.isFloating) {
        move.tile->fadeOpacity(DraggedWindowOpacity, 1.0, d->options->animations.windowMovement);
    } else {
        move.tile->fadeOpacity(1.0, DraggedWindowOpacity, d->options->animations.windowMovement);
        move.tile->keepFadeAfterFinish();
    }
    d->updateDropHint();
}

void Engine::Private::dropInteractiveTile(Monitor &monitor, const Monitor::InsertTarget &insertTarget)
{
    WindowDrag &move = *windowDrag;
    std::size_t workspaceIndex = 0;
    if (insertTarget.workspace.existing) {
        workspaceIndex = monitor.indexOfWorkspace(insertTarget.workspace.id).value_or(monitor.activeWorkspaceIndex());
    } else {
        const std::size_t requested = insertTarget.workspace.newAt;
        if (monitor.options()->layout.emptyWorkspaceAboveFirst && requested == 0) {
            workspaceIndex = 0;
        } else if (monitor.workspaces().size() - 1 <= requested) {
            workspaceIndex = monitor.workspaces().size() - 1;
        } else {
            monitor.insertEmptyWorkspace(requested);
            workspaceIndex = requested;
        }
    }

    const QPointF renderLoc = move.renderLocation();
    Tile tile = std::move(*move.tile);
    tile.fadeOpacity(tile.alpha(), 1.0, options->animations.windowMovement);
    const WorkspaceId workspaceId = monitor.workspaces()[workspaceIndex].id();
    const DropSlot position = move.isFloating
        ? DropSlot {DropSlot::Kind::Floating, 0, 0}
        : monitor.workspaces()[workspaceIndex].tiledDropSlotAt(
              move.pointerPos - QPointF(0.0, insertTarget.workspaceY), tile.window().rules().columnPosition);

    if (position.kind == DropSlot::Kind::InColumn) {
        monitor.insertIntoColumn(workspaceIndex, position.column, position.tile, std::move(tile), true, !overviewOpen);
        return;
    }
    if (position.kind == DropSlot::Kind::Floating) {
        tile.savedFloatingPosition
            = monitor.workspaces()[workspaceIndex].floating().absoluteToRelative(renderLoc - QPointF(0.0, insertTarget.workspaceY));
        if (const auto size = tile.window().pendingSize()) {
            tile.savedFloatingSize = size;
        }
    }
    MonitorAddRequest request;
    request.target = MonitorAddTarget::onWorkspace(
        workspaceId, position.kind == DropSlot::Kind::NewColumn ? std::optional<std::size_t>(position.column) : std::nullopt);
    request.activate = Activation::Always;
    request.allowActivateWorkspace = !overviewOpen;
    request.width = move.width;
    request.fillsWidth = move.fillsWidth;
    request.isFloating = move.isFloating;
    monitor.addTile(std::move(tile), request);
}

void Engine::Private::interactiveMoveFinish()
{
    if (!windowDrag) {
        return;
    }
    stopEdgeScroll();

    WindowDrag move = std::move(*windowDrag);
    windowDrag.reset();
    if (!move.moving || !move.tile) {
        if (Workspace *workspace = workspaceOf(move.window)) {
            if (Tile *tile = workspace->tileFor(move.window)) {
                const QPointF offset = tile->dragOffset;
                tile->dragOffset = QPointF();
                tile->slideFrom(offset);
            }
            workspace->activateWindow(move.window);
        }
        return;
    }

    windowDrag = std::move(move);
    Monitor *monitor = monitorByName(windowDrag->output);
    if (!monitor) {
        monitor = activeMonitor();
    }
    if (monitor) {
        dropInteractiveTile(*monitor, monitor->insertTargetAt(windowDrag->pointerPos));
    }
    windowDrag.reset();
}

void Engine::Private::dropDraggedWindow(std::optional<WindowId> window)
{
    if (windowDrag && window == windowDrag->window) {
        interactiveMoveFinish();
    }
}

void Engine::endWindowDrag()
{
    d->interactiveMoveFinish();
    d->refresh();
}

void Engine::Private::returnDraggedTile(WindowDrag &move)
{
    DragOrigin origin = move.origin;
    Monitor *monitor = monitorByName(origin.output);
    if (!monitor) {
        monitor = activeMonitor();
    }
    if (!monitor) {
        return;
    }
    if (!workspaceById(origin.placement.workspace)) {
        const std::size_t last = monitor->workspaces().size() - 1;
        const std::size_t index = std::min(origin.workspaceIndex, last);
        if (index < last) {
            monitor->insertEmptyWorkspace(index);
        }
        origin.placement.workspace = monitor->workspaces()[index].id();
    }
    Tile tile = std::move(*move.tile);
    tile.fadeOpacity(tile.alpha(), 1.0, options->animations.windowMovement);
    NewWindowPlan plan;
    plan.activate = Activation::Always;
    MonitorAddRequest request;
    request.activate = Activation::Always;
    request.width = move.width;
    request.fillsWidth = move.fillsWidth;
    request.isFloating = origin.placement.isFloating;
    if (placeRestored(tile, plan, origin.placement, request)) {
        return;
    }
    for (Monitor &each : monitors) {
        if (each.indexOfWorkspace(origin.placement.workspace)) {
            each.addTile(std::move(tile), request);
            return;
        }
    }
}

void Engine::cancelWindowDrag()
{
    if (!d->windowDrag || !d->windowDrag->moving || !d->windowDrag->tile) {
        endWindowDrag();
        return;
    }
    d->stopEdgeScroll();
    WindowDrag move = std::move(*d->windowDrag);
    d->windowDrag.reset();
    d->returnDraggedTile(move);
    Workspace *workspace = d->workspaceOf(move.window);
    if (workspace && move.origin.mode == WindowMode::Fullscreen) {
        workspace->setFullscreen(move.window, true);
    } else if (workspace && move.origin.mode == WindowMode::Maximized) {
        workspace->setMaximized(move.window, true);
    }
    d->refresh();
}

}
