#include "plugin/konveyoreffect_p.h"

namespace Konveyor
{

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
    if (const std::optional<Layout::WindowId> id = d->windows.idOf(KWin::workspace()->activeWindow())) {
        changeEngine().activateWindow(*id);
    } else {
        changeEngine().setLayoutFocused(false);
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
        followActiveWindow();
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
