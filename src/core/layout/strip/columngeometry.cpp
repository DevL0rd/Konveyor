#include "layout/strip/column.h"

#include "layout/common/geometry.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <utility>

namespace Konveyor::Layout
{

double Column::width() const
{
    double tilesWidth = 0.0;
    for (const Tile &tile : tiles) {
        tilesWidth = std::max(tilesWidth, tile.pendingOuterSize().width());
    }
    if (isTabbed() && sizingMode() == WindowMode::Normal) {
        tilesWidth += tabBar.reservedSize(tiles.size(), m_area.scale).width();
    }
    return tilesWidth;
}

QSizeF Column::reservedSize() const
{
    if (isTabbed()) {
        return tabBar.reservedSize(tiles.size(), m_area.scale);
    }
    return {0.0, 0.0};
}

PresetExtent Column::presetWidthExtent(const Config::PresetSize &preset) const
{
    return measurePreset(preset, *m_options, m_area.workingArea.width(), reservedSize().width());
}

PresetExtent Column::presetHeightExtent(const Config::PresetSize &preset) const
{
    return measurePreset(preset, *m_options, m_area.workingArea.height(), reservedSize().height());
}

double Column::widthInPixels(ColumnWidth width) const
{
    if (!width.isProportion) {
        return width.value;
    }
    const double gaps = m_options->layout.gaps;
    return (m_area.workingArea.width() - gaps) * width.value - gaps - reservedSize().width();
}

void Column::resetHeightsToAuto()
{
    std::vector<double> heights;
    heights.reserve(tiles.size());
    for (const Tile &tile : tiles) {
        heights.push_back(tile.outerSize().height());
    }
    std::vector<double> sorted = heights;
    std::ranges::sort(sorted);
    const double median = sorted[sorted.size() / 2];
    for (std::size_t i = 0; i < data.size(); ++i) {
        data[i].height = WindowHeight::autoWeight(heights[i] / median);
    }
}

QPointF Column::tileAreaOrigin() const
{
    switch (sizingMode()) {
    case WindowMode::Maximized:
        return {0.0, m_area.parentArea.y()};
    case WindowMode::Fullscreen:
        return {0.0, 0.0};
    case WindowMode::Normal:
        break;
    }
    QPointF origin(0.0, m_area.workingArea.y() + m_options->layout.gaps);
    if (isTabbed()) {
        origin += tabBar.contentOffset(tiles.size(), m_area.scale);
    }
    return origin;
}

std::vector<QPointF> Column::tilePositions() const
{
    const bool center = m_options->layout.centerFocusedColumn == Config::CenterFocusedColumn::Always;
    const double gaps = m_options->layout.gaps;
    const double tilesWidth = width();

    std::vector<QPointF> offsets;
    offsets.reserve(tiles.size() + 1);
    QPointF origin = tileAreaOrigin();
    const auto push = [&](QSizeF size, bool resizingByLeftEdge) {
        QPointF pos = origin;
        if (center) {
            pos.rx() += (tilesWidth - size.width()) / 2.0;
        } else if (resizingByLeftEdge) {
            pos.rx() += tilesWidth - size.width();
        }
        if (!isTabbed()) {
            origin.ry() += size.height() + gaps;
        }
        offsets.push_back(pos);
    };
    for (std::size_t i = 0; i < tiles.size(); ++i) {
        push(tiles[i].pendingOuterSize(), data[i].resizingFromLeft);
    }
    push(QSizeF(), false);
    return offsets;
}

QPointF Column::tilePosition(std::size_t idx) const
{
    return tilePositions()[idx];
}

QRectF Column::tabBarRect() const
{
    double maxHeight = 0.0;
    for (const Tile &tile : tiles) {
        maxHeight = std::max(maxHeight, tile.outerSize().height());
    }
    const QSizeF size(tiles[activeTileIndex].visibleOuterSize().width(), maxHeight);
    return {tileAreaOrigin(), size};
}

std::vector<std::size_t> Column::renderOrder() const
{
    std::vector<std::size_t> order;
    order.reserve(tiles.size());
    order.push_back(activeTileIndex);
    for (std::size_t i = 0; i < tiles.size(); ++i) {
        if (i != activeTileIndex) {
            order.push_back(i);
        }
    }
    return order;
}

void Column::focusTileAt(std::size_t index)
{
    selectTile(std::min(index > 0 ? index - 1 : 0, tiles.size() - 1));
}

bool Column::focusUp()
{
    return selectTile(activeTileIndex > 0 ? activeTileIndex - 1 : 0);
}

bool Column::focusDown()
{
    return selectTile(std::min(activeTileIndex + 1, tiles.size() - 1));
}

void Column::focusTop()
{
    selectTile(0);
}

void Column::focusBottom()
{
    selectTile(tiles.size() - 1);
}

void Column::swapActive(std::size_t newIndex)
{
    std::swap(tiles[activeTileIndex], tiles[newIndex]);
    std::swap(data[activeTileIndex], data[newIndex]);
    activeTileIndex = newIndex;
}

bool Column::moveUp()
{
    if (activeTileIndex == 0) {
        return false;
    }
    const std::vector<QPointF> offsets = tilePositions();
    const double activeY = offsets[activeTileIndex].y();
    const double nextY = offsets[activeTileIndex + 1].y();
    const std::size_t newIndex = activeTileIndex - 1;
    swapActive(newIndex);
    const double newActiveY = tilePosition(newIndex).y();
    tiles[newIndex].slideYFrom(activeY - newActiveY);
    tiles[newIndex + 1].slideYFrom(activeY - nextY);
    return true;
}

bool Column::moveDown()
{
    if (activeTileIndex + 1 >= tiles.size()) {
        return false;
    }
    const std::vector<QPointF> offsets = tilePositions();
    const double activeY = offsets[activeTileIndex].y();
    const double nextY = offsets[activeTileIndex + 1].y();
    const std::size_t newIndex = activeTileIndex + 1;
    swapActive(newIndex);
    const double newActiveY = tilePosition(newIndex).y();
    tiles[newIndex].slideYFrom(activeY - newActiveY);
    tiles[newIndex - 1].slideYFrom(nextY - activeY);
    return true;
}

}
