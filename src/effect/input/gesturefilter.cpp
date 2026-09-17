#include "input/gesturefilter.h"

#include <input_event.h>

namespace Konveyor
{

namespace
{

qint64 millisecondsOf(std::chrono::microseconds time)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(time).count();
}

}

GestureFilter::GestureFilter(GestureHandlers handlers)
    : KWin::InputEventFilter(KWin::InputFilterOrder::TabBox)
    , m_handlers(std::move(handlers))
{
    KWin::input()->installInputEventFilter(this);
}

GestureFilter::~GestureFilter() = default;

bool GestureFilter::swipeGestureBegin(KWin::PointerSwipeGestureBeginEvent *event)
{
    return m_handlers.swipeBegin(event->fingerCount);
}

bool GestureFilter::swipeGestureUpdate(KWin::PointerSwipeGestureUpdateEvent *event)
{
    return m_handlers.swipeUpdate(event->delta, millisecondsOf(event->time));
}

bool GestureFilter::swipeGestureEnd(KWin::PointerSwipeGestureEndEvent *event)
{
    Q_UNUSED(event)
    return m_handlers.swipeEnd();
}

bool GestureFilter::swipeGestureCancelled(KWin::PointerSwipeGestureCancelEvent *event)
{
    Q_UNUSED(event)
    return m_handlers.swipeEnd();
}

bool GestureFilter::pinchGestureBegin(KWin::PointerPinchGestureBeginEvent *event)
{
    return m_handlers.pinchBegin(event->fingerCount);
}

bool GestureFilter::pinchGestureUpdate(KWin::PointerPinchGestureUpdateEvent *event)
{
    return m_handlers.pinchUpdate(event->scale);
}

bool GestureFilter::pinchGestureEnd(KWin::PointerPinchGestureEndEvent *event)
{
    Q_UNUSED(event)
    return m_handlers.pinchEnd();
}

bool GestureFilter::pinchGestureCancelled(KWin::PointerPinchGestureCancelEvent *event)
{
    Q_UNUSED(event)
    return m_handlers.pinchEnd();
}

bool GestureFilter::touchDown(KWin::TouchDownEvent *event)
{
    m_downIds.insert(event->id);
    const bool consumed = m_handlers.touchDown(event->id, event->pos, millisecondsOf(event->time));
    if (consumed && !m_touchGestureTaken) {
        m_touchGestureTaken = true;
        m_syntheticCancel = true;
        KWin::input()->processFilters(&KWin::InputEventFilter::touchCancel);
        m_syntheticCancel = false;
    }
    return consumed;
}

bool GestureFilter::touchMotion(KWin::TouchMotionEvent *event)
{
    return m_handlers.touchMotion(event->id, event->pos, millisecondsOf(event->time));
}

bool GestureFilter::touchUp(KWin::TouchUpEvent *event)
{
    const bool consumed = m_handlers.touchUp(event->id);
    m_downIds.remove(event->id);
    if (m_downIds.isEmpty()) {
        m_touchGestureTaken = false;
    }
    return consumed;
}

bool GestureFilter::touchCancel()
{
    if (m_syntheticCancel) {
        return false;
    }
    m_touchGestureTaken = false;
    m_downIds.clear();
    m_handlers.touchCancel();
    return false;
}

}
