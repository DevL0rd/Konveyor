#include "layout/strip/columnstrip.h"

#include "layout/common/geometry.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace Konveyor::Layout
{

std::vector<double> ColumnStrip::columnOffsets() const
{
    std::vector<double> xs;
    xs.reserve(m_columns.size() + 1);
    double x = 0.0;
    for (const Column &column : m_columns) {
        xs.push_back(x);
        x += column.width() + m_options->layout.gaps;
    }
    xs.push_back(x);
    return xs;
}

double ColumnStrip::columnOffset(std::size_t idx) const
{
    double x = 0.0;
    for (std::size_t i = 0; i < idx && i < m_columns.size(); ++i) {
        x += m_columns[i].width() + m_options->layout.gaps;
    }
    return x;
}

std::optional<QPointF> ColumnStrip::columnRenderPosition(quint64 columnId) const
{
    for (const ColumnRef &ref : renderedColumns()) {
        if (ref.column->id() == columnId) {
            return ref.pos;
        }
    }
    return std::nullopt;
}

std::vector<ColumnRef> ColumnStrip::renderedColumns() const
{
    return placedColumns(true);
}

std::vector<ColumnRef> ColumnStrip::placedColumns(bool animated) const
{
    std::vector<ColumnRef> result;
    if (m_columns.empty()) {
        return result;
    }
    const std::vector<double> xs = columnOffsets();
    const double viewOff = animated ? -scrollPosition() : -targetScrollPosition();
    const auto push = [&](std::size_t idx) {
        const Column &column = m_columns[idx];
        const QPointF offset = animated ? column.animationOffset() : QPointF();
        result.push_back({&column, QPointF(viewOff + xs[idx], 0.0) + offset});
    };
    push(m_activeColumnIndex);
    for (std::size_t i = 0; i < m_columns.size(); ++i) {
        if (i != m_activeColumnIndex) {
            push(i);
        }
    }
    return result;
}

std::vector<ConstTileRef> ColumnStrip::placedTiles(bool animated) const
{
    std::vector<ConstTileRef> result;
    for (const ColumnRef &ref : placedColumns(animated)) {
        const Column &column = *ref.column;
        const std::vector<QPointF> offsets = column.tilePositions();
        for (const std::size_t i : column.renderOrder()) {
            const Tile &tile = column.tiles[i];
            const QPointF offset = animated ? tile.animationOffset() : QPointF();
            const QPointF pos = roundPoint(ref.pos + offsets[i] + offset, m_area.scale);
            result.push_back({&tile, pos, i == column.activeTileIndex || !column.isTabbed()});
        }
    }
    return result;
}

std::vector<TileRef> ColumnStrip::renderedTilesMut(bool round)
{
    std::vector<TileRef> result;
    for (const ColumnRef &ref : renderedColumns()) {
        auto &column = const_cast<Column &>(*ref.column);
        const std::vector<QPointF> offsets = column.tilePositions();
        for (const std::size_t i : column.renderOrder()) {
            Tile &tile = column.tiles[i];
            const QPointF raw = ref.pos + offsets[i] + tile.animationOffset();
            result.push_back({&tile, round ? roundPoint(raw, m_area.scale) : raw, i == column.activeTileIndex || !column.isTabbed()});
        }
    }
    return result;
}

void ColumnStrip::addColumn(std::optional<std::size_t> idx, Column column, bool activate, std::optional<Config::AnimationParams> anim)
{
    const bool wasEmpty = m_columns.empty();
    const bool newOnRight = m_options->layout.newColumnPosition == Config::NewColumnPosition::Right;
    const std::size_t preferred = wasEmpty ? 0 : m_activeColumnIndex + (newOnRight ? 1 : 0);
    const std::size_t index = allowedColumnIndex(column.pinnedPosition(), idx.value_or(preferred), std::nullopt);
    const std::size_t previousActive = m_activeColumnIndex;

    column.updateConfig(m_area, m_options);
    const auto pos = static_cast<std::ptrdiff_t>(index);
    m_columns.insert(m_columns.begin() + pos, std::move(column));

    if (!wasEmpty && index <= m_activeColumnIndex) {
        m_activeColumnIndex += 1;
    }

    const double offset = columnOffset(index + 1) - columnOffset(index);
    animateColumnsAfter(index, -offset, anim.value_or(m_options->animations.windowMovement), false);

    if (!activate) {
        return;
    }
    if (wasEmpty) {
        m_scroll.setIdle(0.0);
        m_scroll.setIdle(scrollForColumn(std::nullopt, index, std::nullopt));
    }
    std::optional<ReturnColumn> returnColumn;
    if (!wasEmpty && (index == previousActive + 1 || index == previousActive)) {
        returnColumn = ReturnColumn {m_scroll.resting(), index == previousActive};
    }
    selectColumnWith(index, anim.value_or(m_options->animations.horizontalViewMovement));
    m_returnColumnOnClose = returnColumn;
}

void ColumnStrip::addTile(std::optional<std::size_t> colIndex, Tile tile, bool activate, ColumnWidth width, bool fillsWidth,
    std::optional<Config::AnimationParams> anim)
{
    Column column(std::move(tile), m_area, width, fillsWidth);
    addColumn(colIndex, std::move(column), activate, anim);
}

void ColumnStrip::insertIntoColumn(std::size_t colIndex, std::optional<std::size_t> tileIndex, Tile tile, bool activate)
{
    const double prevNextX = columnOffset(colIndex + 1);
    Column &target = m_columns[colIndex];
    const std::size_t idx = tileIndex.value_or(target.tiles.size());
    std::size_t prevActiveTileIndex = target.activeTileIndex;

    target.insertTile(idx, std::move(tile));

    if (idx <= prevActiveTileIndex) {
        target.activeTileIndex += 1;
        prevActiveTileIndex += 1;
    }

    if (activate) {
        target.selectTile(idx);
        if (m_activeColumnIndex != colIndex) {
            activateColumn(colIndex);
        }
    }

    Column &column = m_columns[colIndex];
    if (column.isTabbed()) {
        const std::size_t fading = column.activeTileIndex == idx ? prevActiveTileIndex : idx;
        column.tiles[fading].fadeOpacity(1.0, 0.0, m_options->animations.windowMovement);
    }

    animateColumnsAfter(colIndex, prevNextX - columnOffset(colIndex + 1), m_options->animations.windowMovement, true);
    placeColumnWithinPins(colIndex);
}

void ColumnStrip::insertTileAfter(WindowId rightOf, Tile tile, bool activate, ColumnWidth width, bool fillsWidth)
{
    addTile(columnIndexOf(rightOf) + 1, std::move(tile), activate, width, fillsWidth, std::nullopt);
}

std::size_t ColumnStrip::allowedColumnIndex(
    std::optional<Config::ColumnPosition> pin, std::size_t desired, std::optional<std::size_t> excluding) const
{
    std::size_t count = 0;
    std::size_t leadingStart = 0;
    std::size_t trailingEnd = 0;
    for (std::size_t idx = 0; idx < m_columns.size(); ++idx) {
        if (idx == excluding) {
            continue;
        }
        const std::optional<Config::ColumnPosition> other = m_columns[idx].pinnedPosition();
        if (other == Config::ColumnPosition::Start && leadingStart == count) {
            ++leadingStart;
        }
        trailingEnd = other == Config::ColumnPosition::End ? trailingEnd + 1 : 0;
        ++count;
    }
    if (pin == Config::ColumnPosition::Start) {
        return std::min(desired, leadingStart);
    }
    if (pin == Config::ColumnPosition::End) {
        return std::max(desired, count - trailingEnd);
    }
    return std::clamp(desired, leadingStart, std::max(leadingStart, count - trailingEnd));
}

void ColumnStrip::placeColumnWithinPins(std::size_t from)
{
    const std::size_t to = allowedColumnIndex(m_columns[from].pinnedPosition(), from, from);
    if (to == from) {
        return;
    }
    const double fromX = columnOffset(from);
    Column column = std::move(m_columns[from]);
    m_columns.erase(m_columns.begin() + static_cast<std::ptrdiff_t>(from));
    m_columns.insert(m_columns.begin() + static_cast<std::ptrdiff_t>(to), std::move(column));
    if (m_activeColumnIndex == from) {
        m_activeColumnIndex = to;
    } else if (from < m_activeColumnIndex && to >= m_activeColumnIndex) {
        --m_activeColumnIndex;
    } else if (from > m_activeColumnIndex && to <= m_activeColumnIndex) {
        ++m_activeColumnIndex;
    }
    m_returnColumnOnClose.reset();
    m_columns[to].slideXFrom(fromX - columnOffset(to));
}

void ColumnStrip::animateColumnsAfter(std::size_t idx, double offset, const Config::AnimationParams &config, bool inclusiveBefore)
{
    if (m_activeColumnIndex <= idx) {
        for (std::size_t i = idx + 1; i < m_columns.size(); ++i) {
            m_columns[i].slideXFromWith(offset, config);
        }
        return;
    }
    const std::size_t end = inclusiveBefore ? idx + 1 : idx;
    for (std::size_t i = 0; i < end; ++i) {
        m_columns[i].slideXFromWith(-offset, config);
    }
}

void ColumnStrip::moveOtherColumnsForResize(std::size_t colIndex, double offset, bool ongoingResizeAnim)
{
    const auto apply = [&](Column &column, double value) {
        if (ongoingResizeAnim) {
            column.slideXFromWith(value, m_options->animations.windowResize);
        } else {
            column.shiftSlideX(value);
        }
    };
    if (m_activeColumnIndex <= colIndex) {
        for (std::size_t i = colIndex + 1; i < m_columns.size(); ++i) {
            apply(m_columns[i], offset);
        }
        return;
    }
    for (std::size_t i = 0; i <= colIndex; ++i) {
        apply(m_columns[i], -offset);
    }
}

DetachedTile ColumnStrip::removeTile(WindowId id)
{
    const Location location = *locate(id);
    return detachTileAt(location.column, location.tile, std::nullopt);
}

DetachedTile ColumnStrip::detachTileAt(std::size_t columnIndex, std::size_t tileIndex, std::optional<Config::AnimationParams> anim)
{
    if (m_columns[columnIndex].tiles.size() == 1) {
        Column column = detachColumnAt(columnIndex, anim);
        return DetachedTile {std::move(column.tiles[tileIndex]), column.widthSetting, column.fillsWidth, false};
    }

    Column &column = m_columns[columnIndex];
    const double prevWidth = m_columns[columnIndex].width();
    const Config::AnimationParams movementConfig = anim.value_or(m_options->animations.windowMovement);

    const std::vector<QPointF> offsets = column.tilePositions();
    const double offsetY = offsets[tileIndex + 1].y() - offsets[tileIndex].y();
    for (std::size_t i = tileIndex + 1; i < column.tiles.size(); ++i) {
        column.tiles[i].slideYFrom(offsetY);
    }
    if (column.isTabbed() && tileIndex != column.activeTileIndex) {
        column.tiles[tileIndex].fadeOpacity(0.0, 1.0, movementConfig);
    }

    const bool wasNormal = column.sizingMode() == WindowMode::Normal;
    const auto pos = static_cast<std::ptrdiff_t>(tileIndex);
    Tile tile = std::move(column.tiles[tileIndex]);
    column.tiles.erase(column.tiles.begin() + pos);
    column.data.erase(column.data.begin() + pos);

    if (columnIndex == m_activeColumnIndex && !wasNormal && column.sizingMode() == WindowMode::Normal) {
        m_scrollToRestore.reset();
    }
    if (column.data.size() == 1 && column.data[0].height.isAuto()) {
        column.data[0].height.value = 1.0;
    }
    if (m_resize && m_resize->window == tile.id()) {
        m_resize.reset();
    }

    DetachedTile removed {std::move(tile), column.widthSetting, column.fillsWidth, false};

    if (tileIndex < column.activeTileIndex) {
        column.activeTileIndex -= 1;
    } else if (tileIndex == column.activeTileIndex) {
        if (tileIndex == column.tiles.size()) {
            column.selectTile(tileIndex - 1);
        } else {
            column.tiles[tileIndex].ensureFadesToOpaque();
        }
    }

    column.layoutTiles(true);
    animateColumnsAfter(columnIndex, prevWidth - m_columns[columnIndex].width(), movementConfig, true);
    return removed;
}

std::optional<Column> ColumnStrip::removeActiveColumn()
{
    if (m_columns.empty()) {
        return std::nullopt;
    }
    return detachColumnAt(m_activeColumnIndex, std::nullopt);
}

Column ColumnStrip::detachColumnAt(std::size_t columnIndex, std::optional<Config::AnimationParams> anim)
{
    const Config::AnimationParams movementConfig = anim.value_or(m_options->animations.windowMovement);
    animateColumnsAfter(columnIndex, columnOffset(columnIndex + 1) - columnOffset(columnIndex), movementConfig, false);

    const auto pos = static_cast<std::ptrdiff_t>(columnIndex);
    Column column = std::move(m_columns[columnIndex]);
    m_columns.erase(m_columns.begin() + pos);

    if (m_resize && column.contains(m_resize->window)) {
        m_resize.reset();
    }
    if (m_returnColumnOnClose && columnIndex == (m_returnColumnOnClose->onRight ? m_activeColumnIndex + 1 : m_activeColumnIndex - 1)) {
        m_returnColumnOnClose.reset();
    }
    if (columnIndex == m_activeColumnIndex) {
        m_scrollToRestore.reset();
    }
    if (m_columns.empty()) {
        return column;
    }

    const Config::AnimationParams viewConfig = anim.value_or(m_options->animations.horizontalViewMovement);
    if (columnIndex < m_activeColumnIndex) {
        m_activeColumnIndex -= 1;
        m_returnColumnOnClose.reset();
    } else if (columnIndex == m_activeColumnIndex) {
        activateAfterRemovingActive(columnIndex, viewConfig);
    } else {
        selectColumnWith(std::min(m_activeColumnIndex, m_columns.size() - 1), viewConfig);
    }
    return column;
}

void ColumnStrip::activateAfterRemovingActive(std::size_t removedIndex, const Config::AnimationParams &viewConfig)
{
    const std::optional<ReturnColumn> previous = std::exchange(m_returnColumnOnClose, std::nullopt);
    const bool returns = previous && (previous->onRight ? removedIndex < m_columns.size() : removedIndex > 0);
    if (!returns) {
        selectColumnWith(std::min(m_activeColumnIndex, m_columns.size() - 1), viewConfig);
        return;
    }
    selectColumnWith(previous->onRight ? removedIndex : removedIndex - 1, viewConfig);
    animateScrollWith(m_activeColumnIndex, previous->viewOffset, viewConfig);
    scrollToColumnWith(std::nullopt, m_activeColumnIndex, std::nullopt, viewConfig);
}

}
