#include "plugin/konveyoreffect_p.h"

namespace Konveyor
{

bool KonveyorEffect::routeGesture(bool consumed)
{
    if (consumed) {
        changeEngine();
    }
    return consumed;
}

QPointF KonveyorEffect::interactionPoint() const
{
    const std::optional<QPointF> touch = d->gestures.lastTouchPosition();
    return d->gestures.touchPointCount() > 0 && touch ? *touch : KWin::effects->cursorPos();
}

bool KonveyorEffect::holdsToDecide(Layout::WindowId id, int phase)
{
    if (d->touchMovePending == id) {
        if (phase == interactivePhaseEnd) {
            d->touchMovePending.reset();
        }
        return true;
    }
    const Config::MultiTouch &touch = d->config.config().gestures.touchscreen;
    if (phase != interactivePhaseStart || d->gestures.touchPointCount() != 1 || !touch.enabled || !touch.longPressToMove) {
        return false;
    }
    d->touchMovePending = id;
    return true;
}

void KonveyorEffect::decidePendingTouchMove(qint64 timestampMs)
{
    if (!d->touchMovePending || !d->gestures.hasFirstTouchMoved()) {
        return;
    }
    const Layout::WindowId id = *std::exchange(d->touchMovePending, std::nullopt);
    KWin::Window *window = d->windows.windowOf(id);
    if (!window) {
        return;
    }
    if (d->gestures.isLongPress(timestampMs)) {
        d->touchLift = id;
        handleWindowMove(id, window, interactivePhaseStart);
    } else {
        handleTitlebarDrag(id, window, interactivePhaseStart);
    }
}

bool KonveyorEffect::handleTouchDown(qint32 id, const QPointF &position, qint64 timestampMs)
{
    if (!routeGesture(d->gestures.touchDown(id, position, timestampMs, outputNameAt(position)))) {
        return false;
    }
    endTitlebarDrag();
    return true;
}

bool KonveyorEffect::handleTouchMotion(qint32 id, const QPointF &position, qint64 timestampMs)
{
    if (routeGesture(d->gestures.touchMotion(id, position, timestampMs))) {
        return true;
    }
    decidePendingTouchMove(timestampMs);
    if (d->titlebarDrag && d->gestures.touchPointCount() == 1) {
        handlePointerMotion(position, timestampMs);
    }
    return false;
}

bool KonveyorEffect::handleTouchUp(qint32 id, qint64 timestampMs)
{
    const bool consumed = routeGesture(d->gestures.touchUp(id, timestampMs));
    if (d->gestures.touchPointCount() == 0) {
        endTitlebarDrag();
    }
    return consumed;
}

}
