#include "layout/engine/engineprivate.h"

#include <chrono>

namespace Konveyor::Layout
{

namespace
{

Anim::Duration timestampOf(qint64 milliseconds)
{
    return std::chrono::duration_cast<Anim::Duration>(std::chrono::milliseconds(milliseconds));
}

}

void Engine::beginSwipe(const QString &output, bool isTouchpad)
{
    Monitor *monitor = d->monitorByName(output);
    if (!monitor) {
        return;
    }
    d->viewGestureWorkspace = monitor->activeWorkspace().id();
    monitor->activeWorkspace().beginSwipe(isTouchpad);
}

void Engine::updateSwipe(double delta, qint64 timestampMs, bool isTouchpad)
{
    if (Workspace *workspace = d->viewGestureWorkspace ? d->workspaceById(*d->viewGestureWorkspace) : nullptr) {
        workspace->updateSwipe(delta, timestampOf(timestampMs), isTouchpad);
        if (d->builtinOverviewOpen()) {
            for (const Monitor &monitor : d->monitors) {
                for (const Workspace &ws : monitor.workspaces()) {
                    if (ws.id() == workspace->id()) {
                        d->overview.cameras.remove(monitor.outputName());
                        if (const std::optional<OverviewView> view = d->overviewView(monitor.outputName())) {
                            d->overview.selected = d->centeredOverviewWindow(*view);
                        }
                        break;
                    }
                }
            }
        }
    }
}

void Engine::endSwipe(std::optional<bool> isTouchpad, std::optional<WindowId> keepActive)
{
    if (Workspace *workspace = d->viewGestureWorkspace ? d->workspaceById(*d->viewGestureWorkspace) : nullptr) {
        workspace->endSwipe(isTouchpad, keepActive);
        if (d->builtinOverviewOpen()) {
            d->overview.selected = workspace->activeWindow();
        }
    }
    d->viewGestureWorkspace.reset();
    d->refresh();
}

void Engine::beginWorkspaceSwipe(const QString &output, bool isTouchpad)
{
    Monitor *monitor = d->monitorByName(output);
    if (!monitor) {
        return;
    }
    d->switchGestureOutput = output;
    if (d->builtinOverviewOpen()) {
        d->overview.verticalScrollAccum = 0.0;
        return;
    }
    monitor->beginWorkspaceSwipe(isTouchpad, d->usesWorkspaceMotion(*monitor));
}

void Engine::updateWorkspaceSwipe(double delta, qint64 timestampMs, bool isTouchpad)
{
    Monitor *monitor = d->monitorByName(d->switchGestureOutput);
    if (!monitor) {
        return;
    }
    if (d->builtinOverviewOpen()) {
        constexpr double kSwipeStep = 80.0;
        d->overview.verticalScrollAccum += delta;
        while (std::abs(d->overview.verticalScrollAccum) >= kSwipeStep) {
            const int step = d->overview.verticalScrollAccum > 0.0 ? 1 : -1;
            d->overview.verticalScrollAccum -= step * kSwipeStep;
            overviewNavigate(step > 0 ? OverviewDirection::Down : OverviewDirection::Up);
        }
        return;
    }
    monitor->updateWorkspaceSwipe(delta, timestampOf(timestampMs), isTouchpad);
}

void Engine::endWorkspaceSwipe(std::optional<bool> isTouchpad)
{
    if (Monitor *monitor = d->monitorByName(d->switchGestureOutput)) {
        if (d->builtinOverviewOpen()) {
            d->overview.verticalScrollAccum = 0.0;
        } else {
            monitor->endWorkspaceSwipe(isTouchpad);
        }
    }
    d->switchGestureOutput.clear();
    d->refresh();
}

void Engine::beginDataDrag()
{
    d->dataDragPointer.reset();
    for (Monitor &monitor : d->monitors) {
        monitor.beginEdgeScroll();
        for (Workspace &workspace : monitor.workspaces()) {
            workspace.beginEdgeScroll();
        }
    }
}

void Engine::Private::stopEdgeScroll()
{
    dataDragPointer.reset();
    edgeScrolling = false;
    for (Monitor &monitor : monitors) {
        monitor.endEdgeScroll();
        monitor.dropHint.reset();
        for (Workspace &workspace : monitor.workspaces()) {
            workspace.endEdgeScroll();
        }
    }
}

void Engine::Private::edgeScrollAt(Monitor &monitor, QPointF local)
{
    if (builtinOverviewOpen()) {
        edgeScrolling = false;
        if (!overview.dragWindow || overview.dragOutput != monitor.outputName()) {
            return;
        }
        const std::optional<OverviewView> view = overviewView(monitor.outputName());
        if (!view) {
            return;
        }
        const QRectF workArea = view->outputGeometry.adjusted(
            view->reserved.left(), view->reserved.top(), -view->reserved.right(), -view->reserved.bottom());
        const double edgeX = std::clamp(workArea.width() * 0.08, 72.0, 140.0);
        const double edgeY = std::clamp(workArea.height() * 0.08, 60.0, 120.0);
        const Anim::Duration now = clock.rawNow();

        double verticalProximity = 0.0;
        int stepY = 0;
        if (overview.dragPos.y() < workArea.top() + edgeY && view->offscreen.contains(OverviewSide::Top)) {
            stepY = -1;
            verticalProximity = std::clamp((workArea.top() + edgeY - overview.dragPos.y()) / edgeY, 0.0, 1.0);
        } else if (overview.dragPos.y() > workArea.bottom() - edgeY && view->offscreen.contains(OverviewSide::Bottom)) {
            stepY = 1;
            verticalProximity = std::clamp((overview.dragPos.y() - (workArea.bottom() - edgeY)) / edgeY, 0.0, 1.0);
        }
        const std::size_t wsCount = monitor.workspaces().size();
        const std::size_t activeWs = monitor.activeWorkspaceIndex();
        const std::size_t remainingWorkspaces
            = stepY < 0 ? activeWs : (stepY > 0 && activeWs + 1 < wsCount ? wsCount - 1 - activeWs : 0);
        if (stepY == 0 || remainingWorkspaces == 0) {
            overview.lastVerticalEdgeStep.reset();
        } else {
            const double offscreenBoost = std::min(1.0, static_cast<double>(remainingWorkspaces - 1) / 3.0);
            const double speedFactor = std::clamp(0.65 * verticalProximity + 0.35 * verticalProximity * offscreenBoost, 0.0, 1.0);
            const auto interval = std::chrono::milliseconds(static_cast<int>(std::lerp(480.0, 180.0, speedFactor)));
            const bool ready = !overview.lastVerticalEdgeStep || (now - *overview.lastVerticalEdgeStep >= interval);
            if (!overview.cameraYAnimations.contains(view->output) && ready) {
                const double beforeCameraY = view->camera.y();
                const std::size_t beforeIdx = monitor.activeWorkspaceIndex();
                if (stepY < 0) {
                    monitor.switchWorkspaceUp();
                } else {
                    monitor.switchWorkspaceDown();
                }
                if (monitor.activeWorkspaceIndex() != beforeIdx) {
                    overview.lastVerticalEdgeStep = now;
                    overview.cameras.remove(view->output);
                    if (config.animations.enabled) {
                        if (const std::optional<OverviewView> afterView = overviewView(view->output)) {
                            const double afterCameraY = afterView->camera.y();
                            if (!qFuzzyCompare(beforeCameraY, afterCameraY)) {
                                overview.cameraYAnimations.insert(view->output,
                                    Anim::Animation(clock, beforeCameraY, afterCameraY, 0.0, config.animations.workspaceSwitch));
                            }
                        }
                    }
                    updateOverviewDrop();
                }
            }
            edgeScrolling = true;
        }

        double horizontalProximity = 0.0;
        int stepX = 0;
        if (overview.dragPos.x() < workArea.left() + edgeX) {
            stepX = -1;
            horizontalProximity = std::clamp((workArea.left() + edgeX - overview.dragPos.x()) / edgeX, 0.0, 1.0);
        } else if (overview.dragPos.x() > workArea.right() - edgeX) {
            stepX = 1;
            horizontalProximity = std::clamp((overview.dragPos.x() - (workArea.right() - edgeX)) / edgeX, 0.0, 1.0);
        }
        Workspace *targetWs = overview.drop ? workspaceById(overview.drop->workspace) : nullptr;
        Workspace &ws = targetWs ? *targetWs : monitor.activeWorkspace();
        const std::size_t colCount = ws.scrolling().columns().size();
        const std::size_t activeCol = ws.scrolling().activeColumnIndex();
        const std::size_t remainingCols = stepX < 0 ? activeCol : (stepX > 0 && activeCol + 1 < colCount ? colCount - 1 - activeCol : 0);
        if (stepX == 0 || remainingCols == 0) {
            overview.lastHorizontalEdgeStep.reset();
        } else {
            const double offscreenBoost = std::min(1.0, static_cast<double>(remainingCols - 1) / 4.0);
            const double speedFactor = std::clamp(0.65 * horizontalProximity + 0.35 * horizontalProximity * offscreenBoost, 0.0, 1.0);
            const auto interval = std::chrono::milliseconds(static_cast<int>(std::lerp(380.0, 110.0, speedFactor)));
            const bool ready = !overview.lastHorizontalEdgeStep || (now - *overview.lastHorizontalEdgeStep >= interval);
            if (ready) {
                const bool moved = stepX < 0 ? ws.focusLeft(config.layout.wrapColumns) : ws.focusRight(config.layout.wrapColumns);
                if (moved) {
                    overview.lastHorizontalEdgeStep = now;
                    updateOverviewDrop();
                }
            }
            edgeScrolling = true;
        }
        return;
    }
    edgeScrolling = monitor.edgeScrollBy(local, 1.0);
    for (Workspace &workspace : monitor.workspaces()) {
        edgeScrolling = workspace.edgeScrollBy(local, 1.0) || edgeScrolling;
    }
}

void Engine::Private::continueEdgeScroll()
{
    if (windowDrag) {
        scrollDragEdges();
        updateDropHint();
    } else if (Monitor *monitor = dataDragPointer ? monitorByName(dataDragPointer->first) : nullptr) {
        edgeScrollAt(*monitor, dataDragPointer->second);
    }
}

void Engine::endDataDrag()
{
    d->stopEdgeScroll();
    d->refresh();
}

void Engine::dataDragEdgeScroll(const QString &output, const QPointF &pointer, qint64 timestampMs)
{
    Q_UNUSED(timestampMs)
    if (Monitor *monitor = d->monitorByName(output)) {
        d->dataDragPointer = {output, pointer - d->originOf(monitor->outputName())};
        d->edgeScrollAt(*monitor, d->dataDragPointer->second);
    }
}

}
