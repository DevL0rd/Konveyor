#pragma once

#include "anim/duration.h"
#include "config/types.h"

#include <deque>
#include <optional>

namespace Konveyor::Layout
{

class VelocityTracker
{
public:
    void push(double delta, Anim::Duration timestamp);
    double pos() const { return m_pos; }
    double velocity() const;
    double projectedPosition() const;

private:
    struct Event
    {
        double delta;
        Anim::Duration timestamp;
    };

    std::deque<Event> m_history;
    double m_pos = 0.0;
};

struct EdgeScrollStep
{
    bool handled = false;
    bool scrolled = false;
    double position = 0;
};

EdgeScrollStep stepEdgeScroll(VelocityTracker &tracker, std::optional<Anim::Duration> &lastEventTime,
    std::optional<Anim::Duration> &nonzeroStartTime, Anim::Duration now, double delta, const Config::DndEdgeScroll &config);

struct ElasticLimit
{
    double stiffness;
    double limit;

    double band(double x) const;
    double derivative(double x) const;
    double clamp(double min, double max, double x) const;
    double clampSlope(double min, double max, double x) const;
};

}
