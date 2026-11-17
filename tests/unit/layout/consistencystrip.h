#pragma once

#include "layout/common/geometry.h"
#include "layout/strip/columnstrip.h"

#include <QString>

#include <algorithm>
#include <cmath>

namespace Konveyor::Layout
{

namespace StripChecks
{

inline constexpr double Tolerance = 1.0;

inline bool isNormal(const Column &column)
{
    return column.requestedMode() == WindowMode::Normal && column.sizingMode() == WindowMode::Normal
        && std::ranges::all_of(column.tiles, [](const Tile &tile) { return tile.window().requestedMode() == WindowMode::Normal; });
}

inline double columnMinWidth(const Column &column)
{
    double width = 0.0;
    for (const Tile &tile : column.tiles) {
        width = std::max(width, static_cast<double>(tile.window().minSize().width()));
    }
    return width;
}

inline QString tileLimitsError(const Tile &tile, double columnMinWidth)
{
    const QSizeF size = tile.targetWindowSize();
    const QSize min = tile.window().minSize();
    const QSize max = tile.window().maxSize();
    if (size.width() + Tolerance < min.width() || size.height() + Tolerance < min.height()) {
        return QStringLiteral("scrolling: window %1 is smaller than its minimum size").arg(tile.id());
    }
    const bool widthBounded = max.width() > 0 && columnMinWidth <= max.width();
    const bool heightBounded = max.height() > 0 && min.height() <= max.height();
    if ((widthBounded && size.width() > max.width() + Tolerance) || (heightBounded && size.height() > max.height() + Tolerance)) {
        return QStringLiteral("scrolling: window %1 is larger than its maximum size").arg(tile.id());
    }
    return {};
}

inline QString stackedHeightsError(const Column &column)
{
    if (column.isTabbed()) {
        return {};
    }
    const double gaps = column.options()->layout.gaps * static_cast<double>(column.tiles.size() + 1);
    double minimum = gaps;
    double total = gaps;
    bool anyAuto = false;
    for (std::size_t i = 0; i < column.tiles.size(); ++i) {
        const Tile &tile = column.tiles[i];
        minimum += std::max(tile.minNormalSize().height(), 1.0);
        total += tile.targetOuterSize().height();
        anyAuto = anyAuto || (column.data[i].height.isAuto() && tile.window().maxSize().height() <= 0);
    }
    const double available = column.area().workingArea.height();
    const double tolerance = Tolerance * static_cast<double>(column.tiles.size() + 1);
    if (anyAuto && minimum <= available && std::abs(total - available) > tolerance) {
        return QStringLiteral("scrolling: stacked heights plus gaps are %1 in a column %2 high").arg(total).arg(available);
    }
    return {};
}

inline QString columnError(const Column &column)
{
    if (column.tiles.empty()) {
        return QStringLiteral("scrolling: columns must not be empty");
    }
    if (column.activeTileIndex >= column.tiles.size()) {
        return QStringLiteral("scrolling: active tile index out of range");
    }
    if (column.data.size() != column.tiles.size()) {
        return QStringLiteral("scrolling: tile sizing data must match the tiles");
    }
    if (!isNormal(column)) {
        return {};
    }
    const double minWidth = columnMinWidth(column);
    for (const Tile &tile : column.tiles) {
        if (const QString error = tileLimitsError(tile, minWidth); !error.isEmpty()) {
            return error;
        }
    }
    return stackedHeightsError(column);
}

}

}
