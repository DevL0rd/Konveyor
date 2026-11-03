#include "render/drive.h"

#include <algorithm>
#include <cmath>

namespace Konveyor::Render
{

namespace
{

constexpr double kLongestStep = 0.1;
constexpr double kStillPose = 0.27;
constexpr double kStillBurstAge = 0.3;
constexpr double kBlockRise = 0.05;
constexpr double kBlockFall = 0.5;
constexpr float kDirectionDeadZone = 0.001F;
constexpr float kStretchStiffness = 260.0F;
constexpr float kStretchDamping = 14.0F;
constexpr QLatin1StringView kColorMix("color-mix");

using Kind = Config::BorderLayerKind;

float curved(Config::DriveCurve curve, float value)
{
    const float sign = value < 0.0F ? -1.0F : 1.0F;
    switch (curve) {
    case Config::DriveCurve::Square:
        return value * value * sign;
    case Config::DriveCurve::Sqrt:
        return std::sqrt(std::abs(value)) * sign;
    case Config::DriveCurve::Invert:
        return 1.0F - value;
    default:
        return value;
    }
}

float following(float current, float target, double step, double seconds)
{
    return seconds > 0.0 ? current + (target - current) * static_cast<float>(1.0 - std::exp(-step / seconds)) : target;
}

void clampSettings(Config::BorderLayer &layer)
{
    layer.width = std::max(layer.width, 0.0);
    layer.opacity = std::clamp(layer.opacity, 0.0, 1.0);
    layer.tail = std::clamp(layer.tail, 0.0, 1.0);
    layer.gap = std::max(layer.gap, 0.0);
    layer.dash = std::max(layer.dash, 1.0);
    layer.size = std::max(layer.size, 0.1);
    layer.lifetime = std::max(layer.lifetime, 0.05);
    layer.rate = std::max(layer.rate, 0.01);
    layer.thickness = std::max(layer.thickness, 0.5);
    layer.baseline = std::clamp(layer.baseline, 0.0, 1.0);
    layer.count = std::max(layer.count, 1);
    layer.burst = std::max(layer.burst, 0);
}

bool drivesDirection(const Config::LayerDrive &drive)
{
    return drive.setting == QLatin1String("direction");
}

float directionOf(float smoothed, float previous)
{
    if (std::abs(smoothed) <= kDirectionDeadZone) {
        return previous;
    }
    return smoothed < 0.0F ? -1.0F : 1.0F;
}

void applyDrive(LayerMotion &motion, const Config::LayerDrive &drive, float smoothed)
{
    if (drive.amount == 0.0) {
        return;
    }
    if (drivesDirection(drive)) {
        motion.sign = directionOf(smoothed, motion.sign);
    } else if (drive.setting == kColorMix) {
        motion.colorMix = motion.colorMix.value_or(0.0F) + static_cast<float>(drive.amount) * curved(drive.curve, smoothed);
    } else {
        Config::addToDrivenSetting(motion.resolved, drive.setting, static_cast<float>(drive.amount) * curved(drive.curve, smoothed));
    }
}

void applyDirection(LayerMotion &motion, const Config::BorderLayer &layer)
{
    if (layer.kind != Kind::Particles) {
        if (motion.sign < 0.0F) {
            motion.resolved.speed = -motion.resolved.speed;
        }
    } else if (std::ranges::any_of(layer.drives, drivesDirection)) {
        motion.resolved.direction = motion.sign < 0.0F ? Config::ParticleDirection::Up : Config::ParticleDirection::Down;
    }
}

void applyDrives(LayerMotion &motion, const Config::BorderLayer &layer, const SignalFrame &frame, double step)
{
    motion.smoothed.resize(static_cast<std::size_t>(layer.drives.size()), 0.0F);
    std::size_t index = 0;
    for (const Config::LayerDrive &drive : layer.drives) {
        float &smoothed = motion.smoothed[index++];
        smoothed = motion.started ? following(smoothed, frame.value(drive.signal), step, drive.smooth) : frame.value(drive.signal);
        applyDrive(motion, drive, smoothed);
    }
    applyDirection(motion, layer);
}

void noteFirings(LayerMotion &motion, const Config::BorderLayer &layer, const SignalFrame &frame, double seconds)
{
    const bool fired = motion.fired && std::ranges::any_of(layer.fireOn, [&](Signal signal) {
        const auto index = static_cast<std::size_t>(signal);
        return frame.fired[index] > (*motion.fired)[index];
    });
    motion.fired = frame.fired;
    if (fired) {
        motion.lastFire = seconds;
        motion.bursts.add(seconds);
    }
}

void followBlocks(LayerMotion &motion, const Config::BorderLayer &layer, const SignalFrame &frame, double step)
{
    const double smoothing = motion.started ? layer.smoothing : 0.0;
    const auto gain = static_cast<float>(layer.sensitivity);
    for (std::size_t index = 0; index < frame.bands.size(); ++index) {
        const float target = std::min(frame.bands[index] * gain, 1.0F);
        const double seconds = (target > motion.bands[index] ? kBlockRise : kBlockFall) * smoothing;
        motion.bands[index] = following(motion.bands[index], target, step, seconds);
    }
    for (std::size_t index = 0; index < frame.wave.size(); ++index) {
        const float target = std::clamp(frame.wave[index] * gain, -1.0F, 1.0F);
        motion.wave[index] = following(motion.wave[index], target, step, kBlockRise * smoothing);
    }
}

bool readsBlocks(Kind kind)
{
    return kind == Kind::Equalizer || kind == Kind::Waveform || kind == Kind::Spectrum;
}

void followStretch(LayerMotion &motion, double step)
{
    const float target = static_cast<float>(motion.resolved.baseline) * motion.sign;
    if (!motion.started) {
        motion.stretch = target;
        return;
    }
    const float pull = kStretchStiffness * (target - motion.stretch) - kStretchDamping * motion.stretchSpeed;
    motion.stretchSpeed += pull * static_cast<float>(step);
    motion.stretch += motion.stretchSpeed * static_cast<float>(step);
}

SignalSet gatingSignals(const Config::BorderLayer &layer)
{
    SignalSet found;
    for (const Config::LayerDrive &drive : layer.drives) {
        if (drive.amount != 0.0 && drive.setting != kColorMix) {
            found.set(static_cast<std::size_t>(drive.signal));
        }
    }
    for (const Signal signal : layer.fireOn) {
        found.set(static_cast<std::size_t>(signal));
    }
    if (readsBlocks(layer.kind)) {
        found.set(static_cast<std::size_t>(Signal::AudioLevel));
    }
    return found;
}

bool mixesColor(const Config::LayerDrive &drive)
{
    return drive.amount != 0.0 && drive.setting == kColorMix;
}

}

bool isReactive(const Config::BorderLayer &layer)
{
    return !layer.drives.isEmpty() || !layer.fireOn.isEmpty() || layer.burst > 0 || reactiveShaderKind(layer.kind) >= 0;
}

SignalSet signalsOf(const Config::BorderLayer &layer)
{
    SignalSet found = gatingSignals(layer);
    for (const Config::LayerDrive &drive : layer.drives) {
        if (mixesColor(drive)) {
            found.set(static_cast<std::size_t>(drive.signal));
        }
    }
    if (Config::anyAudioSignal(found)) {
        found.set(static_cast<std::size_t>(Signal::AudioLevel));
    }
    return found;
}

SignalSet signalsOf(const Config::BorderStyle &style)
{
    SignalSet found;
    for (const Config::BorderLayer &layer : style.layers) {
        found |= signalsOf(layer);
    }
    return found;
}

float layerPresence(const Config::BorderLayer &layer, const SignalFrame &frame)
{
    const SignalSet gating = gatingSignals(layer);
    float presence = 0.0F;
    bool gated = false;
    for (std::size_t index = 0; index < kSignalCount; ++index) {
        const SignalSource source = Config::sourceOf(static_cast<Signal>(index));
        if (gating.test(index) && source != SignalSource::None) {
            gated = true;
            presence = std::max(presence, frame.presence(source));
        }
    }
    return gated ? presence : 1.0F;
}

bool hiddenWhenStill(const Config::BorderLayer &layer)
{
    const SignalSet gating = gatingSignals(layer);
    for (std::size_t index = 0; index < kSignalCount; ++index) {
        const SignalSource source = Config::sourceOf(static_cast<Signal>(index));
        if (gating.test(index) && source != SignalSource::Audio && source != SignalSource::None) {
            return true;
        }
    }
    return false;
}

bool layerRests(const Config::BorderLayer &layer, const SignalFrame &frame)
{
    if (gatingSignals(layer).any() || layer.speed != 0.0 || layer.kind == Kind::Particles || reactiveShaderKind(layer.kind) >= 0) {
        return false;
    }
    bool mixes = false;
    for (const Config::LayerDrive &drive : layer.drives) {
        const SignalSource source = Config::sourceOf(drive.signal);
        if (mixesColor(drive) && (source == SignalSource::None || frame.presence(source) > 0.0F)) {
            return false;
        }
        mixes = mixes || mixesColor(drive);
    }
    return mixes;
}

void advanceMotion(LayerMotion &motion, const Config::BorderLayer &layer, const SignalFrame &frame, double seconds, double decorationWidth)
{
    const double step = motion.started ? std::clamp(seconds - motion.lastSeconds, 0.0, kLongestStep) : 0.0;
    motion.lastSeconds = seconds;
    motion.resolved = layer;
    motion.resolved.drives.clear();
    motion.resolved.width = layerWidth(layer, decorationWidth);
    motion.colorMix.reset();
    applyDrives(motion, layer, frame, step);
    clampSettings(motion.resolved);
    motion.resolved.opacity *= layerPresence(layer, frame);
    if (layer.kind == Kind::Elastic) {
        followStretch(motion, step);
    }
    noteFirings(motion, layer, frame, seconds);
    if (readsBlocks(layer.kind)) {
        followBlocks(motion, layer, frame, step);
    }
    motion.phase += step * layerCyclesPerSecond(motion.resolved);
    motion.phase -= std::floor(motion.phase);
    motion.clock += step * motion.resolved.rate / layer.rate;
    motion.resolved.rate = layer.rate;
    motion.started = true;
}

void poseMotion(LayerMotion &motion, const Config::BorderLayer &layer, double seconds, double decorationWidth)
{
    motion = LayerMotion {};
    advanceMotion(motion, layer, simulatedSignals(kStillPose), seconds, decorationWidth);
    motion.clock = seconds;
    motion.lastFire = seconds - kStillBurstAge;
    motion.bursts.add(seconds - kStillBurstAge);
}

QVector4D motionUniforms(const LayerMotion &motion)
{
    const Config::BorderLayer &layer = motion.resolved;
    const float position = static_cast<float>(layer.position - std::floor(layer.position));
    const float level = layer.kind == Kind::Elastic ? motion.stretch : static_cast<float>(layer.baseline);
    return {level, position, motion.sign, static_cast<float>(layer.thickness)};
}

float colorMixUniform(const LayerMotion *motion)
{
    return motion && motion->colorMix ? std::clamp(*motion->colorMix, 0.0F, 1.0F) : -1.0F;
}

float additiveUniform(Config::BorderBlend blend)
{
    return blend == Config::BorderBlend::Add ? 1.0F : 0.0F;
}

}
