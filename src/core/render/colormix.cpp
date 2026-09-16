#include "render/colormix.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace Konveyor::Render
{

namespace
{

using Vec3 = std::array<double, 3>;
using Mat3 = std::array<Vec3, 3>;

constexpr Mat3 rgbToLms {
    {{0.4122214708, 0.5363325363, 0.0514459929}, {0.2119034982, 0.6806995451, 0.1073969566}, {0.0883024619, 0.2817188376, 0.6299787005}}};
constexpr Mat3 lmsToOklab {{{0.2104542553, 0.7936177850, -0.0040720468}, {1.9779984951, -2.4285922050, 0.4505937099},
    {0.0259040371, 0.7827717662, -0.8086757660}}};
constexpr Mat3 oklabToLms {{{1.0, 0.3963377774, 0.2158037573}, {1.0, -0.1055613458, -0.0638541728}, {1.0, -0.0894841775, -1.2914855480}}};
constexpr Mat3 lmsToRgb {{{4.0767416621, -3.3077115913, 0.2309699292}, {-1.2684380046, 2.6097574011, -0.3413193965},
    {-0.0041960863, -0.7034186147, 1.7076147010}}};

Vec3 multiply(const Mat3 &matrix, const Vec3 &vector)
{
    Vec3 result {};
    for (size_t row = 0; row < 3; ++row) {
        result[row] = matrix[row][0] * vector[0] + matrix[row][1] * vector[1] + matrix[row][2] * vector[2];
    }
    return result;
}

Vec3 applyPower(const Vec3 &vector, double exponent)
{
    return {std::pow(vector[0], exponent), std::pow(vector[1], exponent), std::pow(vector[2], exponent)};
}

Vec3 rgbOf(const Rgba &color)
{
    return {color.r, color.g, color.b};
}

Rgba withRgb(const Vec3 &rgb, double alpha)
{
    return {rgb[0], rgb[1], rgb[2], alpha};
}

Vec3 linearToOklab(const Vec3 &linear)
{
    const Vec3 lms = multiply(rgbToLms, linear);
    return multiply(lmsToOklab, {std::cbrt(lms[0]), std::cbrt(lms[1]), std::cbrt(lms[2])});
}

Vec3 oklabToLinear(const Vec3 &lab)
{
    return multiply(lmsToRgb, applyPower(multiply(oklabToLms, lab), 3.0));
}

double degrees(double radians)
{
    return radians * 180.0 / std::numbers::pi;
}

double radians(double degreesValue)
{
    return degreesValue * std::numbers::pi / 180.0;
}

Vec3 labToLch(const Vec3 &lab)
{
    double hue = degrees(std::atan2(lab[2], lab[1]));
    if (hue <= 0.0) {
        hue += 360.0;
    }
    return {lab[0], std::hypot(lab[1], lab[2]), hue};
}

Vec3 lchToLab(const Vec3 &lch)
{
    return {lch[0], lch[1] * std::clamp(std::cos(radians(lch[2])), -1.0, 1.0), lch[1] * std::clamp(std::sin(radians(lch[2])), -1.0, 1.0)};
}

double lerp(double from, double to, double ratio)
{
    return from + (to - from) * ratio;
}

Rgba mixPremultiplied(const Rgba &from, const Rgba &to, double ratio, bool chromaOnly)
{
    const double hueScaleFrom = chromaOnly ? 1.0 : from.a;
    const double hueScaleTo = chromaOnly ? 1.0 : to.a;
    Rgba mixed {lerp(from.r * from.a, to.r * to.a, ratio), lerp(from.g * from.a, to.g * to.a, ratio),
        lerp(from.b * hueScaleFrom, to.b * hueScaleTo, ratio), lerp(from.a, to.a, ratio)};
    if (mixed.a == 0.0) {
        return mixed;
    }
    mixed.r /= mixed.a;
    mixed.g /= mixed.a;
    if (!chromaOnly) {
        mixed.b /= mixed.a;
    }
    return mixed;
}

double modulo(double value, double divisor)
{
    return value - divisor * std::floor(value / divisor);
}

double interpolateHue(double fromHue, double toHue, double ratio, Config::HueInterpolation mode)
{
    const double minHue = std::min(fromHue, toHue);
    const double maxHue = std::max(fromHue, toHue);
    const bool fromIsMin = fromHue == minHue;
    const double modDistance = (360.0 - maxHue + minHue) * ratio;
    const double directDistance = (maxHue - minHue) * ratio;
    const double pathMod = fromIsMin ? modulo(fromHue - modDistance, 360.0) : modulo(fromHue + modDistance, 360.0);
    const double pathDirect = fromIsMin ? fromHue + directDistance : fromHue - directDistance;
    const bool directIsLonger = maxHue - minHue > 360.0 - maxHue + minHue;
    switch (mode) {
    case Config::HueInterpolation::Shorter:
        return directIsLonger ? pathMod : pathDirect;
    case Config::HueInterpolation::Longer:
        return directIsLonger ? pathDirect : pathMod;
    case Config::HueInterpolation::Increasing:
        return fromHue > toHue ? pathMod : pathDirect;
    case Config::HueInterpolation::Decreasing:
        return fromHue <= toHue ? pathMod : pathDirect;
    }
    return pathDirect;
}

Rgba mixOklch(const Rgba &fromLinear, const Rgba &toLinear, double ratio, Config::HueInterpolation hue)
{
    const Vec3 fromLch = labToLch(linearToOklab(rgbOf(fromLinear)));
    const Vec3 toLch = labToLch(linearToOklab(rgbOf(toLinear)));
    Rgba mixed = mixPremultiplied(withRgb(fromLch, fromLinear.a), withRgb(toLch, toLinear.a), ratio, true);
    mixed.b = interpolateHue(fromLch[2], toLch[2], ratio, hue);
    Vec3 linear = oklabToLinear(lchToLab(rgbOf(mixed)));
    for (double &channel : linear) {
        channel = std::clamp(channel, 0.0, 1.0);
    }
    return withRgb(linear, mixed.a);
}

Rgba mixInLinearSpace(
    const Rgba &fromLinear, const Rgba &toLinear, double ratio, Config::GradientInterpolation space, Config::HueInterpolation hue)
{
    if (space == Config::GradientInterpolation::Oklch) {
        return mixOklch(fromLinear, toLinear, ratio, hue);
    }
    if (space == Config::GradientInterpolation::Oklab) {
        const Rgba mixed = mixPremultiplied(
            withRgb(linearToOklab(rgbOf(fromLinear)), fromLinear.a), withRgb(linearToOklab(rgbOf(toLinear)), toLinear.a), ratio, false);
        return withRgb(oklabToLinear(rgbOf(mixed)), mixed.a);
    }
    return mixPremultiplied(fromLinear, toLinear, ratio, false);
}

}

Rgba fromQColor(const QColor &color)
{
    return {color.redF(), color.greenF(), color.blueF(), color.alphaF()};
}

Rgba premultiplied(const Rgba &color)
{
    return {color.r * color.a, color.g * color.a, color.b * color.a, color.a};
}

QRgb toPremultipliedPixel(const Rgba &premultipliedColor)
{
    const auto channel = [](double value) { return static_cast<int>(std::lround(std::clamp(value, 0.0, 1.0) * 255.0)); };
    return qRgba(
        channel(premultipliedColor.r), channel(premultipliedColor.g), channel(premultipliedColor.b), channel(premultipliedColor.a));
}

Rgba mixColors(const Rgba &from, const Rgba &to, double ratio, Config::GradientInterpolation space, Config::HueInterpolation hue)
{
    if (space == Config::GradientInterpolation::Srgb) {
        return premultiplied(mixPremultiplied(from, to, ratio, false));
    }
    const auto toLinear = [](const Rgba &color) { return withRgb(applyPower(rgbOf(color), 2.2), color.a); };
    const Rgba mixed = mixInLinearSpace(toLinear(from), toLinear(to), ratio, space, hue);
    return premultiplied(withRgb(applyPower(rgbOf(mixed), 1.0 / 2.2), mixed.a));
}

}
