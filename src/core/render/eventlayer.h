#pragma once

#include "config/types.h"

#include <QPointF>
#include <QRectF>
#include <QVector4D>

#include <optional>

namespace Konveyor::Render
{

struct EventScene
{
    QRectF subject;
    std::optional<QRectF> from;
    std::optional<QRectF> to;
    std::optional<QPointF> point;
    double farthest = 0.0;
};

struct EventLayerFrame
{
    double phase = 0.0;
    QVector4D params;
    QVector4D front;
};

bool isOneShotLayer(Config::BorderLayerKind kind);
bool isLightFrontLayer(Config::BorderLayerKind kind);
int bandShaderKind(Config::BorderLayerKind kind);
double oneShotSeconds(const Config::BorderLayer &layer, const EventScene &scene);
std::optional<EventLayerFrame> oneShotFrame(
    const Config::BorderLayer &layer, const EventScene &scene, const QRectF &window, double seconds);
std::optional<EventLayerFrame> restingFrame(
    const Config::BorderLayer &layer, const EventScene &scene, const QRectF &window, double seconds, double restCycles);
double styleSeconds(const Config::BorderStyle &style, const EventScene &scene, std::optional<double> override);
double standingEnvelope(double seconds, double duration);
double farthestDistance(const QRectF &from, const QRectF &to);
double distanceBetween(const QRectF &a, const QRectF &b);
bool segmentCrosses(QPointF start, QPointF end, const QRectF &rect);

}
