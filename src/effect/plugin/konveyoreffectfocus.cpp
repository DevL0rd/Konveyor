#include "plugin/konveyoreffect_p.h"

namespace Konveyor
{

namespace
{
bool focusArrivesLater(const KWin::Window *window)
{
#ifdef KONVEYOR_KWIN_ASYNC_FOCUS
    return window->takesAsyncFocus();
#else
    Q_UNUSED(window)
    return false;
#endif
}
}

void KonveyorEffect::handlePointerMotion(const QPointF &position, qint64 timestampMs)
{
    if (d->titlebarDrag) {
        changeEngine().updateSwipe(d->dragOrigin - position.x(), timestampMs, false);
        d->dragOrigin = position.x();
        return;
    }
    const std::optional<QString> output = readEngine().focusedOutput();
    if (output && KWin::waylandServer()->seat()->isDrag()) {
        changeEngine().dataDragEdgeScroll(*output, position, timestampMs);
    }
    if (d->config.config().input.focusFollowsMouse) {
        focusWindowUnderPointer(position);
    }
}

void KonveyorEffect::followActiveWindow()
{
    KWin::Window *active = KWin::workspace()->activeWindow();
    d->followedWindow = active;
    if (const std::optional<Layout::WindowId> id = d->windows.idOf(active)) {
        changeEngine().activateWindow(*id);
        moveActiveOutputHome(*id);
    } else {
        changeEngine().setLayoutFocused(false);
    }
}

void KonveyorEffect::followActiveOutput(const QString &name)
{
    KWin::Window *active = KWin::workspace()->activeWindow();
    if (active != d->followedWindow && d->windows.idOf(active)) {
        return;
    }
    changeEngine().focusOutput(name);
}

void KonveyorEffect::moveActiveOutputHome(Layout::WindowId id)
{
    const std::optional<Layout::WindowState> state = readEngine().windowState(id);
    KWin::LogicalOutput *home = state ? d->outputs.outputNamed(state->output) : nullptr;
    if (home && home != KWin::workspace()->activeOutput()) {
        KWin::workspace()->setActiveOutput(home);
    }
}

void KonveyorEffect::applyFocusRequest()
{
    const std::optional<Layout::WindowId> id = std::exchange(d->focusRequest, std::nullopt);
    KWin::Window *window = id ? d->windows.windowOf(*id) : nullptr;
    if (!window) {
        return;
    }
    if (window != KWin::workspace()->activeWindow()) {
        KWin::workspace()->activateWindow(window);
        if (!focusArrivesLater(window)) {
            followActiveWindow();
        }
    }
    if (window == KWin::workspace()->activeWindow()) {
        warpPointerTo(*id);
    }
}

void KonveyorEffect::focusWindowUnderPointer(const QPointF &position)
{
    const std::optional<Layout::WindowId> id = readEngine().windowAt(position);
    KWin::Window *window = id ? d->windows.windowOf(*id) : nullptr;
    if (window && window != KWin::workspace()->activeWindow()) {
        KWin::workspace()->activateWindow(window);
    }
}

void KonveyorEffect::warpPointerTo(Layout::WindowId id)
{
    const Config::Input &input = d->config.config().input;
    const std::optional<Layout::WindowState> state = readEngine().windowState(id);
    if (!input.warpMouseToFocus || !state) {
        return;
    }
    const QPointF pointer = KWin::effects->cursorPos();
    const QRectF frame = state->targetFrame;
    if (frame.contains(pointer) && input.warpMouseMode != Config::WarpMouseMode::CenterXYAlways) {
        return;
    }
    const bool separate = input.warpMouseMode == Config::WarpMouseMode::Separate;
    const double x = separate ? std::clamp(pointer.x(), frame.left(), frame.right()) : frame.center().x();
    const double y = separate ? std::clamp(pointer.y(), frame.top(), frame.bottom()) : frame.center().y();
    KWin::input()->warpPointer({x, y});
}

}
