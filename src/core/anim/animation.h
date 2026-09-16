#pragma once

#include "anim/clock.h"
#include "anim/duration.h"
#include "anim/easing.h"
#include "anim/spring.h"
#include "config/types.h"

#include <chrono>
#include <variant>

namespace Konveyor::Anim
{

class Animation
{
public:
    Animation(const Clock &clock, double from, double to, double initialVelocity, const Config::AnimationParams &config);

    static Animation ease(
        const Clock &clock, double from, double to, double initialVelocity, std::chrono::milliseconds duration, Curve curve);
    static Animation spring(const Clock &clock, const Spring &spring);
    static Animation decelerate(const Clock &clock, double from, double initialVelocity, double decayRate, double threshold);

    void applyParams(const Config::AnimationParams &config);
    Animation retargeted(double from, double to, double initialVelocity) const;

    bool isFinished() const;
    bool hasReachedTarget() const;

    double valueAt(Duration at) const;
    double value() const;
    double valueWithoutOvershoot() const;

    double to() const;
    double from() const;
    Duration startTime() const;
    Duration endTime() const;
    Duration duration() const;

    void offset(double offset);

private:
    struct Easing
    {
        Curve curve;
    };

    struct DecayDriver
    {
        double initialVelocity;
        double decayRate;
    };

    using Kind = std::variant<Easing, Spring, DecayDriver>;

    Animation(const Clock &clock, const Kind &kind, double from, double to, double initialVelocity);

    bool reached(Duration span) const;
    double easedValue(const Easing &easing, Duration passed) const;
    double springValue(const Spring &spring, Duration passed) const;
    double deceleratedValue(const DecayDriver &deceleration, Duration passed) const;

    double m_from;
    double m_to;
    double m_initialVelocity;
    bool m_instant = false;
    Duration m_duration {};
    Duration m_timeToTarget {};
    Duration m_startTime;
    Clock m_clock;
    Kind m_kind;
};

}
