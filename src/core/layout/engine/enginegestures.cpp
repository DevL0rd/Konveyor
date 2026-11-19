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

struct EdgeHit
{
    int step = 0;
    double proximity = 0.0;
};

// Where the pointer sits relative to the scroll edges of [begin, end]; the low
// edge wins when the region is narrower than two edges.
EdgeHit edgeHitAlong(double pos, double begin, double end, double size, bool canStepLow, bool canStepHigh)
{
    if (canStepLow && pos < begin + size) {
        return {-1, std::clamp((begin + size - pos) / size, 0.0, 1.0)};
    }
    if (canStepHigh && pos > end - size) {
        return {1, std::clamp((pos - (end - size)) / size, 0.0, 1.0)};
    }
    return {};
}

struct EdgeScrollClock
{
    const std::optional<Anim::Duration> &last;
    Anim::Duration now;
    double slowMs = 480.0;
    double fastMs = 180.0;
    double boostDivisor = 3.0;
};

// Repeating edge steps speed up as the pointer digs in and as more cards are
// left off screen.
bool edgeStepDue(const EdgeScrollClock &clock, const EdgeHit &hit, std::size_t remaining)
{
    const double boost = std::min(1.0, static_cast<double>(remaining - 1) / clock.boostDivisor);
    const double factor = std::clamp(0.65 * hit.proximity + 0.35 * hit.proximity * boost, 0.0, 1.0);
    const auto interval = std::chrono::milliseconds(static_cast<int>(std::lerp(clock.slowMs, clock.fastMs, factor)));
    return !clock.last || clock.now - *clock.last >= interval;
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

void Engine::Private::overviewAfterWorkspaceSwipe(WorkspaceId id)
{
    if (!builtinOverviewOpen()) {
        return;
    }
    for (const Monitor &monitor : monitors) {
        const auto &workspaces = monitor.workspaces();
        if (std::ranges::none_of(workspaces, [id](const Workspace &workspace) { return workspace.id() == id; })) {
            continue;
        }
        overview.cameras.remove(monitor.outputName());
        if (const std::optional<OverviewView> view = overviewView(monitor.outputName())) {
            overview.selected = centeredOverviewWindow(*view);
        }
        return;
    }
}

void Engine::updateSwipe(double delta, qint64 timestampMs, bool isTouchpad)
{
    Workspace *workspace = d->viewGestureWorkspace ? d->workspaceById(*d->viewGestureWorkspace) : nullptr;
    if (!workspace) {
        return;
    }
    workspace->updateSwipe(delta, timestampOf(timestampMs), isTouchpad);
    d->overviewAfterWorkspaceSwipe(workspace->id());
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
    if (!builtinOverviewOpen()) {
        edgeScrolling = monitor.edgeScrollBy(local, 1.0);
        for (Workspace &workspace : monitor.workspaces()) {
            edgeScrolling = workspace.edgeScrollBy(local, 1.0) || edgeScrolling;
        }
        return;
    }
    edgeScrolling = false;
    if (!overview.dragWindow || overview.dragOutput != monitor.outputName()) {
        return;
    }
    const std::optional<OverviewView> view = overviewView(monitor.outputName());
    if (!view) {
        return;
    }
    const QRectF workArea
        = view->outputGeometry.adjusted(view->reserved.left(), view->reserved.top(), -view->reserved.right(), -view->reserved.bottom());
    const double edgeX = std::clamp(workArea.width() * 0.08, 72.0, 140.0);
    const double edgeY = std::clamp(workArea.height() * 0.08, 60.0, 120.0);
    const Anim::Duration now = clock.rawNow();
    const bool vertical = scrollOverviewWorkspacesAtEdge(monitor, *view, workArea, edgeY, now);
    const bool horizontal = scrollOverviewColumnsAtEdge(monitor, workArea, edgeX, now);
    edgeScrolling = vertical || horizontal;
}

bool Engine::Private::scrollOverviewWorkspacesAtEdge(
    Monitor &monitor, const OverviewView &view, const QRectF &workArea, double edge, Anim::Duration now)
{
    const std::size_t count = monitor.workspaces().size();
    const std::size_t active = monitor.activeWorkspaceIndex();
    const EdgeHit hit = edgeHitAlong(overview.dragPos.y(), workArea.top(), workArea.bottom(), edge, active > 0, active + 1 < count);
    if (hit.step == 0) {
        overview.lastVerticalEdgeStep.reset();
        return false;
    }
    const std::size_t remaining = hit.step < 0 ? active : count - 1 - active;
    if (edgeStepDue({overview.lastVerticalEdgeStep, now, 480.0, 180.0, 3.0}, hit, remaining)
        && switchOverviewWorkspace(monitor, view, hit.step)) {
        overview.lastVerticalEdgeStep = now;
    }
    return true;
}

bool Engine::Private::scrollOverviewColumnsAtEdge(Monitor &monitor, const QRectF &workArea, double edge, Anim::Duration now)
{
    const EdgeHit hit = edgeHitAlong(overview.dragPos.x(), workArea.left(), workArea.right(), edge, true, true);
    if (hit.step == 0) {
        overview.lastHorizontalEdgeStep.reset();
        return false;
    }
    Workspace *target = overview.drop ? workspaceById(overview.drop->workspace) : nullptr;
    Workspace &workspace = target ? *target : monitor.activeWorkspace();
    const std::size_t count = workspace.scrolling().columns().size();
    const std::size_t active = workspace.scrolling().activeColumnIndex();
    const std::size_t remaining = hit.step < 0 ? active : (hit.step > 0 && active + 1 < count ? count - 1 - active : 0);
    if (remaining == 0) {
        overview.lastHorizontalEdgeStep.reset();
        return false;
    }
    if (edgeStepDue({overview.lastHorizontalEdgeStep, now, 380.0, 110.0, 4.0}, hit, remaining)) {
        const bool moved = hit.step < 0 ? workspace.focusLeft(config.layout.wrapColumns) : workspace.focusRight(config.layout.wrapColumns);
        if (moved) {
            overview.lastHorizontalEdgeStep = now;
            updateOverviewDrop();
        }
    }
    return true;
}

bool Engine::Private::switchOverviewWorkspace(Monitor &monitor, const OverviewView &view, int step)
{
    const double beforeCameraY = view.camera.y();
    const std::size_t beforeIndex = monitor.activeWorkspaceIndex();
    if (step < 0) {
        monitor.switchWorkspaceUp();
    } else {
        monitor.switchWorkspaceDown();
    }
    if (monitor.activeWorkspaceIndex() == beforeIndex) {
        return false;
    }
    overview.cameras.remove(view.output);
    restartOverviewCameraY(view.output, overviewView(view.output), beforeCameraY);
    updateOverviewDrop();
    return true;
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
