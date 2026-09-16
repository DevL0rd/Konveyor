#pragma once

#include "config/types.h"

#include <QImage>
#include <QRectF>

namespace Konveyor::Render
{

struct BorderSpec
{
    QSizeF size;
    double borderWidth = 0;
    Config::CornerRadius outerRadius;
    QColor color;
    std::optional<Config::Gradient> gradient;
    QRectF gradientRect;
    QPointF geometryOffset;
    double scale = 1.0;
    bool operator==(const BorderSpec &) const = default;
};

double cornerCoverage(QPointF coords, QSizeF size, const Config::CornerRadius &radius, double scale);

Config::CornerRadius shrinkRadius(const Config::CornerRadius &radius, double amount);

QImage renderBorder(const BorderSpec &spec);

}
