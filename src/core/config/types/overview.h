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
    bool allowKontrolPanel = false;
    double scrollSpeed = 0.5;
    bool operator==(const Overview &) const = default;
};

}
