#pragma once

#include "config/types.h"

#include <memory>
#include <optional>

namespace Konveyor::Layout
{

struct Options
{
    Config::Layout layout;
    Config::Animations animations;
    Config::Gestures gestures;
    bool operator==(const Options &) const = default;
};

using OptionsPtr = std::shared_ptr<const Options>;

Config::Layout normalizedLayout(const Config::Layout &layout);
Options optionsFromConfig(const Config::Config &config);
Options withLayoutOverride(Options options, const std::optional<Config::Layout> &part);
Options scaledFor(Options options, double scale);
OptionsPtr makeOptions(Options options);

}
