#include "anim/clock.h"

#include <algorithm>
#include <optional>
#include <utility>

namespace Konveyor::Anim
{

namespace
{

Duration scaled(Duration duration, double rate)
{
    return durationFromSeconds(toSeconds(duration) * rate);
}

}

struct Clock::State
{
    explicit State(TimeSource timeSource, std::optional<Duration> initialTime)
        : source(std::move(timeSource))
        , time(initialTime)
    {
        currentTime = unadjustedNow();
        lastSeenTime = currentTime;
    }

    Duration unadjustedNow()
    {
        if (!time) {
            time = source();
        }
        return *time;
    }

    Duration adjustedNow()
    {
        const Duration current = unadjustedNow();
        if (lastSeenTime == current) {
            return currentTime;
        }
        if (lastSeenTime < current) {
            currentTime = saturatingAdd(currentTime, scaled(current - lastSeenTime, rate));
        } else {
            currentTime = saturatingSub(currentTime, scaled(lastSeenTime - current, rate));
        }
        lastSeenTime = current;
        return currentTime;
    }

    TimeSource source;
    std::optional<Duration> time;
    Duration currentTime {};
    Duration lastSeenTime {};
    double rate = 1.0;
    bool completeInstantly = false;
};

Duration monotonicTime()
{
    return std::chrono::duration_cast<Duration>(std::chrono::steady_clock::now().time_since_epoch());
}

Clock::Clock()
    : Clock(monotonicTime)
{ }

Clock::Clock(TimeSource source)
    : m_state(std::make_shared<State>(std::move(source), std::nullopt))
{ }

Clock::Clock(std::shared_ptr<State> state)
    : m_state(std::move(state))
{ }

Clock Clock::frozenAt(Duration time)
{
    return Clock(std::make_shared<State>(monotonicTime, time));
}

Duration Clock::now() const
{
    return m_state->adjustedNow();
}

Duration Clock::rawNow() const
{
    return m_state->unadjustedNow();
}

void Clock::setRawNow(Duration time)
{
    m_state->time = time;
}

void Clock::clear()
{
    m_state->time.reset();
}

double Clock::rate() const
{
    return m_state->rate;
}

void Clock::setRate(double rate)
{
    m_state->rate = std::clamp(rate, 0.0, 1000.0);
}

bool Clock::skipsAnimations() const
{
    return m_state->completeInstantly;
}

void Clock::setSkipAnimations(bool value)
{
    m_state->completeInstantly = value;
}

void Clock::applyConfig(const Config::Animations &config)
{
    setRate(1.0 / std::max(config.slowdown, 0.001));
    setSkipAnimations(!config.enabled);
}

bool Clock::operator==(const Clock &other) const
{
    return m_state == other.m_state;
}

}
