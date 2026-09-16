#pragma once

#include "config/types.h"
#include "kdl/kdl.h"

#include <QColor>
#include <QString>

#include <optional>

namespace Konveyor::Config
{

std::optional<QColor> parseCssColor(const QString &text);

QColor decodeColorNode(const Kdl::Node &node);
Paint decodePaintNode(const Kdl::Node &node);
Gradient decodeGradientNode(const Kdl::Node &node);

void applyPaintColor(std::optional<Paint> &paint, const Paint &value);
void applyPaintGradient(std::optional<Paint> &paint, const Gradient &gradient);
void mergePaint(Paint &base, const std::optional<Paint> &part);
void mergeOptionalPaint(std::optional<Paint> &base, const std::optional<Paint> &part);

}
