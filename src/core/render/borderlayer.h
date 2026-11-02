#pragma once

#include "config/types.h"

#include <QPointF>
#include <QSizeF>
#include <QVector2D>
#include <QVector4D>

#include <array>
#include <vector>

namespace Konveyor::Render
{

inline constexpr double kFrozenParticleSeconds = 600.0;

struct PerimeterPoint
{
    QPointF position;
    QPointF normal;
    QPointF tangent;
};

struct BorderLayerShape
{
    QSizeF size;
    Config::CornerRadius radius;
    double inset = 0.0;
    double width = 0.0;
};

struct BorderParticle
{
    QPointF position;
    double radius = 0.0;
    double ramp = 0.0;
    double alpha = 0.0;
};

struct LayerBursts
{
    static constexpr std::size_t kKept = 8;
    quint64 number = 0;
    std::array<double, kKept> times {};

    void add(double seconds)
    {
        ++number;
        times[number % kKept] = seconds;
    }
};

struct BorderVertex
{
    QVector2D position;
    QVector4D attributes;
};

PerimeterPoint perimeterPoint(QSizeF size, const Config::CornerRadius &radius, double fraction);

double layerWidth(const Config::BorderLayer &layer, double decorationWidth);
double layerOuterReach(const Config::BorderLayer &layer, double decorationWidth);

double layerPhase(const Config::BorderLayer &layer, double seconds);
double layerCyclesPerSecond(const Config::BorderLayer &layer);
int reactiveShaderKind(Config::BorderLayerKind kind);
bool layerBursts(const Config::BorderLayer &layer);
bool layerRunsContinuously(const Config::BorderLayer &layer);

QVector4D layerParams(const Config::BorderLayer &layer);

std::vector<BorderParticle> particlesAt(const Config::BorderLayer &layer, const BorderLayerShape &shape, double seconds);
std::vector<BorderParticle> burstsAt(
    const Config::BorderLayer &layer, const BorderLayerShape &shape, const LayerBursts &bursts, double seconds);

std::vector<BorderVertex> bandVertices(const BorderLayerShape &shape, double outerReach, double scale);
std::vector<BorderVertex> particleVertices(const std::vector<BorderParticle> &particles, double scale);

}
