#include "layout/strip/columnstrip.h"

#include "layout/common/geometry.h"

#include <algorithm>
#include <cmath>

namespace Konveyor::Layout
{

namespace
{

constexpr double Tolerance = 1.0;

bool isNormal(const Column &column)
{
    return column.requestedMode() == WindowMode::Normal && column.sizingMode() == WindowMode::Normal
        && std::ranges::all_of(column.tiles, [](const Tile &tile) { return tile.window().requestedMode() == WindowMode::Normal; });
}

double columnMinWidth(const Column &column)
{
    double width = 0.0;
    for (const Tile &tile : column.tiles) {
        width = std::max(width, static_cast<double>(tile.window().minSize().width()));
    }
    return width;
}

QString tileLimitsError(const Tile &tile, double columnMinWidth)
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

QString stackedHeightsError(const Column &column)
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

QString columnError(const Column &column)
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

QString ColumnStrip::verifyArea() const
{
    if (!(m_area.viewSize.width() > 0.0) || !(m_area.viewSize.height() > 0.0)) {
        return QStringLiteral("scrolling: view size must be positive");
    }
    if (!(m_area.scale > 0.0) || !std::isfinite(m_area.scale)) {
        return QStringLiteral("scrolling: scale must be positive and finite");
    }
    if (m_area.workingArea != workAreaWithStruts(m_area.parentArea, m_area.scale, m_options->layout.struts)) {
        return QStringLiteral("scrolling: working area must match the struts");
    }
    return {};
}

QString ColumnStrip::verifyColumns() const
{
    int previousRank = 0;
    for (const Column &column : m_columns) {
        if (const QString error = columnError(column); !error.isEmpty()) {
            return error;
        }
        const std::optional<Config::ColumnPosition> pin = column.pinnedPosition();
        const int rank = pin == Config::ColumnPosition::Start ? 0 : (pin == Config::ColumnPosition::End ? 2 : 1);
        if (rank < previousRank) {
            return QStringLiteral("scrolling: column %1 is outside its pinned end of the row").arg(column.id());
        }
        previousRank = rank;
    }
    return {};
}

QString ColumnStrip::verifyView() const
{
    if (m_columns.empty() || centersActiveColumn() || m_scroll.isSwiping() || m_resize || !std::ranges::all_of(m_columns, isNormal)) {
        return {};
    }
    const double gaps = m_options->layout.gaps;
    const QRectF &area = m_area.workingArea;
    const double viewLeft = targetScrollPosition() + area.x();
    const double viewRight = viewLeft + area.width();
    const double rowRight = columnOffset(m_columns.size() - 1) + m_columns.back().width();
    const bool fits = rowRight + gaps * 2.0 <= area.width();
    const bool aligned
        = fits ? std::abs(viewLeft + gaps) <= Tolerance : viewLeft >= -gaps - Tolerance && viewRight <= rowRight + gaps + Tolerance;
    if (!aligned) {
        return QStringLiteral("scrolling: the view %1..%2 is past the row 0..%3").arg(viewLeft).arg(viewRight).arg(rowRight);
    }
    return {};
}

QString ColumnStrip::checkConsistency() const
{
    if (const QString error = verifyArea(); !error.isEmpty()) {
        return error;
    }
    if (m_resize && !hasWindow(m_resize->window)) {
        return QStringLiteral("scrolling: interactive resize window must be present");
    }
    if (m_columns.empty()) {
        return {};
    }
    if (m_activeColumnIndex >= m_columns.size()) {
        return QStringLiteral("scrolling: active column index out of range");
    }
    if (const QString error = verifyColumns(); !error.isEmpty()) {
        return error;
    }
    if (m_scrollToRestore && m_columns[m_activeColumnIndex].sizingMode() == WindowMode::Normal) {
        return QStringLiteral("scrolling: the active column must be fullscreen or maximized to restore a view offset");
    }
    return {};
}

}
