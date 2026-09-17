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
constexpr double TouchpadWindowStep = 160.0;
constexpr double TouchscreenWindowStep = 320.0;
constexpr double TouchpadTapTravelMm = 3.0;
constexpr double TouchscreenTapTravel = 24.0;

double lengthOf(QPointF point)
{
    return std::hypot(point.x(), point.y());
}

}

GestureRouter::GestureRouter(Engine &engine)
    : m_engine(engine)
    , m_touchpadTaps(TouchpadTapTravelMm)
    , m_touchTaps(TouchscreenTapTravel)
{ }

void GestureRouter::setConfig(const Config::Gestures &gestures)
{
    m_config = gestures;
}

const Config::MultiTouch &GestureRouter::settingsFor(GestureDevice device) const
{
    return device == GestureDevice::Touchpad ? m_config.touchpad : m_config.touchscreen;
}

GestureRouter::Allowed GestureRouter::allowedFor(const Config::MultiTouch &settings, int fingers)
{
    if (!settings.enabled) {
        return {};
    }
    Allowed allowed;
    allowed.swipe = fingers == settings.swipeFingers
        && (settings.horizontalSwipe != Config::HorizontalSwipe::Off || settings.verticalSwipe != Config::VerticalSwipe::Off);
    allowed.pinch = fingers == settings.pinchFingers && settings.pinch != Config::PinchAction::Off;
    allowed.windowSwipe = !allowed.swipe && fingers == settings.windowSwipeFingers
        && (settings.windowHorizontalSwipe != Config::WindowHorizontalSwipe::Off
            || settings.windowVerticalSwipe != Config::WindowVerticalSwipe::Off);
    allowed.tap = settings.tap(fingers) != Config::TapAction::Off;
    return allowed;
}

void GestureRouter::beginGesture(Swipe &gesture, GestureDevice device, const QString &output, Allowed allowed)
{
    gesture = Swipe {};
    gesture.active = true;
    gesture.device = device;
    gesture.output = output;
    gesture.allowsSwipe = allowed.swipe;
    gesture.allowsPinch = allowed.pinch;
    gesture.allowsWindowSwipe = allowed.windowSwipe;
}

bool GestureRouter::decideAxis(Swipe &gesture, QPointF delta)
{
    const Config::MultiTouch &settings = settingsFor(gesture.device);
    const bool touchpad = gesture.device == GestureDevice::Touchpad;
    gesture.pending += delta;
    if (lengthOf(gesture.pending) < (gesture.allowsPinch ? AxisThreshold * 3.0 : AxisThreshold)) {
        return false;
    }
    const bool horizontal = std::abs(gesture.pending.x()) >= std::abs(gesture.pending.y());
    if (gesture.allowsSwipe && horizontal && settings.horizontalSwipe == Config::HorizontalSwipe::ScrollView) {
        gesture.axis = Axis::Horizontal;
        m_engine.beginSwipe(gesture.output, touchpad);
    } else if (gesture.allowsSwipe && !horizontal && settings.verticalSwipe == Config::VerticalSwipe::SwitchWorkspace) {
        gesture.axis = Axis::Vertical;
        m_engine.beginWorkspaceSwipe(gesture.output, touchpad);
    } else if (gesture.allowsWindowSwipe && horizontal && settings.windowHorizontalSwipe != Config::WindowHorizontalSwipe::Off) {
        gesture.axis = Axis::WindowHorizontal;
    } else if (gesture.allowsWindowSwipe && !horizontal && settings.windowVerticalSwipe != Config::WindowVerticalSwipe::Off) {
        gesture.axis = Axis::WindowVertical;
        focusWindowUnderFingers(gesture);
    } else {
        gesture.axis = Axis::Ignored;
        return false;
    }
    return true;
}

void GestureRouter::focusWindowUnderFingers(const Swipe &gesture)
{
    if (gesture.device != GestureDevice::Touchscreen) {
        return;
    }
    if (const std::optional<WindowId> window = m_engine.windowAt(centroid())) {
        m_engine.perform(Config::Action {QStringLiteral("focus-window"), {}, {}}, window);
    }
}

