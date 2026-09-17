#pragma once

#include <input.h>

#include <QPointF>

#include <functional>

namespace Konveyor
{

struct GestureHandlers
{
    std::function<bool(int)> swipeBegin;
    std::function<bool(const QPointF &, qint64)> swipeUpdate;
    std::function<bool()> swipeEnd;
    std::function<bool(int)> pinchBegin;
    std::function<bool(double)> pinchUpdate;
    std::function<bool()> pinchEnd;
    std::function<bool(qint32, const QPointF &, qint64)> touchDown;
    std::function<bool(qint32, const QPointF &, qint64)> touchMotion;
    std::function<bool(qint32)> touchUp;
    std::function<void()> touchCancel;
    std::function<void()> touchSequenceStarted;
};

class GestureFilter : public KWin::InputEventFilter
{
public:
    explicit GestureFilter(GestureHandlers handlers);
    ~GestureFilter() override;

    bool swipeGestureBegin(KWin::PointerSwipeGestureBeginEvent *event) override;
    bool swipeGestureUpdate(KWin::PointerSwipeGestureUpdateEvent *event) override;
    bool swipeGestureEnd(KWin::PointerSwipeGestureEndEvent *event) override;
    bool swipeGestureCancelled(KWin::PointerSwipeGestureCancelEvent *event) override;
    bool pinchGestureBegin(KWin::PointerPinchGestureBeginEvent *event) override;
    bool pinchGestureUpdate(KWin::PointerPinchGestureUpdateEvent *event) override;
    bool pinchGestureEnd(KWin::PointerPinchGestureEndEvent *event) override;
    bool pinchGestureCancelled(KWin::PointerPinchGestureCancelEvent *event) override;
    bool touchDown(KWin::TouchDownEvent *event) override;
    bool touchMotion(KWin::TouchMotionEvent *event) override;
    bool touchUp(KWin::TouchUpEvent *event) override;
    bool touchCancel() override;

private:
    GestureHandlers m_handlers;
    bool m_touchGestureTaken = false;
    bool m_syntheticCancel = false;
};

}
