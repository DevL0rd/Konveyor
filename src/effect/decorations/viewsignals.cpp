#include "decorations/viewsignals.h"

#include <algorithm>
#include <cmath>

namespace Konveyor
{

namespace
{

constexpr double kFullSpeedPixelsPerSecond = 4000.0;
constexpr double kSettleSeconds = 0.12;
constexpr double kMovementPixels = 0.5;

}

ViewSignals::ViewSignals(Render::SignalBus &bus, EventEffects &events)
    : m_bus(bus)
    , m_events(events)
{
    m_clock.start();
}

double ViewSignals::seconds() const
{
    return static_cast<double>(m_clock.nsecsElapsed()) / 1e9;
}

bool ViewSignals::isWanted() const
{
    const Render::SignalSet wanted = m_bus.wantedAnywhere();
    return m_events.isBound(Config::EventTrigger::View) || wanted[static_cast<std::size_t>(Render::Signal::ViewSpeed)]
        || wanted[static_cast<std::size_t>(Render::Signal::ViewDirection)] || wanted[static_cast<std::size_t>(Render::Signal::ViewDistance)]
        || wanted[static_cast<std::size_t>(Render::Signal::ViewStep)];
}

void ViewSignals::publish(const QString &output, const Tracked &tracked)
{
    for (const Layout::WindowId id : std::as_const(m_windows[output])) {
        m_bus.set(id, Render::Signal::ViewSpeed, static_cast<float>(tracked.speed));
        m_bus.set(id, Render::Signal::ViewDirection, static_cast<float>(tracked.direction));
        m_bus.set(id, Render::Signal::ViewDistance, static_cast<float>(tracked.distance));
    }
}

void ViewSignals::follow(const Layout::OutputViewState &view, Tracked &tracked, double now)
{
    const double delta = view.offset - tracked.offset;
    const double step = now - tracked.seconds;
    if (std::abs(delta) >= kMovementPixels && step > 0.0) {
        tracked.speed = std::min(std::abs(delta) / step / kFullSpeedPixelsPerSecond, 1.0);
        tracked.direction = delta > 0.0 ? 1.0 : -1.0;
        const double width = std::max(view.width, 1.0);
        tracked.distance = std::fmod(tracked.distance + std::abs(delta) / width, 1.0);
        tracked.offset = view.offset;
        tracked.seconds = now;
        publish(view.output, tracked);
        m_events.fireOnOutput(Config::EventTrigger::View, view.output);
    }
    if (tracked.column != view.activeColumn) {
        tracked.column = view.activeColumn;
        for (const Layout::WindowId id : std::as_const(m_windows[view.output])) {
            m_bus.fire(id, Render::Signal::ViewStep);
        }
    }
}

void ViewSignals::update(const QList<Layout::OutputViewState> &views, const QList<Layout::WindowState> &states)
{
    m_windows.clear();
    for (const Layout::WindowState &state : states) {
        m_windows[state.output].append(state.id);
    }
    const double now = seconds();
    for (const Layout::OutputViewState &view : views) {
        const auto found = m_outputs.find(view.output);
        if (found == m_outputs.end() || found->workspace != view.workspace) {
            m_outputs.insert(view.output, {view.workspace, view.offset, now, 0.0, 0.0, 0.0, view.activeColumn});
        } else {
            follow(view, *found, now);
        }
    }
}

void ViewSignals::settle()
{
    const double now = seconds();
    for (auto it = m_outputs.begin(); it != m_outputs.end(); ++it) {
        if (it->speed > 0.0 && now - it->seconds >= kSettleSeconds) {
            it->speed = 0.0;
            publish(it.key(), *it);
        }
    }
}

}
