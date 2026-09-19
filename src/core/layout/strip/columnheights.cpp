#include "layout/strip/column.h"

#include "layout/common/sizelimits.h"

#include "layout/common/geometry.h"

#include <algorithm>
#include <climits>
#include <cmath>

namespace Konveyor::Layout
{

namespace
{

double autoHeightFor(const Tile &tile, double height)
{
    return tile.outerHeightFor(std::max(std::round(tile.innerHeightFor(height)), 1.0));
}

std::optional<std::size_t> nonAutoIndex(const std::vector<TileSizing> &data)
{
    const auto it = std::ranges::find_if(data, [](const TileSizing &d) { return !d.height.isAuto(); });
    if (it == data.end()) {
        return std::nullopt;
    }
    return static_cast<std::size_t>(std::distance(data.begin(), it));
}

double totalAutoWeight(const std::vector<WindowHeight> &heights)
{
    double total = 0.0;
    for (const WindowHeight &h : heights) {
        if (h.isAuto()) {
            total += h.value;
        }
    }
    return total;
}

void applyExactConstraints(std::vector<WindowHeight> &heights, const std::vector<QSizeF> &minSizes, const std::vector<QSizeF> &maxSizes)
{
    for (std::size_t i = 0; i < heights.size(); ++i) {
        if (minSizes[i].height() == maxSizes[i].height()) {
            heights[i] = WindowHeight::fixed(minSizes[i].height());
        }
        if (heights[i].isAuto()) {
            continue;
        }
        double h = heights[i].value;
        if (maxSizes[i].height() > 0.0) {
            h = std::min(h, maxSizes[i].height());
        }
        heights[i].value = std::max(h, minSizes[i].height());
    }
}

}

void Column::layoutTiles(bool animate)
{
    const WindowMode mode = requestedMode();
    const bool forceExpansion = mode == WindowMode::Maximized || (mode == WindowMode::Normal && fillsWidth);
    for (Tile &tile : tiles) {
        tile.window().setExpansionForceResizable(forceExpansion);
    }
    if (mode != WindowMode::Normal) {
        requestExpandedSizes(mode, animate);
        return;
    }

    std::vector<QSizeF> minSizes;
    std::vector<QSizeF> maxSizes;
    for (const Tile &tile : tiles) {
        const QSizeF minSize = tile.minNormalSize();
        minSizes.emplace_back(std::max(minSize.width(), 1.0), std::max(minSize.height(), 1.0));
        maxSizes.push_back(tile.maxNormalSize());
    }

    const double width = columnTileWidth();
    const double maxTileHeight = m_area.workingArea.height() - m_options->layout.gaps * 2.0 - reservedSize().height();
    std::vector<WindowHeight> heights = fixedHeights(maxFixedWindowHeight(minSizes, maxTileHeight), maxTileHeight);

    if (isTabbed()) {
        const auto fixedIt = std::ranges::find_if(heights, [](const WindowHeight &h) { return !h.isAuto(); });
        double tabbedHeight = fixedIt != heights.end() ? fixedIt->value : maxTileHeight;
        double minHeight = 0.0;
        for (const QSizeF &size : minSizes) {
            minHeight = std::max(minHeight, size.height());
        }
        tabbedHeight = std::max(tabbedHeight, std::min(maxTileHeight, minHeight));
        std::ranges::fill(heights, WindowHeight::fixed(tabbedHeight));
    }

    distributeHeights(heights, minSizes, maxSizes);

    for (std::size_t i = 0; i < tiles.size(); ++i) {
        tiles[i].requestOuterSize(QSizeF(width, heights[i].value), animate);
    }
}

void Column::requestExpandedSizes(WindowMode mode, bool animate)
{
    for (Tile &tile : tiles) {
        if (mode == WindowMode::Fullscreen) {
            tile.requestFullscreen(animate);
        } else {
            tile.requestMaximized(m_area.parentArea.size(), animate);
        }
    }
}

double Column::columnTileWidth() const
{
    double minWidth = 1.0;
    double maxWidth = static_cast<double>(INT_MAX);
    for (const Tile &tile : tiles) {
        minWidth = std::max(minWidth, std::max(tile.minNormalSize().width(), 1.0));
        const double w = tile.maxNormalSize().width();
        if (w != 0.0) {
            maxWidth = std::min(maxWidth, w);
        }
    }
    maxWidth = std::max(maxWidth, minWidth);
    const ColumnWidth width = fillsWidth ? ColumnWidth::proportion(1.0) : widthSetting;
    return std::max(std::min(widthInPixels(width), maxWidth), minWidth);
}

std::optional<double> Column::maxFixedWindowHeight(const std::vector<QSizeF> &minSizes, double maxTileHeight) const
{
    if (tiles.size() <= 1 || isTabbed()) {
        return std::nullopt;
    }
    const auto idx = nonAutoIndex(data);
    if (!idx) {
        return std::nullopt;
    }
    double minHeightTaken = 0.0;
    for (std::size_t i = 0; i < minSizes.size(); ++i) {
        if (i != *idx) {
            minHeightTaken += minSizes[i].height() + m_options->layout.gaps;
        }
    }
    const double left = maxTileHeight - minHeightTaken;
    return std::max(1.0, std::round(tiles[*idx].innerHeightFor(left)));
}

std::vector<WindowHeight> Column::fixedHeights(std::optional<double> maxNonAuto, double maxTileHeight) const
{
    std::vector<WindowHeight> heights;
    heights.reserve(tiles.size());
    for (std::size_t i = 0; i < tiles.size(); ++i) {
        const Tile &tile = tiles[i];
        const WindowHeight &height = data[i].height;
        if (height.isAuto()) {
            heights.push_back(height);
            continue;
        }
        double windowHeight = 0.0;
        if (height.kind == WindowHeight::Kind::Fixed) {
            windowHeight = std::max(std::round(height.value), 1.0);
            const double limit = maxNonAuto ? *maxNonAuto : std::round(tile.innerHeightFor(maxTileHeight));
            windowHeight = std::min(windowHeight, limit);
        } else {
            const PresetExtent resolved = presetHeightExtent(m_options->layout.presetWindowHeights[height.preset]);
            windowHeight = resolved.isTile ? tile.innerHeightFor(resolved.value) : resolved.value;
            windowHeight = std::clamp(std::round(windowHeight), 1.0, MaxPixelSize);
            windowHeight = maxNonAuto ? std::min(windowHeight, *maxNonAuto) : windowHeight;
        }
        heights.push_back(WindowHeight::fixed(tile.outerHeightFor(windowHeight)));
    }
    return heights;
}

void Column::distributeHeights(
    std::vector<WindowHeight> &heights, const std::vector<QSizeF> &minSizes, const std::vector<QSizeF> &maxSizes) const
{
    applyExactConstraints(heights, minSizes, maxSizes);
    const double gaps = m_options->layout.gaps;
    double heightLeft = m_area.workingArea.height() - gaps * static_cast<double>(tiles.size() + 1);
    for (const WindowHeight &h : heights) {
        heightLeft -= h.isAuto() ? 0.0 : h.value;
    }
    double totalWeight = totalAutoWeight(heights);

    const auto findUnsatisfied = [&]() -> std::optional<std::size_t> {
        double left = heightLeft;
        double weightLeft = totalWeight;
        for (std::size_t i = 0; i < heights.size(); ++i) {
            if (!heights[i].isAuto()) {
                continue;
            }
            const double autoHeight = left * (heights[i].value / weightLeft);
            if (minSizes[i].height() > autoHeight) {
                return i;
            }
            left -= autoHeightFor(tiles[i], autoHeight);
            weightLeft -= heights[i].value;
        }
        return std::nullopt;
    };

    while (const auto unsatisfied = findUnsatisfied()) {
        const std::size_t i = *unsatisfied;
        totalWeight -= heights[i].value;
        heights[i] = WindowHeight::fixed(minSizes[i].height());
        heightLeft -= minSizes[i].height();
    }

    for (std::size_t i = 0; i < heights.size(); ++i) {
        if (!heights[i].isAuto()) {
            continue;
        }
        const double weight = heights[i].value;
        const double autoHeight = autoHeightFor(tiles[i], heightLeft * (weight / totalWeight));
        heights[i] = WindowHeight::fixed(autoHeight);
        heightLeft -= autoHeight;
        totalWeight -= weight;
    }
}

}
