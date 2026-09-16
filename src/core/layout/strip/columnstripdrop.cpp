#include "layout/strip/columnstrip.h"

#include "layout/common/geometry.h"

#include <algorithm>
#include <cmath>

namespace Konveyor::Layout
{

namespace
{

constexpr double HintThickness = 150.0;
constexpr double HintColumnWidth = 300.0;

DropSlot newColumnAt(std::size_t idx)
{
    return {DropSlot::Kind::NewColumn, idx, 0};
}

std::size_t closestIndex(const std::vector<double> &values, double target)
{
    std::size_t best = 0;
    for (std::size_t i = 1; i < values.size(); ++i) {
        if (std::abs(values[i] - target) < std::abs(values[best] - target)) {
            best = i;
        }
    }
    return best;
}

std::pair<std::size_t, double> closestTileGap(const Column &column, double y)
{
    const std::vector<QPointF> offsets = column.tilePositions();
    if (column.isTabbed()) {
        const double top = offsets[column.activeTileIndex].y();
        const double bottom = top + column.tiles[column.activeTileIndex].pendingOuterSize().height();
        if (std::abs(top - y) <= std::abs(bottom - y)) {
            return {column.activeTileIndex, top};
        }
        return {column.activeTileIndex + 1, bottom};
    }
    std::size_t best = 0;
    for (std::size_t i = 1; i < offsets.size(); ++i) {
        if (std::abs(offsets[i].y() - y) < std::abs(offsets[best].y() - y)) {
            best = i;
        }
    }
    return {best, offsets[best].y()};
}

}

DropSlot ColumnStrip::dropSlotAt(QPointF pos, std::optional<Config::ColumnPosition> movingPin) const
{
    DropSlot position = unpinnedDropSlotAt(pos);
    if (position.kind == DropSlot::Kind::InColumn && movingPin && m_columns[position.column].pinnedPosition() != movingPin) {
        position = newColumnAt(position.column);
    }
    if (position.kind == DropSlot::Kind::NewColumn) {
        position.column = allowedColumnIndex(movingPin, position.column, std::nullopt);
    }
    return position;
}

DropSlot ColumnStrip::unpinnedDropSlotAt(QPointF pos) const
{
    if (m_columns.empty()) {
        return newColumnAt(0);
    }

    const double gaps = m_options->layout.gaps;
    const double x = pos.x() + scrollPosition() + gaps / 2.0;
    const double y = pos.y() + gaps / 2.0;
    if (x < 0.0) {
        return newColumnAt(0);
    }

    const std::vector<double> xs = columnOffsets();
    const std::size_t closestColIndex = closestIndex(xs, x);
    std::size_t colIndex = 0;
    for (std::size_t i = 0; i < xs.size() && xs[i] <= x; ++i) {
        colIndex = i;
    }
    if (colIndex == m_columns.size()) {
        return newColumnAt(closestColIndex);
    }

    const auto [closestTileIndex, tileY] = closestTileGap(m_columns[colIndex], y);
    if (std::abs(xs[closestColIndex] - x) <= std::abs(tileY - y)) {
        return newColumnAt(closestColIndex);
    }
    return {DropSlot::Kind::InColumn, colIndex, closestTileIndex};
}

std::optional<QRectF> ColumnStrip::dropSlotRect(DropSlot position) const
{
    auto area = rawDropSlotRect(position);
    if (!area) {
        return std::nullopt;
    }
    if (m_columns.empty()) {
        const double offset = centersActiveColumn() ? centeredScroll(0.0, 0.0, area->width(), WindowMode::Normal)
                                                    : fitScroll(0.0, 0.0, area->width(), WindowMode::Normal);
        area->moveLeft(area->x() - offset);
    } else {
        area->moveLeft(area->x() - scrollPosition());
    }
    return area;
}

std::optional<QRectF> ColumnStrip::rawDropSlotRect(DropSlot position) const
{
    const double gaps = m_options->layout.gaps;
    const double height = m_area.workingArea.height() - gaps * 2.0;
    const double top = m_area.workingArea.y() + gaps;

    if (position.kind == DropSlot::Kind::Floating) {
        return std::nullopt;
    }
    if (position.column > m_columns.size()) {
        return std::nullopt;
    }

    if (position.kind == DropSlot::Kind::NewColumn) {
        double x = columnOffset(position.column);
        if (position.column == 0 && !m_columns.empty()) {
            x -= HintColumnWidth + gaps;
        } else if (position.column != 0 && position.column != m_columns.size()) {
            x -= HintColumnWidth / 2.0 + gaps / 2.0;
        }
        return QRectF(x, top, HintColumnWidth, height);
    }

    const Column &column = m_columns[position.column];
    if (position.tile > column.tiles.size()) {
        return std::nullopt;
    }
    const auto [hintHeight, hintY] = inColumnHintGeometry(column, position.tile);
    const double extraWidth = column.isTabbed() && column.sizingMode() == WindowMode::Normal ? column.reservedSize().width() : 0.0;
    const double width = m_columns[position.column].width() - extraWidth;
    return QRectF(columnOffset(position.column) + column.tileAreaOrigin().x(), hintY, width, hintHeight);
}

std::pair<double, double> ColumnStrip::inColumnHintGeometry(const Column &column, std::size_t tileIndex) const
{
    if (column.isTabbed()) {
        const double top = column.tilePosition(column.activeTileIndex).y();
        if (tileIndex <= column.activeTileIndex) {
            return {HintThickness, top};
        }
        return {HintThickness, top + column.tiles[column.activeTileIndex].pendingOuterSize().height() - HintThickness};
    }

    const double top = column.tilePosition(tileIndex).y();
    if (tileIndex == 0) {
        return {HintThickness, top};
    }
    if (tileIndex == column.tiles.size()) {
        return {HintThickness, top - m_options->layout.gaps - HintThickness};
    }
    return {HintThickness * 2.0, top - m_options->layout.gaps / 2.0 - HintThickness};
}

QSize ColumnStrip::initialWindowSize(const std::optional<Config::PresetSize> &width, const std::optional<Config::PresetSize> &height,
    const EffectiveWindowRules &rules) const
{
    const Config::Border border = mergeBorder(m_options->layout.border, rules.border);
    const double borderExtra = border.enabled ? border.width * 2.0 : 0.0;
    const auto display = rules.defaultColumnDisplay.value_or(m_options->layout.defaultColumnDisplay);
    QSizeF extra(0.0, 0.0);
    if (display == Config::ColumnDisplay::Tabbed) {
        extra = TabBar(m_options->layout.tabIndicator).reservedSize(1, m_area.scale);
    }

    const auto resolve = [&](const Config::PresetSize &preset, double available, double extent) {
        const PresetExtent resolved = measurePreset(preset, *m_options, available, extent);
        return resolved.isTile ? resolved.value - borderExtra : resolved.value;
    };

    const int resolvedWidth = width ? std::max(1, floorToInt(resolve(*width, m_area.workingArea.width(), extra.width()))) : 0;
    const double fullHeight = m_area.workingArea.height() - m_options->layout.gaps * 2.0 - borderExtra;
    const double resolvedHeight = height ? std::min(resolve(*height, m_area.workingArea.height(), extra.height()), fullHeight) : fullHeight;
    return {resolvedWidth, std::max(floorToInt(resolvedHeight), 1)};
}

bool ColumnStrip::beginResize(WindowId window, quint8 edges)
{
    if (m_resize) {
        return false;
    }
    const auto location = locate(window);
    if (!location) {
        return false;
    }
    Column &column = m_columns[location->column];
    if (column.requestedMode() != WindowMode::Normal) {
        return false;
    }
    m_resize = ResizeSession {window, column.tiles[location->tile].windowSize(), edges};
    m_scroll.halt();
    return true;
}

bool ColumnStrip::updateResize(WindowId window, QPointF delta)
{
    if (!m_resize || m_resize->window != window) {
        return false;
    }
    const auto location = locate(window);
    if (!location) {
        return false;
    }
    const QSizeF original = m_resize->originalWindowSize;
    const quint8 edges = m_resize->edges;
    const auto has = [edges](ResizeEdge edge) { return (edges & static_cast<quint8>(edge)) != 0; };
    Column &column = m_columns[location->column];

    if (has(ResizeEdge::Left) || has(ResizeEdge::Right)) {
        double dx = has(ResizeEdge::Left) ? -delta.x() : delta.x();
        if (centersActiveColumn()) {
            dx *= 2.0;
        }
        column.setColumnWidth({ChangeKind::SetFixed, std::round(original.width() + dx)}, location->tile, false);
    }
    if ((has(ResizeEdge::Top) || has(ResizeEdge::Bottom)) && (!has(ResizeEdge::Top) || location->tile != 0)) {
        const double dy = has(ResizeEdge::Top) ? -delta.y() : delta.y();
        column.setWindowHeight({ChangeKind::SetFixed, std::round(original.height() + dy)}, location->tile, false);
    }
    return true;
}

void ColumnStrip::endResize(std::optional<WindowId> window)
{
    if (!m_resize) {
        return;
    }
    if (window) {
        if (*window != m_resize->window) {
            return;
        }
        if (!m_columns.empty() && m_columns[m_activeColumnIndex].contains(*window)) {
            scrollToColumn(std::nullopt, m_activeColumnIndex, std::nullopt);
        }
    }
    m_resize.reset();
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
    for (const Column &column : m_columns) {
        if (column.tiles.empty()) {
            return QStringLiteral("scrolling: columns must not be empty");
        }
        if (column.activeTileIndex >= column.tiles.size()) {
            return QStringLiteral("scrolling: active tile index out of range");
        }
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
