#include "layout/strip/columnstrip.h"

#include "layout/common/geometry.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace Konveyor::Layout
{

namespace
{

template<class Fn> void withTargetColumn(std::vector<Column> &columns, std::size_t activeIndex, std::optional<WindowId> window, Fn fn)
{
    if (columns.empty()) {
        return;
    }
    if (!window) {
        fn(columns[activeIndex], std::optional<std::size_t>());
        return;
    }
    for (Column &column : columns) {
        if (const auto idx = column.position(*window)) {
            fn(column, idx);
            return;
        }
    }
}

}

bool ColumnStrip::setFullscreen(WindowId window, bool fullscreen)
{
    std::size_t colIndex = columnIndexOf(window);
    if (fullscreen == m_columns[colIndex].fullscreenPending) {
        return false;
    }
    cancelResizeForColumn(m_columns[colIndex]);
    if (fullscreen && m_columns[colIndex].tiles.size() > 1 && !m_columns[colIndex].isTabbed()) {
        consumeOrExpelWindowRight(window);
        colIndex += 1;
    }
    m_columns[colIndex].setFullscreen(fullscreen);
    return true;
}

bool ColumnStrip::setMaximized(WindowId window, bool maximize)
{
    std::size_t colIndex = columnIndexOf(window);
    if (maximize == m_columns[colIndex].maximizePending) {
        return false;
    }
    cancelResizeForColumn(m_columns[colIndex]);
    if (maximize && m_columns[colIndex].tiles.size() > 1 && !m_columns[colIndex].isTabbed()) {
        consumeOrExpelWindowRight(window);
        colIndex += 1;
    }
    m_columns[colIndex].setMaximized(maximize);
    return true;
}

void ColumnStrip::toggleWidth(bool forwards)
{
    if (m_columns.empty()) {
        return;
    }
    Column &column = m_columns[m_activeColumnIndex];
    column.toggleWidth(std::nullopt, forwards);
    cancelResizeForColumn(column);
}

void ColumnStrip::toggleFillWidth()
{
    if (!m_columns.empty()) {
        toggleFillWidthAt(m_activeColumnIndex);
    }
}

void ColumnStrip::toggleFillWidthFor(WindowId window)
{
    if (hasWindow(window)) {
        toggleFillWidthAt(columnIndexOf(window));
    }
}

void ColumnStrip::toggleFillWidthAt(std::size_t idx)
{
    Column &column = m_columns[idx];
    column.toggleFillWidth();
    cancelResizeForColumn(column);
    if (idx == m_activeColumnIndex) { }
}

void ColumnStrip::setWindowWidth(std::optional<WindowId> window, SizeChange change)
{
    withTargetColumn(m_columns, m_activeColumnIndex, window, [&](Column &column, std::optional<std::size_t> tileIndex) {
        column.setColumnWidth(change, tileIndex, true);
        cancelResizeForColumn(column);
    });
}

void ColumnStrip::setWindowHeight(std::optional<WindowId> window, SizeChange change)
{
    withTargetColumn(m_columns, m_activeColumnIndex, window, [&](Column &column, std::optional<std::size_t> tileIndex) {
        column.setWindowHeight(change, tileIndex, true);
        cancelResizeForColumn(column);
    });
}

void ColumnStrip::resetWindowHeight(std::optional<WindowId> window)
{
    withTargetColumn(m_columns, m_activeColumnIndex, window, [&](Column &column, std::optional<std::size_t> tileIndex) {
        column.resetWindowHeight(tileIndex);
        cancelResizeForColumn(column);
    });
}

void ColumnStrip::toggleWindowWidth(std::optional<WindowId> window, bool forwards)
{
    withTargetColumn(m_columns, m_activeColumnIndex, window, [&](Column &column, std::optional<std::size_t> tileIndex) {
        column.toggleWidth(tileIndex, forwards);
        cancelResizeForColumn(column);
    });
}

void ColumnStrip::toggleWindowHeight(std::optional<WindowId> window, bool forwards)
{
    withTargetColumn(m_columns, m_activeColumnIndex, window, [&](Column &column, std::optional<std::size_t> tileIndex) {
        column.toggleWindowHeight(tileIndex, forwards);
        cancelResizeForColumn(column);
    });
}

void ColumnStrip::expandColumnToAvailableWidth()
{
    if (m_columns.empty()) {
        return;
    }
    Column &column = m_columns[m_activeColumnIndex];
    if (column.requestedMode() != WindowMode::Normal || column.fillsWidth) {
        return;
    }
    if (centersActiveColumn()) {
        column.toggleFillWidth();
        cancelResizeForColumn(column);
        return;
    }

    const VisibleColumns visible = fullyVisibleColumns();
    if (!visible.activeColX) {
        return;
    }
    const double gap = m_options->layout.gaps;
    const double availableWidth = m_area.workingArea.width() - gap - visible.widthTaken - column.reservedSize().width();
    if (availableWidth <= 0.0) {
        return;
    }
    cancelResizeForColumn(column);
    if (!visible.countedNonActive) {
        column.toggleFillWidth();
        return;
    }

    column.widthSetting = ColumnWidth::fixed(m_columns[m_activeColumnIndex].width() + availableWidth);
    column.presetWidthIndex.reset();
    column.fillsWidth = false;
    column.layoutTiles(true);

    const double newViewX = *visible.leftmostColX - gap - m_area.workingArea.x();
    animateScroll(m_activeColumnIndex, newViewX - *visible.activeColX);
    scrollToColumn(std::nullopt, m_activeColumnIndex, std::nullopt);
}

}
