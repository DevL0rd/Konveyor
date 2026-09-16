#include "anim/duration.h"

#include <cmath>

namespace Konveyor::Anim
{

double toSeconds(Duration duration)
{
    const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
    const auto nanos = duration - seconds;
    return static_cast<double>(seconds.count()) + static_cast<double>(nanos.count()) / 1e9;
}

Duration durationFromSeconds(double seconds)
{
    if (std::isnan(seconds) || seconds <= 0.0) {
        return Duration::zero();
    }
    if (seconds >= toSeconds(Duration::max())) {
        return Duration::max();
    }
    return std::chrono::round<Duration>(std::chrono::duration<double>(seconds));
}

Duration saturatingAdd(Duration lhs, Duration rhs)
{
    if (rhs > Duration::max() - lhs) {
        return Duration::max();
    }
    return lhs + rhs;
}

Duration saturatingSub(Duration lhs, Duration rhs)
{
    if (rhs >= lhs) {
        return Duration::zero();
    }
    return lhs - rhs;
}

}
