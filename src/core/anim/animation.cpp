#include "anim/animation.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace Konveyor::Anim
{

namespace
{

template<class... Ts> struct Overloaded : Ts...
{
    using Ts::operator()...;
};

double scaledVelocity(const Clock &clock, double velocity)
{
    return velocity / std::max(clock.rate(), 0.001);
}

std::chrono::milliseconds configDuration(double durationMs)
{
    return std::chrono::milliseconds(std::llround(std::max(durationMs, 0.0)));
}

}

Animation::Animation(const Clock &clock, const Kind &kind, double from, double to, double initialVelocity)
    : m_from(from)
    , m_to(to)
    , m_initialVelocity(initialVelocity)
    , m_startTime(clock.now())
    , m_clock(clock)
    , m_kind(kind)
{ }

Animation::Animation(const Clock &clock, double from, double to, double initialVelocity, const Config::AnimationParams &config)
    : Animation(ease(clock, from, to, scaledVelocity(clock, initialVelocity), std::chrono::milliseconds::zero(),
          Curve(Config::EasingCurve::EaseOutCubic)))
{
    applyParams(config);
}

Animation Animation::ease(
    const Clock &clock, double from, double to, double initialVelocity, std::chrono::milliseconds duration, Curve curve)
{
    Animation animation(clock, Easing {std::move(curve)}, from, to, initialVelocity);
    animation.m_duration = duration;
    animation.m_timeToTarget = duration;
    return animation;
}

Animation Animation::spring(const Clock &clock, const Spring &spring)
{
    Animation animation(clock, spring, spring.from, spring.to, spring.initialVelocity);
    animation.m_duration = spring.duration();
    animation.m_timeToTarget = spring.timeToTarget().value_or(animation.m_duration);
    return animation;
}

Animation Animation::decelerate(const Clock &clock, double from, double initialVelocity, double decayRate, double threshold)
{
    const double coeff = 1000.0 * std::log(decayRate);
    const double durationSeconds = initialVelocity == 0.0 ? 0.0 : std::log(-coeff * threshold / std::abs(initialVelocity)) / coeff;
    const double to = from - initialVelocity / coeff;
    Animation animation(clock, DecayDriver {initialVelocity, decayRate}, from, to, initialVelocity);
    animation.m_duration = durationFromSeconds(durationSeconds);
    animation.m_timeToTarget = animation.m_duration;
    return animation;
}

void Animation::applyParams(const Config::AnimationParams &config)
{
    m_instant = !config.enabled;
    if (m_instant) {
        m_duration = Duration::zero();
        m_timeToTarget = Duration::zero();
        return;
    }

    const Duration startTime = m_startTime;
    *this = std::visit(Overloaded {
                           [this](const Config::SpringParams &params) {
                               const SpringParams springParams(params.dampingRatio, params.stiffness, params.epsilon);
                               return spring(m_clock, Spring {m_from, m_to, m_initialVelocity, springParams});
                           },
                           [this](const Config::EasingParams &params) {
                               return ease(
                                   m_clock, m_from, m_to, m_initialVelocity, configDuration(params.durationMs), Curve::fromConfig(params));
                           },
                       },
        config.kind);
    m_startTime = startTime;
}

Animation Animation::retargeted(double from, double to, double initialVelocity) const
{
    if (m_instant) {
        return *this;
    }

    const double velocity = scaledVelocity(m_clock, initialVelocity);
    return std::visit(Overloaded {
                          [&](const Easing &easing) {
                              const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(m_duration);
                              return ease(m_clock, from, to, velocity, duration, easing.curve);
                          },
                          [&](const Spring &current) { return spring(m_clock, Spring {from, to, m_initialVelocity, current.params}); },
                          [&](const DecayDriver &deceleration) {
                              return decelerate(m_clock, from, deceleration.initialVelocity, deceleration.decayRate, 0.001);
                          },
                      },
        m_kind);
}

bool Animation::reached(Duration span) const
{
    return m_clock.skipsAnimations() || m_clock.now() >= saturatingAdd(m_startTime, span);
}

bool Animation::isFinished() const
{
    return reached(m_duration);
}

bool Animation::hasReachedTarget() const
{
    return reached(m_timeToTarget);
}

double Animation::easedValue(const Easing &easing, Duration passed) const
{
    const double x = std::clamp(toSeconds(passed) / toSeconds(m_duration), 0.0, 1.0);
    return easing.curve.y(x) * (m_to - m_from) + m_from;
}

double Animation::springValue(const Spring &spring, Duration passed) const
{
    const double value = spring.valueAt(passed);
    const double range = (m_to - m_from) * 10.0;
    const double a = m_from - range;
    const double b = m_to + range;
    return m_from <= m_to ? std::clamp(value, a, b) : std::clamp(value, b, a);
}

double Animation::deceleratedValue(const DecayDriver &deceleration, Duration passed) const
{
    const double coeff = 1000.0 * std::log(deceleration.decayRate);
    return m_from + (std::pow(deceleration.decayRate, 1000.0 * toSeconds(passed)) - 1.0) / coeff * deceleration.initialVelocity;
}

double Animation::valueAt(Duration at) const
{
    if (at <= m_startTime) {
        return m_from;
    }
    if (saturatingAdd(m_startTime, m_duration) <= at || m_clock.skipsAnimations()) {
        return m_to;
    }

    const Duration passed = at - m_startTime;
    return std::visit(Overloaded {
                          [&](const Easing &easing) { return easedValue(easing, passed); },
                          [&](const Spring &spring) { return springValue(spring, passed); },
                          [&](const DecayDriver &deceleration) { return deceleratedValue(deceleration, passed); },
                      },
        m_kind);
}

double Animation::value() const
{
    return valueAt(m_clock.now());
}

double Animation::valueWithoutOvershoot() const
{
    if (hasReachedTarget()) {
        return m_to;
    }
    return value();
}

double Animation::to() const
{
    return m_to;
}

double Animation::from() const
{
    return m_from;
}

Duration Animation::startTime() const
{
    return m_startTime;
}

Duration Animation::endTime() const
{
    return saturatingAdd(m_startTime, m_duration);
}

Duration Animation::duration() const
{
    return m_duration;
}

void Animation::offset(double offset)
{
    m_from += offset;
    m_to += offset;
    if (auto *spring = std::get_if<Spring>(&m_kind)) {
        spring->from += offset;
        spring->to += offset;
    }
}

}
