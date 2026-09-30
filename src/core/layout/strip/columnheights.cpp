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

double autoWeightOf(const std::vector<TileSizing> &data)
{
    double total = 0.0;
    for (const TileSizing &sizing : data) {
        total += sizing.height.isAuto() ? sizing.height.value : 0.0;
    }
    return total;
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

struct AutoBounds
{
    double lower = 0;
    double upper = 0;
};

double fillLevel(const std::vector<WindowHeight> &heights, const std::vector<AutoBounds> &bounds, double available)
{
    const auto filled = [&](double level) {
        double total = 0.0;
        for (std::size_t i = 0; i < heights.size(); ++i) {
            if (heights[i].isAuto()) {
                total += std::clamp(level * heights[i].value, bounds[i].lower, bounds[i].upper);
            }
        }
        return total;
    };
    double low = 0.0;
    double high = 0.0;
    for (std::size_t i = 0; i < heights.size(); ++i) {
        if (heights[i].isAuto() && heights[i].value > 0.0) {
            high = std::max(high, bounds[i].upper / heights[i].value);
        }
    }
    if (filled(high) < available) {
        return high * 2.0;
    }
    for (int step = 0; step < 64; ++step) {
        const double middle = (low + high) / 2.0;
        if (filled(middle) < available) {
            low = middle;
        } else {
            high = middle;
        }
    }
    return high;
}

double spaceLeft(const std::vector<WindowHeight> &heights, double available)
{
    for (const WindowHeight &height : heights) {
        available -= height.isAuto() ? 0.0 : height.value;
    }
    return available;
}

void holdAtBounds(
    std::vector<WindowHeight> &heights, const std::vector<QSizeF> &minSizes, const std::vector<QSizeF> &maxSizes, double available)
{
    std::vector<AutoBounds> bounds;
    bounds.reserve(heights.size());
    for (std::size_t i = 0; i < heights.size(); ++i) {
        const double lower = minSizes[i].height();
        bounds.push_back({lower, std::max(lower, maxSizes[i].height() > 0.0 ? maxSizes[i].height() : MaxPixelSize)});
    }
    const double level = fillLevel(heights, bounds, available);
    for (std::size_t i = 0; i < heights.size(); ++i) {
        const double share = level * heights[i].value;
        if (!heights[i].isAuto() || (share > bounds[i].lower && share < bounds[i].upper)) {
            continue;
        }
        heights[i] = WindowHeight::fixed(share <= bounds[i].lower ? bounds[i].lower : bounds[i].upper);
    }
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

std::optional<CarriedHeight> Column::heightToCarry(std::size_t idx) const
{
    if (isTabbed()) {
        return std::nullopt;
    }
    const WindowHeight &height = data[idx].height;
    return CarriedHeight {height, height.isAuto() ? height.value / autoWeightOf(data) : 1.0};
}

WindowHeight Column::insertedHeight(const Tile &tile)
{
    const std::optional<CarriedHeight> &carried = tile.carriedHeight;
    if (!carried || isTabbed()) {
        return WindowHeight::autoWeight(1.0);
    }
    const WindowHeight &height = carried->height;
    if (height.isAuto()) {
        const double total = autoWeightOf(data);
        if (carried->share >= 1.0 || total <= 0.0) {
            return WindowHeight::autoWeight(1.0);
        }
        return WindowHeight::autoWeight(carried->share * total / (1.0 - carried->share));
    }
    if (std::ranges::any_of(data, [](const TileSizing &sizing) { return !sizing.height.isAuto(); })) {
        resetHeightsToAuto();
    }
    if (height.kind == WindowHeight::Kind::Preset && std::cmp_greater_equal(height.preset, m_options->layout.presetWindowHeights.size())) {
        return WindowHeight::fixed(tile.windowSize().height());
    }
    return height;
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
        const double maxWidth = maxSizes[i].width() > 0.0 ? std::min(width, maxSizes[i].width()) : width;
        const double tileWidth = std::max(maxWidth, minSizes[i].width());
        tiles[i].requestOuterSize(QSizeF(tileWidth, heights[i].value), animate);
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
    const ColumnWidth width = (fillsWidth || expandedAlone) ? ColumnWidth::proportion(1.0) : widthSetting;
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
    const double available = m_area.workingArea.height() - gaps * static_cast<double>(tiles.size() + 1);
    holdAtBounds(heights, minSizes, maxSizes, spaceLeft(heights, available));

    double heightLeft = spaceLeft(heights, available);
    double totalWeight = totalAutoWeight(heights);
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
