#pragma once

namespace Konveyor::Config
{

enum class OverviewIndicators
{
    Chevrons,
    Off,
};

struct Overview
{
    bool enabled = false;
    double minimumScale = 0.3;
    double cardGap = 0.08;
    OverviewIndicators offscreenIndicators = OverviewIndicators::Chevrons;
    bool operator==(const Overview &) const = default;
};

}
