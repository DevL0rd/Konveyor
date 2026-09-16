#pragma once

#include "anim/duration.h"

#include <optional>

namespace Konveyor::Anim
{

struct SpringParams
{
    SpringParams(double dampingRatio, double stiffness, double epsilon);

    double damping;
    double mass;
    double stiffness;
    double epsilon;
};

struct Spring
{
    double from;
    double to;
    double initialVelocity;
    SpringParams params;

    double valueAt(Duration t) const;
    double velocityAt(Duration t) const;
    Duration duration() const;
    std::optional<Duration> timeToTarget() const;
};

}
