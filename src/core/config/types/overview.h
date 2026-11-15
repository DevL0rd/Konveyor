#pragma once

namespace Konveyor::Config
{

struct Overview
{
    bool enabled = false;
    double minimumScale = 0.3;
    double cardGap = 0.08;
    bool operator==(const Overview &) const = default;
};

}
