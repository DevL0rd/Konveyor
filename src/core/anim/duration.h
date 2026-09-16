#pragma once

#include <chrono>

namespace Konveyor::Anim
{

using Duration = std::chrono::nanoseconds;

double toSeconds(Duration duration);
Duration durationFromSeconds(double seconds);
Duration saturatingAdd(Duration lhs, Duration rhs);
Duration saturatingSub(Duration lhs, Duration rhs);

}
