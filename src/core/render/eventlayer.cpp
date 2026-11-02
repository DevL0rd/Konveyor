#include "render/eventlayer.h"

#include <algorithm>
#include <cmath>

namespace Konveyor::Render
{

namespace
{

constexpr int kFirstEventShader = 100;
constexpr double kRippleSeconds = 0.8;
constexpr double kPulsePixelsPerSecond = 2200.0;
constexpr double kPulseLead = 70.0;
constexpr double kPulseTrail = 280.0;
constexpr double kPulseShortestSeconds = 0.55;
constexpr double kPulseFarthestDim = 0.5;
constexpr double kWavePixelsPerSecond = 2400.0;
constexpr double kWaveLead = 60.0;
constexpr double kWaveTrail = 320.0;
constexpr double kWaveShortestSeconds = 0.4;
constexpr double kWaveFadeIn = 60.0;
constexpr double kDrawInSeconds = 0.75;
constexpr double kDrawInTraceShare = 0.7;
constexpr double kDrawInHeadSharpness = 6.0;
constexpr double kFadeOutSeconds = 0.45;
constexpr double kDefaultStyleSeconds = 1.0;
constexpr double kStandingFade = 0.2;

struct WaveLine
{
    QPointF start;
    QPointF axis;
    double length = 0.0;
};

QPointF waveStart(const EventScene &scene, QPointF end)
{
    if (scene.from) {
        return scene.from->center();
    }
    if (scene.point && *scene.point != end) {
        return *scene.point;
    }
    return end - QPointF(scene.subject.width() / 2.0, 0.0);
}

WaveLine waveLine(const EventScene &scene)
{
    const QPointF end = scene.to ? scene.to->center() : scene.subject.center();
    const QPointF start = waveStart(scene, end);
    const QPointF delta = end - start;
    const double length = std::hypot(delta.x(), delta.y());
    return {start, length > 0.0 ? delta / length : QPointF(1.0, 0.0), length};
}

double waveTotal(const EventScene &scene)
{
    const WaveLine line = waveLine(scene);
    const QRectF &end = scene.to ? *scene.to : scene.subject;
    const double reach = std::abs(line.axis.x()) * end.width() / 2.0 + std::abs(line.axis.y()) * end.height() / 2.0;
    return line.length + reach + 2.0 * kWaveTrail;
}

double pulseTotal(const EventScene &scene)
{
    return scene.farthest + 2.0 * kPulseTrail;
}

QVector4D frontVector(QPointF origin, QPointF second, const QRectF &window)
{
    const QPointF relative = origin - window.center();
    return {
        static_cast<float>(relative.x()), static_cast<float>(relative.y()), static_cast<float>(second.x()), static_cast<float>(second.y())};
}

EventLayerFrame pulseFrontFrame(const EventScene &scene, const QRectF &window, double progress)
{
    const float fade = static_cast<float>(1.0 - kPulseFarthestDim * progress);
    const QVector4D params(kPulseLead, kPulseTrail, fade, static_cast<float>(progress));
    if (scene.point) {
        return {pulseTotal(scene) * progress, params, frontVector(*scene.point, QPointF(), window)};
    }
    const QPointF half(scene.subject.width() / 2.0, scene.subject.height() / 2.0);
    return {pulseTotal(scene) * progress, params, frontVector(scene.subject.center(), half, window)};
}

EventLayerFrame waveFrame(const EventScene &scene, const QRectF &window, double progress)
{
    const double total = waveTotal(scene);
    const double travelled = total * progress;
    const double fade = std::min({1.0, travelled / kWaveFadeIn, (total - travelled) / (2.0 * kWaveTrail)});
    const WaveLine line = waveLine(scene);
    return {travelled, QVector4D(kWaveLead, kWaveTrail, static_cast<float>(fade), static_cast<float>(progress)),
        frontVector(line.start, line.axis, window)};
}

EventLayerFrame drawInFrame(double progress)
{
    const double fade = progress < kDrawInTraceShare ? 1.0 : (1.0 - progress) / (1.0 - kDrawInTraceShare);
    return {progress / kDrawInTraceShare, QVector4D(kDrawInHeadSharpness, 0.f, static_cast<float>(fade), 0.f), {}};
}

}

bool isOneShotLayer(Config::BorderLayerKind kind)
{
    return kind == Config::BorderLayerKind::Ripple || isLightFrontLayer(kind);
}

bool isLightFrontLayer(Config::BorderLayerKind kind)
{
    return kind == Config::BorderLayerKind::PulseFront || kind == Config::BorderLayerKind::Wave || kind == Config::BorderLayerKind::DrawIn
        || kind == Config::BorderLayerKind::FadeOut;
}

int bandShaderKind(Config::BorderLayerKind kind)
{
    if (isLightFrontLayer(kind)) {
        return kFirstEventShader + static_cast<int>(kind) - static_cast<int>(Config::BorderLayerKind::PulseFront);
    }
    return static_cast<int>(kind);
}

double oneShotSeconds(const Config::BorderLayer &layer, const EventScene &scene)
{
    const double speed = std::abs(layer.speed);
    if (speed <= 0.0) {
        return 0.0;
    }
    switch (layer.kind) {
    case Config::BorderLayerKind::PulseFront:
        return std::max(kPulseShortestSeconds, pulseTotal(scene) / kPulsePixelsPerSecond) / speed;
    case Config::BorderLayerKind::Wave:
        return std::max(kWaveShortestSeconds, waveTotal(scene) / kWavePixelsPerSecond) / speed;
    case Config::BorderLayerKind::DrawIn:
        return kDrawInSeconds / speed;
    case Config::BorderLayerKind::FadeOut:
        return kFadeOutSeconds / speed;
    default:
        return kRippleSeconds / speed;
    }
}

std::optional<EventLayerFrame> oneShotFrame(const Config::BorderLayer &layer, const EventScene &scene, const QRectF &window, double seconds)
{
    const double duration = oneShotSeconds(layer, scene);
    if (duration <= 0.0 || seconds < 0.0 || seconds >= duration) {
        return std::nullopt;
    }
    const double progress = seconds / duration;
    switch (layer.kind) {
    case Config::BorderLayerKind::PulseFront:
        return pulseFrontFrame(scene, window, progress);
    case Config::BorderLayerKind::Wave:
        return waveFrame(scene, window, progress);
    case Config::BorderLayerKind::DrawIn:
        return drawInFrame(progress);
    default:
        return EventLayerFrame {progress, {}, {}};
    }
}

std::optional<EventLayerFrame> restingFrame(
    const Config::BorderLayer &layer, const EventScene &scene, const QRectF &window, double seconds, double restCycles)
{
    const double duration = oneShotSeconds(layer, scene);
    if (duration <= 0.0) {
        return std::nullopt;
    }
    return oneShotFrame(layer, scene, window, std::fmod(seconds, duration * restCycles));
}

double styleSeconds(const Config::BorderStyle &style, const EventScene &scene, std::optional<double> override)
{
    if (override) {
        return *override;
    }
    double longest = 0.0;
    for (const Config::BorderLayer &layer : style.layers) {
        if (isOneShotLayer(layer.kind)) {
            longest = std::max(longest, oneShotSeconds(layer, scene));
        }
    }
    return longest > 0.0 ? longest : kDefaultStyleSeconds;
}

double standingEnvelope(double seconds, double duration)
{
    return std::clamp(std::min(seconds, duration - seconds) / kStandingFade, 0.0, 1.0);
}

double distanceBetween(const QRectF &a, const QRectF &b)
{
    const double dx = std::max({a.left() - b.right(), b.left() - a.right(), 0.0});
    const double dy = std::max({a.top() - b.bottom(), b.top() - a.bottom(), 0.0});
    return std::hypot(dx, dy);
}

double farthestDistance(const QRectF &from, const QRectF &to)
{
    const double dx = std::max({from.left() - to.left(), to.right() - from.right(), 0.0});
    const double dy = std::max({from.top() - to.top(), to.bottom() - from.bottom(), 0.0});
    return std::hypot(dx, dy);
}

bool segmentCrosses(QPointF start, QPointF end, const QRectF &rect)
{
    double enter = 0.0;
    double leave = 1.0;
    const auto clip = [&](double from, double delta, double low, double high) {
        if (delta == 0.0) {
            return from >= low && from <= high;
        }
        const double a = (low - from) / delta;
        const double b = (high - from) / delta;
        enter = std::max(enter, std::min(a, b));
        leave = std::min(leave, std::max(a, b));
        return enter <= leave;
    };
    return clip(start.x(), end.x() - start.x(), rect.left(), rect.right())
        && clip(start.y(), end.y() - start.y(), rect.top(), rect.bottom());
}

}
