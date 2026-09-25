#include "input/inputfilter.h"

#include <input_event.h>

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
    : KWin::InputEventFilter(KWin::InputFilterOrder::Effects)
    , m_handlers(std::move(handlers))
{
    KWin::input()->installInputEventFilter(this);
}

InputFilter::~InputFilter() = default;

bool InputFilter::pointerAxis(KWin::PointerAxisEvent *event)
{
    const std::optional<Config::ScrollDirection> direction = directionOf(event);
    if (!direction || event->modifiersRelevantForGlobalShortcuts == Qt::NoModifier) {
        return false;
    }
    return m_handlers.pointerBind(
        scrollTriggerOf(event), event->modifiersRelevantForGlobalShortcuts, Config::MouseButton::Left, *direction);
}

bool InputFilter::pointerButton(KWin::PointerButtonEvent *event)
{
    const std::optional<Config::MouseButton> button = buttonOf(event->button);
    const bool pressed = event->state == KWin::PointerButtonState::Pressed;
    if (!pressed) {
        m_handlers.pointerReleased();
    }
    if (!pressed || !button || event->modifiersRelevantForShortcuts == Qt::NoModifier) {
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
    if (event->modifiersRelevantForGlobalShortcuts == Qt::NoModifier) {
        return false;
    }
    const bool repeat = event->state == KWin::KeyboardKeyState::Repeated;
    if (!m_handlers.keyPositionBind(event->nativeScanCode + 8, event->modifiersRelevantForGlobalShortcuts, repeat)) {
        return false;
    }
    m_swallowedKeys.insert(event->nativeScanCode);
    return true;
}

}
