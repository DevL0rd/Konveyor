#include "input/inputfilter.h"

#include <input_event.h>
#include <keyboard_input.h>
#include <wayland/seat.h>
#include <wayland_server.h>
#include <xkb.h>

#include <xkbcommon/xkbcommon-names.h>

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

bool levelActive(xkb_state *state, const char *name)
{
    return state && xkb_state_mod_name_is_active(state, name, XKB_STATE_MODS_EFFECTIVE) > 0;
}

Config::BindModifiers bindModifiersOf(Qt::KeyboardModifiers modifiers)
{
    Config::BindModifiers result;
    const QList<std::pair<Qt::KeyboardModifier, Config::BindModifier>> mapping {
        {Qt::ControlModifier, Config::BindModifier::Ctrl},
        {Qt::ShiftModifier, Config::BindModifier::Shift},
        {Qt::AltModifier, Config::BindModifier::Alt},
        {Qt::MetaModifier, Config::BindModifier::Super},
    };
    for (const auto &[qtModifier, bindModifier] : mapping) {
        if (modifiers.testFlag(qtModifier)) {
            result |= bindModifier;
        }
    }
    xkb_state *state = KWin::input()->keyboard()->xkb()->state();
    if (levelActive(state, XKB_VMOD_NAME_LEVEL3)) {
        result |= Config::BindModifier::IsoLevel3Shift;
    }
    if (levelActive(state, XKB_VMOD_NAME_LEVEL5)) {
        result |= Config::BindModifier::IsoLevel5Shift;
    }
    return result;
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
    if (!direction) {
        return false;
    }
    return m_pointerBind(
        scrollTriggerOf(event), bindModifiersOf(event->modifiersRelevantForGlobalShortcuts), Config::MouseButton::Left, *direction);
}

bool InputFilter::pointerButton(KWin::PointerButtonEvent *event)
{
    const std::optional<Config::MouseButton> button = buttonOf(event->button);
    const bool pressed = event->state == KWin::PointerButtonState::Pressed;
    if (!pressed) {
        m_handlers.pointerReleased();
        return m_swallowedButtons.remove(event->button);
    }
    if (event->button == Qt::RightButton && m_handlers.toggleDragFloating()) {
        m_swallowedButtons.insert(event->button);
        return true;
    }
    const Config::BindModifiers modifiers = bindModifiersOf(event->modifiersRelevantForShortcuts);
    if (!modifiers && event->button == Qt::LeftButton && m_handlers.tabClicked(event->position)) {
        m_swallowedButtons.insert(event->button);
        return true;
    }
    if (!button) {
        return false;
    }
    return m_handlers.pointerBind(Config::BindTrigger::MouseButton, modifiers, *button, Config::ScrollDirection::Down);
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
    const Config::BindModifiers modifiers = bindModifiersOf(event->modifiersRelevantForGlobalShortcuts);
    if (!modifiers) {
        return false;
    }
    const bool repeat = event->state == KWin::KeyboardKeyState::Repeated;
    return m_handlers.keyPositionBind(event->nativeScanCode + 8, modifiers, repeat);
}

}
