#include "layout/gestures/gesturerouter.h"

#include "layout/engine/engine.h"

#include <cmath>
#include <utility>

namespace Konveyor::Layout
{

namespace
{

constexpr double AxisThreshold = 16.0;
constexpr double TouchMoveThreshold = 12.0;
constexpr double PinchInScale = 0.8;
constexpr double PinchOutScale = 1.25;
constexpr double PinchSpreadThreshold = 0.2;

double lengthOf(QPointF point)
{
    return std::hypot(point.x(), point.y());
}

}

GestureRouter::GestureRouter(Engine &engine)
    : m_engine(engine)
{ }

void GestureRouter::setConfig(const Config::Gestures &gestures)
{
    m_config = gestures;
}

const Config::MultiTouch &GestureRouter::settingsFor(GestureDevice device) const
{
    return device == GestureDevice::Touchpad ? m_config.touchpad : m_config.touchscreen;
}

void GestureRouter::beginGesture(Swipe &gesture, GestureDevice device, const QString &output, bool allowsSwipe, bool allowsPinch)
{
    gesture = Swipe {};
    gesture.active = true;
    gesture.device = device;
    gesture.output = output;
    gesture.allowsSwipe = allowsSwipe;
    gesture.allowsPinch = allowsPinch;
}

void GestureRouter::feedTranslation(Swipe &gesture, QPointF delta, qint64 timestampMs)
{
    const Config::MultiTouch &settings = settingsFor(gesture.device);
    const bool touchpad = gesture.device == GestureDevice::Touchpad;
    const double sign = settings.naturalSwipe ? -1.0 : 1.0;
    if (gesture.axis == Axis::Undecided) {
        if (!gesture.allowsSwipe) {
            return;
        }
        gesture.pending += delta;
        if (lengthOf(gesture.pending) < AxisThreshold) {
            return;
        }
        const bool horizontal = std::abs(gesture.pending.x()) >= std::abs(gesture.pending.y());
        if (horizontal && settings.horizontalSwipe == Config::HorizontalSwipe::ScrollView) {
            gesture.axis = Axis::Horizontal;
            m_engine.beginSwipe(gesture.output, touchpad);
        } else if (!horizontal && settings.verticalSwipe == Config::VerticalSwipe::SwitchWorkspace) {
            gesture.axis = Axis::Vertical;
            m_engine.beginWorkspaceSwipe(gesture.output, touchpad);
        } else {
            gesture.axis = Axis::Ignored;
            return;
        }
        delta = gesture.pending;
    }
    if (gesture.axis == Axis::Horizontal) {
        m_engine.updateSwipe(sign * delta.x(), timestampMs, touchpad);
    } else if (gesture.axis == Axis::Vertical) {
        m_engine.updateWorkspaceSwipe(sign * delta.y(), timestampMs, touchpad);
    }
}

void GestureRouter::feedPinch(Swipe &gesture, double scale)
{
    if (!gesture.allowsPinch || gesture.pinchTriggered || (gesture.axis != Axis::Undecided && gesture.axis != Axis::Pinch)) {
        return;
    }
    if (scale > PinchInScale && scale < PinchOutScale) {
        return;
    }
    gesture.axis = Axis::Pinch;
    gesture.pinchTriggered = true;
    m_engine.setOverviewOpen(scale <= PinchInScale);
}

bool GestureRouter::finishGesture(Swipe &gesture)
{
    if (!gesture.active) {
        return false;
    }
    const bool touchpad = gesture.device == GestureDevice::Touchpad;
    if (gesture.axis == Axis::Horizontal) {
        m_engine.endSwipe(touchpad);
    } else if (gesture.axis == Axis::Vertical) {
        m_engine.endWorkspaceSwipe(touchpad);
    }
    const bool consumed = gesture.axis != Axis::Ignored;
    gesture = Swipe {};
    return consumed;
}

bool GestureRouter::touchpadSwipeBegin(int fingers, const QString &output)
{
    const Config::MultiTouch &settings = m_config.touchpad;
    if (!settings.enabled || fingers != settings.swipeFingers) {
        return false;
    }
    const bool allowsSwipe
        = settings.horizontalSwipe != Config::HorizontalSwipe::Off || settings.verticalSwipe != Config::VerticalSwipe::Off;
    if (!allowsSwipe) {
        return false;
    }
    beginGesture(m_touchpad, GestureDevice::Touchpad, output, true, false);
    return true;
}

bool GestureRouter::touchpadSwipeUpdate(QPointF delta, qint64 timestampMs)
{
    if (!m_touchpad.active || m_touchpad.axis == Axis::Ignored) {
        return false;
    }
    feedTranslation(m_touchpad, delta, timestampMs);
    return m_touchpad.axis != Axis::Ignored;
}

bool GestureRouter::touchpadSwipeEnd()
{
    return m_touchpad.device == GestureDevice::Touchpad && finishGesture(m_touchpad);
}

bool GestureRouter::touchpadPinchBegin(int fingers)
{
    const Config::MultiTouch &settings = m_config.touchpad;
    if (!settings.enabled || fingers != settings.pinchFingers || settings.pinch == Config::PinchAction::Off) {
        return false;
    }
    beginGesture(m_touchpad, GestureDevice::Touchpad, QString(), false, true);
    return true;
}

bool GestureRouter::touchpadPinchUpdate(double scale)
{
    if (!m_touchpad.active || !m_touchpad.allowsPinch) {
        return false;
    }
    feedPinch(m_touchpad, scale);
    return true;
}

bool GestureRouter::touchpadPinchEnd()
{
    if (!m_touchpad.active || !m_touchpad.allowsPinch) {
        return false;
    }
    m_touchpad = Swipe {};
    return true;
}

QPointF GestureRouter::centroid() const
{
    QPointF sum;
    for (const QPointF &point : m_points) {
        sum += point;
    }
    return m_points.isEmpty() ? sum : sum / static_cast<double>(m_points.size());
}

double GestureRouter::spread() const
{
    const QPointF center = centroid();
    double total = 0.0;
    for (const QPointF &point : m_points) {
        total += lengthOf(point - center);
    }
    return m_points.isEmpty() ? 0.0 : total / static_cast<double>(m_points.size());
}

bool GestureRouter::isLongPress(qint64 timestampMs) const
{
    const qint64 stillUntil = m_firstTouchMoved ? m_firstMoveMs : timestampMs;
    return m_config.touchscreen.enabled && m_config.touchscreen.longPressToMove && m_points.size() == 1
        && stillUntil - m_firstTouchMs >= m_config.touchscreen.longPressMs;
}

bool GestureRouter::touchDown(qint32 id, QPointF position, qint64 timestampMs, const QString &output)
{
    if (m_points.isEmpty()) {
        m_firstTouchMs = timestampMs;
        m_firstTouchStart = position;
        m_firstTouchMoved = false;
    }
    m_points.insert(id, position);
    m_lastTouch = position;
    const Config::MultiTouch &settings = m_config.touchscreen;
    if (!settings.enabled) {
        return false;
    }
    const int count = static_cast<int>(m_points.size());
    const bool allowsSwipe = count == settings.swipeFingers
        && (settings.horizontalSwipe != Config::HorizontalSwipe::Off || settings.verticalSwipe != Config::VerticalSwipe::Off);
    const bool allowsPinch = count == settings.pinchFingers && settings.pinch != Config::PinchAction::Off;
    if (m_touch.active) {
        if (m_touch.axis == Axis::Undecided) {
            m_touch.allowsSwipe = allowsSwipe;
            m_touch.allowsPinch = allowsPinch;
            m_touch.pending = QPointF();
        }
        m_gestureIds.insert(id);
        m_lastCentroid = centroid();
        m_touch.startSpread = spread();
        return true;
    }
    if (!allowsSwipe && !allowsPinch) {
        return false;
    }
    beginGesture(m_touch, GestureDevice::Touchscreen, output, allowsSwipe, allowsPinch);
    m_gestureIds.insert(id);
    m_lastCentroid = centroid();
    m_touch.startSpread = spread();
    return true;
}

bool GestureRouter::touchMotion(qint32 id, QPointF position, qint64 timestampMs)
{
    const auto point = m_points.find(id);
    if (point == m_points.end()) {
        return false;
    }
    if (m_points.size() == 1 && !m_firstTouchMoved && lengthOf(position - m_firstTouchStart) >= TouchMoveThreshold) {
        m_firstTouchMoved = true;
        m_firstMoveMs = timestampMs;
    }
    *point = position;
    m_lastTouch = position;
    if (!m_touch.active) {
        return m_gestureIds.contains(id);
    }
    const QPointF center = centroid();
    const QPointF delta = center - std::exchange(m_lastCentroid, center);
    if (m_touch.allowsPinch && m_touch.startSpread > 0.0 && m_touch.axis == Axis::Undecided) {
        const double ratio = spread() / m_touch.startSpread;
        if (std::abs(ratio - 1.0) >= PinchSpreadThreshold) {
            feedPinch(m_touch, ratio);
            return true;
        }
    }
    feedTranslation(m_touch, delta, timestampMs);
    return true;
}

bool GestureRouter::touchUp(qint32 id)
{
    m_points.remove(id);
    if (m_touch.active) {
        finishGesture(m_touch);
    }
    return m_gestureIds.remove(id);
}

void GestureRouter::touchCancel()
{
    finishGesture(m_touch);
    m_points.clear();
    m_gestureIds.clear();
    m_lastTouch.reset();
}

}
