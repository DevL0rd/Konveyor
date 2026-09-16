#include "anim/spring.h"

#include <algorithm>
#include <cfloat>
#include <cmath>

namespace Konveyor::Anim
{

namespace
{

enum class Regime
{
    CriticallyDamped,
    Underdamped,
    Overdamped
};

struct Motion
{
    double beta;
    double omega;
    double x0;
    double v0;
    Regime regime;
};

double betaOf(const Spring &spring)
{
    return spring.params.damping / (2.0 * spring.params.mass);
}

Motion motionOf(const Spring &spring)
{
    const double beta = betaOf(spring);
    const double omega0 = std::sqrt(spring.params.stiffness / spring.params.mass);
    Motion motion {beta, 0.0, spring.from - spring.to, spring.initialVelocity, Regime::CriticallyDamped};
    if (std::abs(beta - omega0) <= static_cast<double>(FLT_EPSILON)) {
        return motion;
    }
    if (beta < omega0) {
        motion.regime = Regime::Underdamped;
        motion.omega = std::sqrt((omega0 * omega0) - (beta * beta));
    } else {
        motion.regime = Regime::Overdamped;
        motion.omega = std::sqrt((beta * beta) - (omega0 * omega0));
    }
    return motion;
}

struct Sample
{
    double position;
    double velocity;
};

Sample sample(const Spring &spring, double t)
{
    const Motion m = motionOf(spring);
    const double envelope = std::exp(-m.beta * t);
    const double b = (m.beta * m.x0 + m.v0);
    if (m.regime == Regime::CriticallyDamped) {
        return {spring.to + envelope * (m.x0 + b * t), envelope * (m.v0 - m.beta * b * t)};
    }
    const bool under = m.regime == Regime::Underdamped;
    const double even = under ? std::cos(m.omega * t) : std::cosh(m.omega * t);
    const double odd = under ? std::sin(m.omega * t) : std::sinh(m.omega * t);
    const double sign = under ? -1.0 : 1.0;
    const double position = spring.to + envelope * (m.x0 * even + (b / m.omega) * odd);
    const double velocity = envelope * (m.v0 * even + (sign * m.x0 * m.omega - m.beta * b / m.omega) * odd);
    return {position, velocity};
}

double oscillate(const Spring &spring, double t)
{
    return sample(spring, t).position;
}

std::optional<Duration> trivialDuration(const Spring &spring)
{
    const double beta = betaOf(spring);
    if (std::abs(beta) <= DBL_EPSILON || beta < 0.0) {
        return Duration::max();
    }
    if (std::abs(spring.to - spring.from) <= DBL_EPSILON) {
        return Duration::zero();
    }
    return std::nullopt;
}

double newtonStep(const Spring &spring, double x)
{
    constexpr double delta = 0.001;
    const double y = oscillate(spring, x);
    const double slope = (oscillate(spring, x + delta) - y) / delta;
    return (spring.to - y + slope * x) / slope;
}

Duration overdampedDuration(const Spring &spring, double x0)
{
    double x1 = newtonStep(spring, x0);
    double y1 = oscillate(spring, x1);
    int iteration = 0;
    while (std::abs(spring.to - y1) > spring.params.epsilon) {
        if (iteration > 1000) {
            return Duration::zero();
        }
        x0 = x1;
        x1 = newtonStep(spring, x0);
        y1 = oscillate(spring, x1);
        if (!std::isfinite(y1)) {
            return durationFromSeconds(x0);
        }
        ++iteration;
    }
    return durationFromSeconds(x1);
}

bool beforeTarget(const Spring &spring, double y)
{
    const double epsilon = spring.params.epsilon;
    return (spring.to - spring.from > DBL_EPSILON && spring.to - y > epsilon)
        || (spring.from - spring.to > DBL_EPSILON && y - spring.to > epsilon);
}

}

SpringParams::SpringParams(double dampingRatio, double stiffness, double epsilon)
    : mass(1.0)
    , stiffness(std::max(stiffness, 0.0))
    , epsilon(std::max(epsilon, 0.0))
{
    const double criticalDamping = 2.0 * std::sqrt(mass * this->stiffness);
    damping = std::max(dampingRatio, 0.0) * criticalDamping;
}

double Spring::valueAt(Duration t) const
{
    return oscillate(*this, toSeconds(t));
}

double Spring::velocityAt(Duration t) const
{
    return sample(*this, toSeconds(t)).velocity;
}

Duration Spring::duration() const
{
    if (const auto trivial = trivialDuration(*this)) {
        return *trivial;
    }
    const Motion motion = motionOf(*this);
    const double envelopeTime = -std::log(params.epsilon) / motion.beta;
    if (motion.regime != Regime::Overdamped) {
        return durationFromSeconds(envelopeTime);
    }
    return overdampedDuration(*this, envelopeTime);
}

std::optional<Duration> Spring::timeToTarget() const
{
    if (const auto trivial = trivialDuration(*this)) {
        return trivial;
    }
    int millis = 1;
    double y = oscillate(*this, static_cast<double>(millis) / 1000.0);
    while (beforeTarget(*this, y)) {
        if (millis > 3000) {
            return std::nullopt;
        }
        ++millis;
        y = oscillate(*this, static_cast<double>(millis) / 1000.0);
    }
    return std::chrono::milliseconds(millis);
}

}
