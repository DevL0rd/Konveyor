#include "layout/engine/engine.h"
#include "layout/gestures/gesturerouter.h"

namespace Konveyor::Layout
{

void GestureRouter::touchpadContactDown(qint32 slot, QPointF millimeters, qint64 timestampMs)
{
    m_touchpadTaps.down(slot, millimeters, timestampMs);
}

void GestureRouter::touchpadContactMotion(qint32 slot, QPointF millimeters)
{
    m_touchpadTaps.motion(slot, millimeters);
}

bool GestureRouter::touchpadContactUp(qint32 slot, qint64 timestampMs)
{
    const std::optional<int> fingers = m_touchpadTaps.up(slot, timestampMs);
    return fingers && m_config.touchpad.enabled && performTap(m_config.touchpad, *fingers, std::nullopt);
}

void GestureRouter::touchpadPhysicalClick()
{
    m_touchpadTaps.press();
}

void GestureRouter::touchpadContactsReset()
{
    m_touchpadTaps.cancel();
}

bool GestureRouter::takesTouchpadTapButton() const
{
    return m_config.touchpad.enabled && m_config.touchpad.threeFingerTap != Config::TapAction::Off && m_touchpadTaps.fingers() == 3
        && !m_touchpadTaps.pressed();
}

bool GestureRouter::performTap(const Config::MultiTouch &settings, int fingers, std::optional<QPointF> position)
{
    switch (settings.tap(fingers)) {
    case Config::TapAction::CycleWidth:
        if (const std::optional<WindowId> window = position ? m_engine.windowAt(*position) : std::nullopt) {
            m_engine.perform(Config::Action {QStringLiteral("focus-window"), {}, {}}, window);
        }
        m_engine.perform(Config::Action {QStringLiteral("switch-preset-column-width"), {}, {}});
        return true;
    case Config::TapAction::KontrolPanel:
        m_engine.perform(Config::Action {QStringLiteral("spawn-sh"), {QStringLiteral("\"$HOME/.local/bin/portal-launcher\" toggle")}, {}});
        return true;
    case Config::TapAction::ToggleOverview:
        m_engine.perform(Config::Action {QStringLiteral("toggle-overview"), {}, {}});
        return true;
    case Config::TapAction::Off:
        break;
    }
    return false;
}

}
