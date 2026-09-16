#pragma once

#include "config/types.h"

namespace Konveyor::Render
{

struct Rgba
{
    double r = 0;
    double g = 0;
    double b = 0;
    double a = 0;
};

Rgba fromQColor(const QColor &color);
Rgba premultiplied(const Rgba &color);
QRgb toPremultipliedPixel(const Rgba &premultipliedColor);

Rgba mixColors(const Rgba &from, const Rgba &to, double ratio, Config::GradientInterpolation space, Config::HueInterpolation hue);

}
