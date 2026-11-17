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
    // Touchpad sensitivity per axis; mouse wheel notches are unaffected.
    double touchpadScrollSpeedX = 1.0;
    double touchpadScrollSpeedY = 0.1;
    bool operator==(const Overview &) const = default;
};

}
