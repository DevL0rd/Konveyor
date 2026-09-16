#include "layout/strip/columnstrip.h"

#include "layout/common/geometry.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace Konveyor::Layout
{

void ColumnStrip::consumeLeftIntoAdjacent(Location source, QPointF prevOff, bool sourceWasActive)
{
    const std::size_t target = source.column - 1;
    const double offX = m_activeColumnIndex <= source.column ? columnOffset(source.column) - columnOffset(target)
                                                             : std::max(0.0, m_columns[target].width() - m_columns[source.column].width());
    QPointF offset(offX, 0.0);
    if (sourceWasActive && !m_returnColumnOnClose) {
        m_returnColumnOnClose = ReturnColumn {m_scroll.resting() + offset.x(), false};
    }
    offset += m_columns[source.column].animationOffset();
    DetachedTile removed = detachTileAt(source.column, 0, m_options->animations.windowMovement);
    insertIntoColumn(target, std::nullopt, std::move(removed.tile), sourceWasActive);

    Column &targetColumn = m_columns[target];
    offset -= targetColumn.animationOffset();
    offset += prevOff - targetColumn.tilePosition(targetColumn.tiles.size() - 1);
    targetColumn.tiles.back().slideFrom(offset);
}

void ColumnStrip::expelLeft(Location source, QPointF prevOff, bool sourceWasActive)
{
    QPointF offset = m_columns[source.column].animationOffset();
    DetachedTile removed = detachTileAt(source.column, source.tile, std::nullopt);
    const std::size_t target = source.column;
    addTile(target, std::move(removed.tile), sourceWasActive, removed.width, removed.fillsWidth, m_options->animations.windowMovement);
    if (sourceWasActive) {
        m_returnColumnOnClose.reset();
    }
    if (target <= m_activeColumnIndex) {
        offset.rx() += columnOffset(target + 1) - columnOffset(target);
    }
    Column &newColumn = m_columns[target];
    offset += prevOff - newColumn.tilePosition(0);
    newColumn.tiles[0].slideFrom(offset);
}

void ColumnStrip::consumeOrExpelWindowLeft(std::optional<WindowId> window)
{
    consumeOrExpelWindow(window, ScrollDirection::Left);
}

void ColumnStrip::consumeRightIntoAdjacent(Location source, QPointF prevOff, bool sourceWasActive)
{
    const std::size_t target = source.column;
    QPointF offset = m_columns[source.column].animationOffset();
    offset.rx() += columnOffset(source.column) - columnOffset(source.column + 1);
    offset -= m_columns[source.column + 1].animationOffset();
    if (sourceWasActive) {
        m_returnColumnOnClose.reset();
    }
    DetachedTile removed = detachTileAt(source.column, 0, m_options->animations.windowMovement);
    insertIntoColumn(target, std::nullopt, std::move(removed.tile), sourceWasActive);

    Column &targetColumn = m_columns[target];
    offset += prevOff - targetColumn.tilePosition(targetColumn.tiles.size() - 1);
    targetColumn.tiles.back().slideFrom(offset);
}

void ColumnStrip::expelRight(Location source, QPointF prevOff, bool sourceWasActive)
{
    const double curX = columnOffset(source.column);
    const double prevWidth = m_columns[source.column].width();
    QPointF offset = m_columns[source.column].animationOffset();
    DetachedTile removed = detachTileAt(source.column, source.tile, std::nullopt);
    const std::size_t target = source.column + 1;
    addTile(target, std::move(removed.tile), sourceWasActive, removed.width, removed.fillsWidth, m_options->animations.windowMovement);
    if (m_activeColumnIndex <= target) {
        offset.rx() += curX - columnOffset(target);
    } else {
        offset.rx() -= std::max(0.0, prevWidth - m_columns[target].width());
    }
    Column &newColumn = m_columns[target];
    offset += prevOff - newColumn.tilePosition(0);
    newColumn.tiles[0].slideFrom(offset);
}

void ColumnStrip::consumeOrExpelWindowRight(std::optional<WindowId> window)
{
    consumeOrExpelWindow(window, ScrollDirection::Right);
}

void ColumnStrip::consumeOrExpelWindow(std::optional<WindowId> window, ScrollDirection direction)
{
    if (m_columns.empty()) {
        return;
    }
    const bool left = direction == ScrollDirection::Left;
    const Location source = targetLocation(window);
    const Column &sourceColumn = m_columns[source.column];
    const QPointF prevOff = sourceColumn.tilePosition(source.tile);
    const bool wasActive = m_activeColumnIndex == source.column && sourceColumn.activeTileIndex == source.tile;

    if (sourceColumn.tiles.size() > 1) {
        if (left) {
            expelLeft(source, prevOff, wasActive);
        } else {
            expelRight(source, prevOff, wasActive);
        }
        return;
    }
    if (left && source.column > 0) {
        consumeLeftIntoAdjacent(source, prevOff, wasActive);
    } else if (!left && source.column + 1 < m_columns.size()) {
        consumeRightIntoAdjacent(source, prevOff, wasActive);
    }
}

void ColumnStrip::consumeIntoColumn()
{
    if (m_columns.size() < 2 || m_activeColumnIndex == m_columns.size() - 1) {
        return;
    }
    const std::size_t target = m_activeColumnIndex;
    const std::size_t source = m_activeColumnIndex + 1;
    QPointF offset = m_columns[source].animationOffset();
    offset.rx() += columnOffset(source) - columnOffset(target);
    const QPointF prevOff = m_columns[source].tilePosition(0);

    DetachedTile removed = detachTileAt(source, 0, std::nullopt);
    insertIntoColumn(target, std::nullopt, std::move(removed.tile), false);

    Column &targetColumn = m_columns[target];
    offset += prevOff - targetColumn.tilePosition(targetColumn.tiles.size() - 1);
    offset -= targetColumn.animationOffset();
    targetColumn.tiles.back().slideFrom(offset);
}

void ColumnStrip::expelFromColumn()
{
    if (m_columns.empty() || m_columns[m_activeColumnIndex].tiles.size() == 1) {
        return;
    }
    const std::size_t source = m_activeColumnIndex;
    const std::size_t target = m_activeColumnIndex + 1;
    const double curX = columnOffset(source);
    const Column &sourceColumn = m_columns[source];
    const std::size_t sourceTile = sourceColumn.tiles.size() - 1;
    QPointF offset = sourceColumn.animationOffset();
    const QPointF prevOff = sourceColumn.tilePosition(sourceTile);

    DetachedTile removed = detachTileAt(source, sourceTile, std::nullopt);
    addTile(target, std::move(removed.tile), false, removed.width, removed.fillsWidth, m_options->animations.windowMovement);

    offset.rx() += curX - columnOffset(target);
    Column &newColumn = m_columns[target];
    offset += prevOff - newColumn.tilePosition(0);
    newColumn.tiles[0].slideFrom(offset);
}

void ColumnStrip::swapTiles(std::size_t sourceColumnIndex, std::size_t targetColumnIndex, ScrollDirection direction)
{
    const std::size_t sourceTileIndex = m_columns[sourceColumnIndex].activeTileIndex;
    const std::size_t targetTileIndex = m_columns[targetColumnIndex].activeTileIndex;
    const bool sourceDrained = m_columns[sourceColumnIndex].tiles.size() == 1;

    QPointF sourcePt = m_columns[sourceColumnIndex].animationOffset() + m_columns[sourceColumnIndex].tilePosition(sourceTileIndex);
    QPointF targetPt = m_columns[targetColumnIndex].animationOffset() + m_columns[targetColumnIndex].tilePosition(targetTileIndex);
    sourcePt.rx() += columnOffset(sourceColumnIndex);
    targetPt.rx() += columnOffset(targetColumnIndex);

    DetachedTile sourceRemoved = detachTileAt(sourceColumnIndex, sourceTileIndex, std::nullopt);
    const std::size_t adjustedTarget = direction == ScrollDirection::Right && sourceDrained ? targetColumnIndex - 1 : targetColumnIndex;
    insertIntoColumn(adjustedTarget, targetTileIndex, std::move(sourceRemoved.tile), false);
    DetachedTile targetRemoved = detachTileAt(adjustedTarget, targetTileIndex + 1, std::nullopt);

    if (sourceDrained) {
        addTile(sourceColumnIndex, std::move(targetRemoved.tile), true, sourceRemoved.width, sourceRemoved.fillsWidth, std::nullopt);
    } else {
        insertIntoColumn(sourceColumnIndex, sourceTileIndex, std::move(targetRemoved.tile), false);
    }

    m_columns[sourceColumnIndex].activeTileIndex = sourceTileIndex;
    m_columns[targetColumnIndex].activeTileIndex = targetTileIndex;

    Tile &targetTile = m_columns[targetColumnIndex].tiles[targetTileIndex];
    targetTile.slideFrom(sourcePt - targetPt);
    targetTile.ensureFadesToOpaque();

    Tile &sourceTile = m_columns[sourceColumnIndex].tiles[sourceTileIndex];
    sourceTile.stopSlides();
    sourceTile.slideFrom(targetPt - sourcePt);
    sourceTile.ensureFadesToOpaque();

    activateColumn(targetColumnIndex);
}

void ColumnStrip::swapWindowInDirection(ScrollDirection direction)
{
    if (m_columns.empty()) {
        return;
    }
    const bool left = direction == ScrollDirection::Left;
    if ((left && m_activeColumnIndex == 0) || (!left && m_activeColumnIndex == m_columns.size() - 1)) {
        return;
    }
    const std::size_t source = m_activeColumnIndex;
    const std::size_t target = left ? source - 1 : source + 1;
    if (m_columns[source].tiles.size() == 1 && m_columns[target].tiles.size() == 1) {
        moveColumnTo(target);
        return;
    }
    swapTiles(source, target, direction);
}

void ColumnStrip::toggleColumnTabbedDisplay()
{
    if (m_columns.empty()) {
        return;
    }
    const bool tabbed = m_columns[m_activeColumnIndex].isTabbed();
    setColumnDisplay(tabbed ? Config::ColumnDisplay::Normal : Config::ColumnDisplay::Tabbed);
}

void ColumnStrip::setColumnDisplay(Config::ColumnDisplay display)
{
    if (m_columns.empty() || m_columns[m_activeColumnIndex].displayStyle == display) {
        return;
    }
    Column &column = m_columns[m_activeColumnIndex];
    cancelResizeForColumn(column);
    column.setColumnDisplay(display);
    column.layoutTiles(true);

    if (display != Config::ColumnDisplay::Tabbed && column.tiles.size() > 1) {
        const WindowId window = column.tiles[column.activeTileIndex].id();
        setFullscreen(window, false);
        setMaximized(window, false);
    }
}

}
