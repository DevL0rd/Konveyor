#pragma once

#include "render/borderlayer.h"
#include "render/signals.h"

#include <QVector4D>

#include <array>
#include <optional>
#include <vector>

namespace Konveyor::Render
{

struct LayerMotion
{
    Config::BorderLayer resolved;
    std::vector<float> smoothed;
    std::optional<std::array<quint32, kSignalCount>> fired;
    std::array<float, 16> bands {};
    std::array<float, 64> wave {};
    LayerBursts bursts;
    double lastSeconds = 0.0;
    double lastFire = -3600.0;
    double phase = 0.0;
    double clock = 0.0;
    float sign = 1.0F;
    float stretch = 0.0F;
    float stretchSpeed = 0.0F;
    std::optional<float> colorMix;
    bool started = false;
};

bool isReactive(const Config::BorderLayer &layer);
SignalSet signalsOf(const Config::BorderLayer &layer);
SignalSet signalsOf(const Config::BorderStyle &style);
float layerPresence(const Config::BorderLayer &layer, const SignalFrame &frame);
bool hiddenWhenStill(const Config::BorderLayer &layer);
bool layerRests(const Config::BorderLayer &layer, const SignalFrame &frame);

void advanceMotion(LayerMotion &motion, const Config::BorderLayer &layer, const SignalFrame &frame, double seconds, double decorationWidth);
void poseMotion(LayerMotion &motion, const Config::BorderLayer &layer, double seconds, double decorationWidth);

QVector4D motionUniforms(const LayerMotion &motion);
float colorMixUniform(const LayerMotion *motion);
float additiveUniform(Config::BorderBlend blend);

}
