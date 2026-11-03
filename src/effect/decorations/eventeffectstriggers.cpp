#include "decorations/eventeffects.h"

#include <scene/item.h>
#include <window.h>

#include <algorithm>

namespace Konveyor
{

namespace
{

Render::Signal signalOf(Config::EventTrigger trigger)
{
    switch (trigger) {
    case Config::EventTrigger::Focus:
        return Render::Signal::Focus;
    case Config::EventTrigger::Unfocus:
        return Render::Signal::Unfocus;
    case Config::EventTrigger::Click:
        return Render::Signal::Click;
    case Config::EventTrigger::Scroll:
        return Render::Signal::ScrollStep;
    case Config::EventTrigger::Keypress:
        return Render::Signal::Keypress;
    case Config::EventTrigger::Urgent:
        return Render::Signal::Urgent;
    case Config::EventTrigger::Beat:
        return Render::Signal::AudioBeat;
    default:
        return Render::Signal::ViewStep;
    }
}

}

void EventEffects::updateWanted()
{
    Render::SignalSet wanted;
    for (const Known &window : m_known) {
        for (std::size_t index = 0; index < Config::kEventTriggerCount; ++index) {
            const std::optional<Config::EventEffect> &effect = window.effects.slots_[index];
            if (effect && effect->style != Config::kBorderStyleNone) {
                wanted.set(static_cast<std::size_t>(signalOf(static_cast<Config::EventTrigger>(index))));
            }
        }
    }
    m_bus.setWantedEverywhere(Render::SignalClient::Events, wanted);
}

void EventEffects::syncFocus(const std::optional<Layout::WindowId> &focused)
{
    const std::optional<Layout::WindowId> previous = m_focused;
    if (!focused) {
        const Known *lost = previous ? known(*previous) : nullptr;
        if (m_seeded && lost && !m_focusLost) {
            cancelPlays(Config::EventTrigger::Focus);
            cancelPlays(Config::EventTrigger::Unfocus);
            fire(Config::EventTrigger::Unfocus, lost->id);
        }
        m_focusLost = previous.has_value();
        return;
    }
    const bool changed = previous != focused;
    const bool unfocusPlayed = m_focusLost;
    m_focused = focused;
    m_focusLost = false;
    if (!m_seeded || !changed) {
        return;
    }
    cancelPlays(Config::EventTrigger::Focus);
    cancelPlays(Config::EventTrigger::Unfocus);
    const Known *now = known(*focused);
    const Known *old = previous ? known(*previous) : nullptr;
    const bool sameView = old && now && old->shown && old->output == now->output && old->workspace == now->workspace;
    const Pair pair {sameView ? old : nullptr, now, std::nullopt};
    if (now) {
        m_bus.fire(now->id, Render::Signal::Focus);
        start(Config::EventTrigger::Focus, *now, pair);
    }
    if (old && !unfocusPlayed) {
        m_bus.fire(old->id, Render::Signal::Unfocus);
        start(Config::EventTrigger::Unfocus, *old, pair);
    }
}

void EventEffects::syncUrgent()
{
    QSet<Layout::WindowId> urgent;
    for (const Known &window : m_known) {
        if (!window.urgent) {
            continue;
        }
        urgent.insert(window.id);
        if (m_seeded && !m_urgent.contains(window.id)) {
            fire(Config::EventTrigger::Urgent, window.id);
        }
    }
    m_urgent = std::move(urgent);
}

void EventEffects::cancelPlays(Config::EventTrigger trigger)
{
    std::erase_if(m_plays, [trigger](const Play &play) { return play.trigger == trigger; });
}

bool EventEffects::isBound(Config::EventTrigger trigger) const
{
    return std::ranges::any_of(m_known, [trigger](const Known &window) {
        const std::optional<Config::EventEffect> &effect = window.effects.of(trigger);
        return effect && effect->style != Config::kBorderStyleNone;
    });
}

void EventEffects::fire(Config::EventTrigger trigger, Layout::WindowId id, std::optional<QPointF> point, QPointF direction)
{
    const Known *subject = known(id);
    if (!subject) {
        return;
    }
    m_bus.fire(id, signalOf(trigger), point ? Render::SignalPoint {*point, direction, true} : Render::SignalPoint {});
    const Pair pair {nullptr, nullptr, point};
    const auto running = std::ranges::find_if(
        m_plays, [&](const Play &play) { return play.trigger == trigger && play.subject == id && play.mode == Config::EventMode::While; });
    if (running != m_plays.end()) {
        running->lastFired = seconds();
        return;
    }
    start(trigger, *subject, pair);
}

void EventEffects::fireOnOutput(Config::EventTrigger trigger, const QString &output)
{
    const auto onOutput = [&output](const Known &window) { return window.output == output && window.shown; };
    auto subject = std::ranges::find_if(m_known, [&onOutput](const Known &window) { return onOutput(window) && window.focused; });
    if (subject == m_known.end()) {
        subject = std::ranges::find_if(m_known, onOutput);
    }
    if (subject != m_known.end()) {
        fire(trigger, subject->id);
    }
}

}
