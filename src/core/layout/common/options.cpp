#include "layout/common/options.h"

#include "layout/common/geometry.h"

namespace Konveyor::Layout
{

namespace
{

QList<Config::PresetSize> defaultPresets()
{
    return {Config::Proportion {1.0 / 3.0}, Config::Proportion {0.5}, Config::Proportion {2.0 / 3.0}};
}

}

Config::Layout normalizedLayout(const Config::Layout &layout)
{
    Config::Layout result = layout;
    if (result.presetColumnWidths.isEmpty()) {
        result.presetColumnWidths = defaultPresets();
    }
    if (result.presetWindowHeights.isEmpty()) {
        result.presetWindowHeights = defaultPresets();
    }
    return result;
}

Options optionsFromConfig(const Config::Config &config)
{
    Options options;
    options.layout = normalizedLayout(config.layout);
    options.animations = config.animations;
    options.gestures = config.gestures;
    options.overview = config.overview;
    return options;
}

Options withLayoutOverride(Options options, const std::optional<Config::Layout> &part)
{
    if (part) {
        options.layout = normalizedLayout(*part);
    }
    return options;
}

Options scaledFor(Options options, double scale)
{
    options.layout.gaps = snapToPixelsAtLeastOne(scale, options.layout.gaps);
    return options;
}

OptionsPtr makeOptions(Options options)
{
    return std::make_shared<const Options>(std::move(options));
}

}
