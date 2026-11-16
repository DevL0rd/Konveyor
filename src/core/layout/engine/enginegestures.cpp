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
        const double speed = std::clamp(d->config.overview.scrollSpeed, 0.05, 5.0);
        d->overview.verticalScrollAccum += delta * speed;
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
