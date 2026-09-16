#include "layout/common/velocitytracker.h"

#include <algorithm>
#include <chrono>
#include <cmath>

namespace Konveyor::Layout
{

namespace
{

constexpr auto SampleWindow = std::chrono::milliseconds(150);
constexpr double TouchpadDeceleration = 0.997;

}

void VelocityTracker::push(double delta, Anim::Duration timestamp)
{
    if (!m_history.empty() && timestamp < m_history.back().timestamp) {
        return;
    }
    m_history.push_back({delta, timestamp});
    m_pos += delta;
    while (!m_history.empty() && timestamp > m_history.front().timestamp + SampleWindow) {
        m_history.pop_front();
    }
}

double VelocityTracker::velocity() const
{
    if (m_history.empty()) {
        return 0.0;
    }
    const double totalTime = Anim::toSeconds(m_history.back().timestamp - m_history.front().timestamp);
    if (totalTime == 0.0) {
        return 0.0;
    }
    double totalDelta = 0.0;
    for (const Event &event : m_history) {
        totalDelta += event.delta;
    }
    return totalDelta / totalTime;
}

double VelocityTracker::projectedPosition() const
{
    return m_pos - velocity() / (1000.0 * std::log(TouchpadDeceleration));
}

EdgeScrollStep stepEdgeScroll(VelocityTracker &tracker, std::optional<Anim::Duration> &lastEventTime,
    std::optional<Anim::Duration> &nonzeroStartTime, Anim::Duration now, double delta, const Config::DndEdgeScroll &config)
{
    const Anim::Duration last = lastEventTime.value_or(now);
    lastEventTime = now;
    if (delta == 0.0) {
        nonzeroStartTime.reset();
        return {};
    }
    if (!nonzeroStartTime) {
        nonzeroStartTime = now;
    }
    if (Anim::toSeconds(Anim::saturatingSub(now, *nonzeroStartTime)) * 1000.0 < config.delayMs) {
        return {true, false, tracker.pos()};
    }
    tracker.push(delta * Anim::toSeconds(Anim::saturatingSub(now, last)) * config.maxSpeed, now);
    return {true, true, tracker.pos()};
}

double ElasticLimit::band(double x) const
{
    return (1.0 - (1.0 / (x * stiffness / limit + 1.0))) * limit;
}

double ElasticLimit::derivative(double x) const
{
    const double denom = stiffness * x + limit;
    return stiffness * limit * limit / (denom * denom);
}

double ElasticLimit::clamp(double min, double max, double x) const
{
    const double clamped = std::clamp(x, min, max);
    const double sign = x < clamped ? -1.0 : 1.0;
    return clamped + sign * band(std::abs(x - clamped));
}

double ElasticLimit::clampSlope(double min, double max, double x) const
{
    if (min <= x && x <= max) {
        return 1.0;
    }
    return derivative(std::abs(x - std::clamp(x, min, max)));
}

}
