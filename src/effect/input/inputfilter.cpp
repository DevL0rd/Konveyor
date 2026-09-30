#include "input/inputfilter.h"

#include <input_event.h>
#include <wayland/seat.h>
#include <wayland_server.h>

#include <optional>

namespace Konveyor
{

namespace
{

std::optional<Config::ScrollDirection> directionOf(const KWin::PointerAxisEvent *event)
{
    if (event->delta == 0) {
        return std::nullopt;
    }
    const bool positive = event->delta > 0;
    if (event->orientation == Qt::Vertical) {
        return positive ? Config::ScrollDirection::Down : Config::ScrollDirection::Up;
    }
    return positive ? Config::ScrollDirection::Right : Config::ScrollDirection::Left;
}

std::optional<Config::MouseButton> buttonOf(Qt::MouseButton button)
{
    switch (button) {
    case Qt::LeftButton:
        return Config::MouseButton::Left;
    case Qt::RightButton:
        return Config::MouseButton::Right;
    case Qt::MiddleButton:
        return Config::MouseButton::Middle;
    case Qt::BackButton:
        return Config::MouseButton::Back;
    case Qt::ForwardButton:
        return Config::MouseButton::Forward;
    default:
        return std::nullopt;
    }
}

Config::BindTrigger scrollTriggerOf(const KWin::PointerAxisEvent *event)
{
    return event->source == KWin::PointerAxisSource::Finger ? Config::BindTrigger::TouchpadScroll : Config::BindTrigger::Wheel;
}

}

InputFilter::InputFilter(InputHandlers handlers)
    : KWin::InputEventFilter(KWin::InputFilterOrder::GlobalShortcut)
    , m_handlers(std::move(handlers))
{
    KWin::input()->installInputEventFilter(this);
}

InputFilter::~InputFilter() = default;

DragMotionFilter::DragMotionFilter(std::function<void(const QPointF &, qint64)> moved)
    : KWin::InputEventFilter(KWin::InputFilterOrder::DragAndDrop)
    , m_moved(std::move(moved))
{
    KWin::input()->installInputEventFilter(this);
}

bool DragMotionFilter::pointerMotion(KWin::PointerMotionEvent *event)
{
    if (KWin::waylandServer()->seat()->isDragPointer()) {
        m_moved(event->position, event->timestamp.count() / 1000);
    }
    return false;
}

AxisFilter::AxisFilter(PointerBind pointerBind)
    : KWin::InputEventFilter(KWin::InputFilterOrder::Effects)
    , m_pointerBind(std::move(pointerBind))
{
    KWin::input()->installInputEventFilter(this);
}

bool AxisFilter::pointerAxis(KWin::PointerAxisEvent *event)
{
    const std::optional<Config::ScrollDirection> direction = directionOf(event);
    if (!direction || event->modifiersRelevantForGlobalShortcuts == Qt::NoModifier) {
        return false;
    }
    return m_pointerBind(scrollTriggerOf(event), event->modifiersRelevantForGlobalShortcuts, Config::MouseButton::Left, *direction);
}

bool InputFilter::pointerButton(KWin::PointerButtonEvent *event)
{
    const std::optional<Config::MouseButton> button = buttonOf(event->button);
    const bool pressed = event->state == KWin::PointerButtonState::Pressed;
    if (!pressed) {
        m_handlers.pointerReleased();
        return m_swallowedButtons.remove(event->button);
    }
    if (event->modifiersRelevantForShortcuts == Qt::NoModifier) {
        const bool onTab = event->button == Qt::LeftButton && m_handlers.tabClicked(event->position);
        if (onTab) {
            m_swallowedButtons.insert(event->button);
        }
        return onTab;
    }
    if (!button) {
        return false;
    }
    return m_handlers.pointerBind(
        Config::BindTrigger::MouseButton, event->modifiersRelevantForShortcuts, *button, Config::ScrollDirection::Down);
}

bool InputFilter::pointerMotion(KWin::PointerMotionEvent *event)
{
    m_handlers.pointerMoved(event->position, event->timestamp.count() / 1000);
    return false;
}

bool InputFilter::keyboardKey(KWin::KeyboardKeyEvent *event)
{
    if (event->state == KWin::KeyboardKeyState::Released) {
        return m_swallowedKeys.remove(event->nativeScanCode);
    }
    if (triggersBind(event)) {
        m_swallowedKeys.insert(event->nativeScanCode);
        return true;
    }
    if (event->state == KWin::KeyboardKeyState::Pressed && event->key == Qt::Key_Escape) {
        m_handlers.escapePressed();
    }
    return false;
}

bool InputFilter::triggersBind(const KWin::KeyboardKeyEvent *event)
{
    if (event->modifiersRelevantForGlobalShortcuts == Qt::NoModifier) {
        return false;
    }
    const bool repeat = event->state == KWin::KeyboardKeyState::Repeated;
    return m_handlers.keyPositionBind(event->nativeScanCode + 8, event->modifiersRelevantForGlobalShortcuts, repeat);
}

}