void GestureRouter::feedWindowSwipe(Swipe &gesture, double delta)
{
    gesture.travel += delta;
    const bool horizontal = gesture.axis == Axis::WindowHorizontal;
    const double step = gesture.device == GestureDevice::Touchpad ? TouchpadWindowStep : TouchscreenWindowStep;
    while (std::abs(gesture.travel) >= step) {
        const bool forward = gesture.travel > 0.0;
        gesture.travel -= forward ? step : -step;
        const QString name = horizontal
            ? (forward ? QStringLiteral("consume-or-expel-window-right") : QStringLiteral("consume-or-expel-window-left"))
            : (forward ? QStringLiteral("move-window-down-or-to-workspace-down") : QStringLiteral("move-window-up-or-to-workspace-up"));
        m_engine.perform(Config::Action {name, {}, {}});
    }
}

void GestureRouter::feedTranslation(Swipe &gesture, QPointF delta, qint64 timestampMs)
{
    const bool touchpad = gesture.device == GestureDevice::Touchpad;
    const double sign = settingsFor(gesture.device).naturalSwipe ? -1.0 : 1.0;
    if (gesture.axis == Axis::Undecided) {
        if ((!gesture.allowsSwipe && !gesture.allowsWindowSwipe) || !decideAxis(gesture, delta)) {
            return;
        }
        delta = gesture.pending;
    }
    switch (gesture.axis) {
    case Axis::Horizontal:
        m_engine.updateSwipe(sign * delta.x(), timestampMs, touchpad);
        break;
    case Axis::Vertical:
        m_engine.updateWorkspaceSwipe(sign * delta.y(), timestampMs, touchpad);
        break;
    case Axis::WindowHorizontal:
        feedWindowSwipe(gesture, delta.x());
        break;
    case Axis::WindowVertical:
        feedWindowSwipe(gesture, delta.y());
        break;
    default:
        break;
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
    m_touchpadTaps.invalidate();
    Allowed allowed = allowedFor(m_config.touchpad, fingers);
    allowed.pinch = false;
    if (!allowed.swipe && !allowed.windowSwipe) {
        return false;
    }
    beginGesture(m_touchpad, GestureDevice::Touchpad, output, allowed);
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
    m_touchpadTaps.invalidate();
    const Config::MultiTouch &settings = m_config.touchpad;
    if (!settings.enabled || fingers != settings.pinchFingers || settings.pinch == Config::PinchAction::Off) {
        return false;
    }
    beginGesture(m_touchpad, GestureDevice::Touchpad, QString(), Allowed {false, true, false});
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
    m_touchTaps.down(id, position, timestampMs);
    m_lastTouch = position;
    if (!m_config.touchscreen.enabled) {
        return false;
    }
    const Allowed allowed = allowedFor(m_config.touchscreen, static_cast<int>(m_points.size()));
    if (m_touch.active) {
        if (m_touch.axis == Axis::Undecided) {
            m_touch.allowsSwipe = allowed.swipe;
            m_touch.allowsPinch = allowed.pinch;
            m_touch.allowsWindowSwipe = allowed.windowSwipe;
            m_touch.pending = QPointF();
        }
        m_gestureIds.insert(id);
        m_lastCentroid = centroid();
        m_touch.startSpread = spread();
        return true;
    }
    if (!allowed.swipe && !allowed.pinch && !allowed.windowSwipe && !allowed.tap) {
        return false;
    }
    beginGesture(m_touch, GestureDevice::Touchscreen, output, allowed);
    for (auto it = m_points.cbegin(); it != m_points.cend(); ++it) {
        m_gestureIds.insert(it.key());
    }
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
    m_touchTaps.motion(id, position);
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
            m_touchTaps.invalidate();
            return true;
        }
    }
    feedTranslation(m_touch, delta, timestampMs);
    if (m_touch.axis != Axis::Undecided) {
        m_touchTaps.invalidate();
    }
    return true;
}

bool GestureRouter::touchUp(qint32 id, qint64 timestampMs)
{
    m_points.remove(id);
    if (m_touch.active) {
        finishGesture(m_touch);
    }
    if (const std::optional<int> fingers = m_touchTaps.up(id, timestampMs); fingers && m_config.touchscreen.enabled) {
        performTap(m_config.touchscreen, *fingers, m_touchTaps.centroid());
    }
    return m_gestureIds.remove(id);
}

bool GestureRouter::resetTouches()
{
    const bool hadGesture = m_touch.active;
    touchCancel();
    return hadGesture;
}

void GestureRouter::touchCancel()
{
    finishGesture(m_touch);
    m_touchTaps.cancel();
    m_points.clear();
    m_gestureIds.clear();
    m_lastTouch.reset();
}

}
