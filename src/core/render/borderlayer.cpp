#include "render/borderlayer.h"

#include "render/eventlayer.h"

#include <QRectF>
#include <QtMath>

#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

namespace Konveyor::Render
{

namespace
{

constexpr double kParticleVelocity = 40.0;
constexpr double kParticleMaxVelocityFactor = 1.4;
constexpr int kDefaultBurst = 16;
constexpr double kLightFrontWidth = 12.0;
constexpr double kElasticOvershoot = 1.3;

struct Segment
{
    double length = 0.0;
    QPointF origin;
    QPointF tangent;
    QPointF normal;
    double arcRadius = 0.0;
    double arcStart = 0.0;
};

std::array<Segment, 8> perimeterSegments(QSizeF size, const Config::CornerRadius &radius)
{
    const double w = size.width();
    const double h = size.height();
    const double tl = radius.topLeft;
    const double tr = radius.topRight;
    const double br = radius.bottomRight;
    const double bl = radius.bottomLeft;
    const auto arc = [](double r, QPointF center, double start) { return Segment {M_PI_2 * r, center, {}, {}, r, start}; };
    return {
        Segment {std::max(w - tl - tr, 0.0), {tl, 0}, {1, 0}, {0, -1}},
        arc(tr, {w - tr, tr}, -M_PI_2),
        Segment {std::max(h - tr - br, 0.0), {w, tr}, {0, 1}, {1, 0}},
        arc(br, {w - br, h - br}, 0.0),
        Segment {std::max(w - br - bl, 0.0), {w - br, h}, {-1, 0}, {0, 1}},
        arc(bl, {bl, h - bl}, M_PI_2),
        Segment {std::max(h - bl - tl, 0.0), {0, h - bl}, {0, -1}, {-1, 0}},
        arc(tl, {tl, tl}, M_PI),
    };
}

PerimeterPoint pointOnSegment(const Segment &segment, double distance)
{
    if (segment.arcRadius <= 0.0) {
        return {segment.origin + segment.tangent * distance, segment.normal, segment.tangent};
    }
    const double angle = segment.arcStart + distance / segment.arcRadius;
    const QPointF normal(std::cos(angle), std::sin(angle));
    return {segment.origin + normal * segment.arcRadius, normal, QPointF(-normal.y(), normal.x())};
}

double cycleSeconds(Config::BorderLayerKind kind)
{
    switch (kind) {
    case Config::BorderLayerKind::Gradient:
        return 8.0;
    case Config::BorderLayerKind::Sweep:
    case Config::BorderLayerKind::Comet:
        return 4.0;
    case Config::BorderLayerKind::Glow:
        return 3.0;
    case Config::BorderLayerKind::Pulse:
        return 1.5;
    case Config::BorderLayerKind::Dashes:
        return 0.5;
    case Config::BorderLayerKind::Ripple:
    case Config::BorderLayerKind::Shimmer:
        return 8.0;
    case Config::BorderLayerKind::SpeedLines:
        return 4.0;
    default:
        break;
    }
    return 1.0;
}

double hashed(quint64 seed)
{
    seed += 0x9e3779b97f4a7c15ULL;
    seed = (seed ^ (seed >> 30U)) * 0xbf58476d1ce4e5b9ULL;
    seed = (seed ^ (seed >> 27U)) * 0x94d049bb133111ebULL;
    seed ^= seed >> 31U;
    return static_cast<double>(seed >> 11U) / static_cast<double>(1ULL << 53U);
}

QPointF rotated(QPointF vector, double angle)
{
    const double c = std::cos(angle);
    const double s = std::sin(angle);
    return {vector.x() * c - vector.y() * s, vector.x() * s + vector.y() * c};
}

QPointF particleHeading(Config::ParticleDirection direction, const PerimeterPoint &origin, quint64 seed)
{
    const double jitter = (hashed(seed + 2) - 0.5);
    switch (direction) {
    case Config::ParticleDirection::Out:
        return rotated(origin.normal, jitter);
    case Config::ParticleDirection::Up:
        return rotated({0, -1}, jitter);
    case Config::ParticleDirection::Down:
        return rotated({0, 1}, jitter);
    case Config::ParticleDirection::Along:
        return rotated(origin.tangent, jitter * 0.3);
    case Config::ParticleDirection::Random:
        break;
    }
    return rotated({1, 0}, jitter * 2.0 * M_PI);
}

BorderParticle particleOf(const Config::BorderLayer &layer, const BorderLayerShape &shape, quint64 seed, double age)
{
    const PerimeterPoint origin = perimeterPoint(shape.size, shape.radius, hashed(seed));
    const double start = shape.inset + hashed(seed + 1) * shape.width;
    const double velocity = kParticleVelocity * layer.speed * (0.6 + (kParticleMaxVelocityFactor - 0.6) * hashed(seed + 3));
    const QPointF travel = particleHeading(layer.direction, origin, seed) * velocity * age;
    const double life = age / layer.lifetime;
    const double fadeIn = std::min(life * 8.0, 1.0);
    return {origin.position + origin.normal * start + travel, layer.size * (1.0 - 0.5 * life), life, fadeIn * (1.0 - life)};
}

double particleTravel(const Config::BorderLayer &layer)
{
    return std::abs(layer.speed) * kParticleVelocity * kParticleMaxVelocityFactor * layer.lifetime + layer.size;
}

void appendRect(std::vector<BorderVertex> &vertices, const QRectF &rect, QPointF center, double scale)
{
    if (rect.isEmpty()) {
        return;
    }
    const auto vertex = [&](QPointF corner) {
        const QPointF local = corner - center;
        return BorderVertex {QVector2D(corner * scale), QVector4D(static_cast<float>(local.x()), static_cast<float>(local.y()), 0.f, 1.f)};
    };
    const BorderVertex topLeft = vertex(rect.topLeft());
    const BorderVertex topRight = vertex(rect.topRight());
    const BorderVertex bottomRight = vertex(rect.bottomRight());
    const BorderVertex bottomLeft = vertex(rect.bottomLeft());
    vertices.insert(vertices.end(), {topLeft, topRight, bottomRight, bottomRight, bottomLeft, topLeft});
}

double largestRadius(const Config::CornerRadius &radius)
{
    return std::max({radius.topLeft, radius.topRight, radius.bottomRight, radius.bottomLeft});
}

}

PerimeterPoint perimeterPoint(QSizeF size, const Config::CornerRadius &radius, double fraction)
{
    const std::array<Segment, 8> segments = perimeterSegments(size, radius);
    double total = 0.0;
    for (const Segment &segment : segments) {
        total += segment.length;
    }
    double remaining = (fraction - std::floor(fraction)) * total;
    for (const Segment &segment : segments) {
        if (remaining <= segment.length && segment.length > 0.0) {
            return pointOnSegment(segment, remaining);
        }
        remaining -= segment.length;
    }
    return pointOnSegment(segments.front(), 0.0);
}

double layerWidth(const Config::BorderLayer &layer, double decorationWidth)
{
    if (layer.width > 0.0) {
        return layer.width;
    }
    return isLightFrontLayer(layer.kind) ? std::max(decorationWidth, kLightFrontWidth) : decorationWidth;
}

double layerOuterReach(const Config::BorderLayer &layer, double decorationWidth)
{
    if (!layer.drives.isEmpty()) {
        Config::BorderLayer peak = layer;
        peak.width = layerWidth(layer, decorationWidth);
        for (const Config::LayerDrive &drive : layer.drives) {
            Config::addToDrivenSetting(peak, drive.setting, std::max(drive.amount, 0.0));
        }
        peak.drives.clear();
        return layerOuterReach(peak, decorationWidth);
    }
    const double width = layerWidth(layer, decorationWidth);
    switch (layer.kind) {
    case Config::BorderLayerKind::Pulse:
        return width * 2.0 + 1.0;
    case Config::BorderLayerKind::Particles:
        return width + particleTravel(layer);
    case Config::BorderLayerKind::Ripple:
        return width * 3.0 + 1.0;
    case Config::BorderLayerKind::BassThump:
        return width * 2.0 + 1.0;
    case Config::BorderLayerKind::Wavy:
        return width + layer.thickness + 1.0;
    case Config::BorderLayerKind::Elastic:
        return width * kElasticOvershoot + layer.thickness + 1.0;
    default:
        return width + 1.0;
    }
}

double layerPhase(const Config::BorderLayer &layer, double seconds)
{
    const double cycles = seconds * layerCyclesPerSecond(layer);
    return cycles - std::floor(cycles);
}

double layerCyclesPerSecond(const Config::BorderLayer &layer)
{
    return layer.speed / cycleSeconds(layer.kind);
}

int reactiveShaderKind(Config::BorderLayerKind kind)
{
    switch (kind) {
    case Config::BorderLayerKind::Equalizer:
        return 0;
    case Config::BorderLayerKind::Waveform:
        return 1;
    case Config::BorderLayerKind::Spectrum:
        return 2;
    case Config::BorderLayerKind::Shimmer:
        return 3;
    case Config::BorderLayerKind::BassThump:
        return 4;
    case Config::BorderLayerKind::SpeedLines:
        return 5;
    case Config::BorderLayerKind::ScrollTrail:
        return 6;
    case Config::BorderLayerKind::Bulge:
        return 7;
    case Config::BorderLayerKind::Wavy:
        return 8;
    case Config::BorderLayerKind::Elastic:
        return 9;
    case Config::BorderLayerKind::Chevrons:
        return 10;
    case Config::BorderLayerKind::EdgeFlash:
        return 11;
    case Config::BorderLayerKind::Gauge:
        return 12;
    default:
        return -1;
    }
}

bool layerBursts(const Config::BorderLayer &layer)
{
    return layer.kind == Config::BorderLayerKind::Particles && (layer.burst > 0 || !layer.fireOn.isEmpty());
}

bool layerRunsContinuously(const Config::BorderLayer &layer)
{
    return layer.kind == Config::BorderLayerKind::Particles || layer.speed != 0.0 || !layer.drives.isEmpty() || !layer.fireOn.isEmpty()
        || reactiveShaderKind(layer.kind) >= 0;
}

QVector4D layerParams(const Config::BorderLayer &layer)
{
    switch (layer.kind) {
    case Config::BorderLayerKind::Gradient:
        return {static_cast<float>(qDegreesToRadians(layer.angle)), 0, 0, 0};
    case Config::BorderLayerKind::Comet:
        return {static_cast<float>(layer.count), static_cast<float>(layer.tail), layer.speed < 0.0 ? -1.0f : 1.0f, 0};
    case Config::BorderLayerKind::Equalizer:
        return {static_cast<float>(layer.count), static_cast<float>(layer.gap), 0, 0};
    case Config::BorderLayerKind::SpeedLines:
    case Config::BorderLayerKind::ScrollTrail:
    case Config::BorderLayerKind::Bulge:
    case Config::BorderLayerKind::Wavy:
    case Config::BorderLayerKind::Chevrons:
    case Config::BorderLayerKind::EdgeFlash:
    case Config::BorderLayerKind::Gauge:
        return {static_cast<float>(layer.count), static_cast<float>(layer.tail), 0, 0};
    case Config::BorderLayerKind::Dashes:
        return {static_cast<float>(layer.dash), static_cast<float>(layer.gap), 0, 0};
    default:
        return {};
    }
}

std::vector<BorderParticle> particlesAt(const Config::BorderLayer &layer, const BorderLayerShape &shape, double seconds)
{
    std::vector<BorderParticle> particles;
    particles.reserve(static_cast<std::size_t>(layer.count));
    const double period = std::max(layer.lifetime, layer.count / layer.rate);
    for (int slot = 0; slot < layer.count; ++slot) {
        const auto index = static_cast<quint64>(slot);
        const double cycle = (seconds + hashed(index * 131) * period) / period;
        const double generation = std::floor(cycle);
        const double age = (cycle - generation) * period;
        if (age < layer.lifetime) {
            particles.push_back(particleOf(layer, shape, index * 7919 + static_cast<quint64>(generation) * 4099, age));
        }
    }
    return particles;
}

std::vector<BorderParticle> burstsAt(
    const Config::BorderLayer &layer, const BorderLayerShape &shape, const LayerBursts &bursts, double seconds)
{
    std::vector<BorderParticle> particles;
    const auto limit = static_cast<std::size_t>(layer.count);
    const auto perBurst = static_cast<quint64>(layer.burst > 0 ? layer.burst : kDefaultBurst);
    particles.reserve(limit);
    for (std::size_t index = 0; index < LayerBursts::kKept && index < bursts.number; ++index) {
        const quint64 generation = bursts.number - index;
        const double age = seconds - bursts.times[generation % LayerBursts::kKept];
        if (age < 0.0 || age >= layer.lifetime) {
            continue;
        }
        for (quint64 slot = 0; slot < perBurst && particles.size() < limit; ++slot) {
            particles.push_back(particleOf(layer, shape, slot * 7919 + generation * 4099, age));
        }
    }
    return particles;
}

std::vector<BorderVertex> bandVertices(const BorderLayerShape &shape, double outerReach, double scale)
{
    const double w = shape.size.width();
    const double h = shape.size.height();
    const double outer = shape.inset + outerReach;
    const double inwardX = std::min(largestRadius(shape.radius), w / 2.0);
    const double inwardY = std::min(largestRadius(shape.radius), h / 2.0);
    const QPointF center(w / 2.0, h / 2.0);
    std::vector<BorderVertex> vertices;
    vertices.reserve(24);
    appendRect(vertices, QRectF(-outer, -outer, w + 2 * outer, outer + inwardY), center, scale);
    appendRect(vertices, QRectF(-outer, h - inwardY, w + 2 * outer, outer + inwardY), center, scale);
    appendRect(vertices, QRectF(-outer, inwardY, outer + inwardX, h - 2 * inwardY), center, scale);
    appendRect(vertices, QRectF(w - inwardX, inwardY, outer + inwardX, h - 2 * inwardY), center, scale);
    return vertices;
}

std::vector<BorderVertex> particleVertices(const std::vector<BorderParticle> &particles, double scale)
{
    std::vector<BorderVertex> vertices;
    vertices.reserve(particles.size() * 6);
    for (const BorderParticle &particle : particles) {
        const QVector2D center(particle.position * scale);
        const auto radius = static_cast<float>(particle.radius * scale);
        const auto ramp = static_cast<float>(particle.ramp);
        const auto alpha = static_cast<float>(particle.alpha);
        for (const auto &[u, v] : {std::pair(-1.f, -1.f), {1.f, -1.f}, {1.f, 1.f}, {1.f, 1.f}, {-1.f, 1.f}, {-1.f, -1.f}}) {
            vertices.push_back({center + QVector2D(u, v) * radius, QVector4D(u, v, ramp, alpha)});
        }
    }
    return vertices;
}

}
