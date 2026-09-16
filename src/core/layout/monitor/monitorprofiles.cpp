#include "layout/monitor/monitorprofiles.h"

#include <algorithm>

namespace Konveyor::Layout
{

namespace
{

bool monitorMatches(const Config::MonitorMatch &match, const QString &name, QSizeF size)
{
    if (match.name && !match.name->match(name).hasMatch()) {
        return false;
    }
    const double aspect = size.height() > 0 ? size.width() / size.height() : 0.0;
    const auto above = [](const std::optional<double> &limit, double value) { return !limit || value > *limit; };
    const auto below = [](const std::optional<double> &limit, double value) { return !limit || value < *limit; };
    return above(match.aspectRatioAbove, aspect) && below(match.aspectRatioBelow, aspect) && above(match.widthAbove, size.width())
        && below(match.widthBelow, size.width()) && above(match.heightAbove, size.height()) && below(match.heightBelow, size.height());
}

}

const Config::MonitorProfile *monitorProfileFor(const Config::Config &config, const QString &name, QSizeF size)
{
    for (const Config::MonitorProfile &profile : config.monitorProfiles) {
        const bool matches = profile.matches.isEmpty()
            || std::ranges::any_of(profile.matches, [&](const Config::MonitorMatch &match) { return monitorMatches(match, name, size); });
        if (matches) {
            return &profile;
        }
    }
    return nullptr;
}

}
