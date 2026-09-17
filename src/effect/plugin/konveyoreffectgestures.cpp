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

bool KonveyorEffect::isTouchLongPress() const
{
    const qint64 now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    return d->gestures.touchPointCount() == 1 && d->gestures.isLongPress(now);
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
    if (d->titlebarDrag && d->gestures.touchPointCount() == 1) {
        handlePointerMotion(position, timestampMs);
        return true;
    }
    return false;
}

bool KonveyorEffect::handleTouchUp(qint32 id)
{
    const bool dragging = d->titlebarDrag.has_value();
    const bool consumed = routeGesture(d->gestures.touchUp(id));
    if (dragging && d->gestures.touchPointCount() == 0) {
        endTitlebarDrag();
        return true;
    }
    return consumed;
}

}
