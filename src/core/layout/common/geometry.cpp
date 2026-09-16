#include "layout/common/geometry.h"

#include <algorithm>
#include <climits>
#include <cmath>
#include <variant>

namespace Konveyor::Layout
{

ColumnWidth ColumnWidth::fromPreset(const Config::PresetSize &preset)
{
    if (const auto *prop = std::get_if<Config::Proportion>(&preset)) {
        return proportion(std::clamp(prop->value, 0.0, 10000.0));
    }
    return fixed(std::clamp(std::round(std::get<Config::Fixed>(preset).value), 1.0, 100000.0));
}

bool resolveActivation(Activation activate, bool smart)
{
    switch (activate) {
    case Activation::Always:
        return true;
    case Activation::Smart:
        return smart;
    case Activation::Never:
        break;
    }
    return false;
}

SizeChange sizeChangeFromPreset(const Config::PresetSize &preset)
{
    if (const auto *proportion = std::get_if<Config::Proportion>(&preset)) {
        return {ChangeKind::SetProportion, proportion->value * 100.0};
    }
    return {ChangeKind::SetFixed, std::round(std::get<Config::Fixed>(preset).value)};
}

double snapToPixels(double scale, double logical)
{
    return std::round(logical * scale) / scale;
}

double snapToPixelsAtLeastOne(double scale, double logical)
{
    if (logical == 0.0) {
        return 0.0;
    }
    return std::round(std::max(logical * scale, 1.0)) / scale;
}

double floorToPixelsAtLeastOne(double scale, double logical)
{
    if (logical == 0.0) {
        return 0.0;
    }
    return std::floor(std::max(logical * scale, 1.0)) / scale;
}

double ceilToPixels(double scale, double logical)
{
    return std::ceil(logical * scale) / scale;
}

QPointF roundPoint(QPointF point, double scale)
{
    return {snapToPixels(scale, point.x()), snapToPixels(scale, point.y())};
}

QSizeF roundSize(QSizeF size, double scale)
{
    return {snapToPixels(scale, size.width()), snapToPixels(scale, size.height())};
}

int saturatingInt(double value)
{
    if (std::isnan(value)) {
        return 0;
    }
    return static_cast<int>(std::clamp(value, static_cast<double>(INT_MIN), static_cast<double>(INT_MAX)));
}

int floorToInt(double value)
{
    return saturatingInt(std::floor(value));
}

int roundToInt(double value)
{
    return saturatingInt(std::round(value));
}

QSize floorSize(QSizeF size)
{
    return {floorToInt(size.width()), floorToInt(size.height())};
}

QSize nonNegativeSize(QSizeF size)
{
    return {std::max(0, roundToInt(size.width())), std::max(0, roundToInt(size.height()))};
}

QSizeF clampedNonNegative(QSizeF size)
{
    return {std::max(0.0, size.width()), std::max(0.0, size.height())};
}

QSize roundedSize(QSizeF size)
{
    return {roundToInt(size.width()), roundToInt(size.height())};
}

QRectF workAreaWithStruts(QRectF parentArea, double scale, const Config::Struts &struts)
{
    double x = parentArea.x() + struts.left;
    double y = parentArea.y() + struts.top;
    double w = std::max(0.0, parentArea.width() - struts.left - struts.right);
    double h = std::max(0.0, parentArea.height() - struts.top - struts.bottom);

    const double locX = ceilToPixels(scale, x);
    const double locY = ceilToPixels(scale, y);
    w -= std::min(w, locX - x);
    h -= std::min(h, locY - y);
    return {locX, locY, w, h};
}

double scrollToReveal(double curX, double viewWidth, double newColX, double newColWidth, double gaps)
{
    if (viewWidth <= newColWidth) {
        return 0.0;
    }

    const double padding = std::clamp((viewWidth - newColWidth) / 2.0, 0.0, gaps);
    const double newX = newColX - padding;
    const double newRightX = newColX + newColWidth + padding;

    if (curX <= newX && newRightX <= curX + viewWidth) {
        return -(newColX - curX);
    }

    const double distToLeft = std::abs(curX - newX);
    const double distToRight = std::abs((curX + viewWidth) - newRightX);
    if (distToLeft <= distToRight) {
        return -padding;
    }
    return -(viewWidth - padding - newColWidth);
}

int clampToSizeLimits(int x, int minSize, int maxSize)
{
    if (maxSize > 0) {
        x = std::min(x, maxSize);
    }
    if (minSize > 0) {
        x = std::max(x, minSize);
    }
    return x;
}

int clampToSizeLimitsAllowZero(int x, int minSize, int maxSize)
{
    if (x != 0) {
        return clampToSizeLimits(x, minSize, maxSize);
    }
    if (minSize > 0 && minSize == maxSize) {
        return minSize;
    }
    return 0;
}

QPointF clampIntoArea(QRectF area, QRectF rect)
{
    double x = std::min(rect.x(), area.x() + area.width() - rect.width());
    double y = std::min(rect.y(), area.y() + area.height() - rect.height());
    x = std::max(x, area.x());
    y = std::max(y, area.y());
    return {x, y};
}

QPointF centerInArea(QRectF area, QSizeF size)
{
    const double dx = std::max((area.width() - size.width()) / 2.0, 0.0);
    const double dy = std::max((area.height() - size.height()) / 2.0, 0.0);
    return {area.x() + dx, area.y() + dy};
}

PresetExtent measurePreset(const Config::PresetSize &preset, const Options &options, double viewSize, double reservedSize)
{
    if (const auto *proportion = std::get_if<Config::Proportion>(&preset)) {
        const double gaps = options.layout.gaps;
        return {true, (viewSize - gaps) * proportion->value - gaps - reservedSize};
    }
    return {false, std::round(std::get<Config::Fixed>(preset).value)};
}

PresetExtent measureFloatingPreset(const Config::PresetSize &preset, double viewSize)
{
    if (const auto *proportion = std::get_if<Config::Proportion>(&preset)) {
        return {true, viewSize * proportion->value};
    }
    return {false, std::round(std::get<Config::Fixed>(preset).value)};
}

double borderExtent(const Config::Border &border)
{
    return border.enabled ? border.width * 2.0 : 0.0;
}

QSize maxWindowBounds(const Config::Border &border, QSizeF workingAreaSize, QSizeF reservedSize, double gaps)
{
    const double extent = borderExtent(border);
    const double w = std::max(workingAreaSize.width() - gaps * 2.0 - reservedSize.width() - extent, 1.0);
    const double h = std::max(workingAreaSize.height() - gaps * 2.0 - reservedSize.height() - extent, 1.0);
    return {floorToInt(w), floorToInt(h)};
}

}
