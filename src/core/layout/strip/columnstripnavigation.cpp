#include "layout/strip/columnstrip.h"

#include "layout/common/geometry.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace Konveyor::Layout
{

bool ColumnStrip::focusLeft()
{
    if (m_activeColumnIndex == 0) {
        return false;
    }
    activateColumn(m_activeColumnIndex - 1);
    return true;
}

bool ColumnStrip::focusRight()
{
    if (m_activeColumnIndex + 1 >= m_columns.size()) {
        return false;
    }
    activateColumn(m_activeColumnIndex + 1);
    return true;
}

void ColumnStrip::focusColumnFirst()
{
    activateColumn(0);
}

void ColumnStrip::focusColumnLast()
{
    if (!m_columns.empty()) {
        activateColumn(m_columns.size() - 1);
    }
}

void ColumnStrip::focusColumn(std::size_t index)
{
    if (!m_columns.empty()) {
        activateColumn(std::min(index > 0 ? index - 1 : 0, m_columns.size() - 1));
    }
}

void ColumnStrip::focusWindowInColumn(std::size_t index)
{
    if (!m_columns.empty()) {
        m_columns[m_activeColumnIndex].focusTileAt(index);
    }
}

bool ColumnStrip::focusDown()
{
    return !m_columns.empty() && m_columns[m_activeColumnIndex].focusDown();
}

bool ColumnStrip::focusUp()
{
    return !m_columns.empty() && m_columns[m_activeColumnIndex].focusUp();
}

void ColumnStrip::focusDownOrLeft()
{
    if (!m_columns.empty() && !m_columns[m_activeColumnIndex].focusDown()) {
        focusLeft();
    }
}

void ColumnStrip::focusDownOrRight()
{
    if (!m_columns.empty() && !m_columns[m_activeColumnIndex].focusDown()) {
        focusRight();
    }
}

void ColumnStrip::focusUpOrLeft()
{
    if (!m_columns.empty() && !m_columns[m_activeColumnIndex].focusUp()) {
        focusLeft();
    }
}

void ColumnStrip::focusUpOrRight()
{
    if (!m_columns.empty() && !m_columns[m_activeColumnIndex].focusUp()) {
        focusRight();
    }
}

void ColumnStrip::focusTop()
{
    if (!m_columns.empty()) {
        m_columns[m_activeColumnIndex].focusTop();
    }
}

void ColumnStrip::focusBottom()
{
    if (!m_columns.empty()) {
        m_columns[m_activeColumnIndex].focusBottom();
    }
}

void ColumnStrip::centerColumn()
{
    if (m_columns.empty()) {
        return;
    }
    scrollToColumnCentered(std::nullopt, m_activeColumnIndex, m_options->animations.horizontalViewMovement);
    cancelResizeForColumn(m_columns[m_activeColumnIndex]);
}

void ColumnStrip::centerWindow(std::optional<WindowId> window)
{
    if (m_columns.empty()) {
        return;
    }
    const std::size_t colIndex = window ? columnIndexOf(*window) : m_activeColumnIndex;
    if (colIndex == m_activeColumnIndex) {
        centerColumn();
    }
}

ColumnStrip::VisibleColumns ColumnStrip::fullyVisibleColumns() const
{
    VisibleColumns result;
    const double viewX = targetScrollPosition();
    const double workingX = m_area.workingArea.x();
    const double workingW = m_area.workingArea.width();
    const double gap = m_options->layout.gaps;
    const std::vector<double> xs = columnOffsets();
    for (std::size_t idx = 0; idx < m_columns.size(); ++idx) {
        const double colX = xs[idx];
        if (colX < viewX + workingX + gap) {
            continue;
        }
        if (!result.leftmostColX) {
            result.leftmostColX = colX;
        }
        const double width = m_columns[idx].width();
        if (viewX + workingX + workingW < colX + width + gap) {
            break;
        }
        if (idx == m_activeColumnIndex) {
            result.activeColX = colX;
        } else {
            result.countedNonActive = true;
        }
        result.widthTaken += width + gap;
    }
    return result;
}

void ColumnStrip::centerVisibleColumns()
{
    if (m_columns.empty() || centersActiveColumn()) {
        return;
    }
    const VisibleColumns visible = fullyVisibleColumns();
    if (!visible.activeColX) {
        return;
    }
    cancelResizeForColumn(m_columns[m_activeColumnIndex]);
    const double gap = m_options->layout.gaps;
    const double freeSpace = m_area.workingArea.width() - visible.widthTaken + gap;
    const double newViewX = *visible.leftmostColX - freeSpace / 2.0 - m_area.workingArea.x();
    animateScroll(m_activeColumnIndex, newViewX - *visible.activeColX);
    scrollToColumn(std::nullopt, m_activeColumnIndex, std::nullopt);
}

void ColumnStrip::moveColumnToIndex(std::size_t index)
{
    if (!m_columns.empty()) {
        moveColumnTo(std::min(index > 0 ? index - 1 : 0, m_columns.size() - 1));
    }
}

void ColumnStrip::cancelResizeForColumn(Column &column)
{
    if (m_resize && column.contains(m_resize->window)) {
        m_resize.reset();
    }
    for (Tile &tile : column.tiles) {
        tile.window().cancelInteractiveResize();
    }
}

void ColumnStrip::moveColumnTo(std::size_t requestedIndex)
{
    if (m_columns.empty()) {
        return;
    }
    const std::size_t newIndex = allowedColumnIndex(m_columns[m_activeColumnIndex].pinnedPosition(), requestedIndex, m_activeColumnIndex);
    if (m_activeColumnIndex == newIndex) {
        return;
    }
    const double currentColX = columnOffset(m_activeColumnIndex);
    const double nextColX = columnOffset(m_activeColumnIndex + 1);

    const auto oldPos = static_cast<std::ptrdiff_t>(m_activeColumnIndex);
    Column column = std::move(m_columns[m_activeColumnIndex]);
    m_columns.erase(m_columns.begin() + oldPos);
    cancelResizeForColumn(column);
    const auto newPos = static_cast<std::ptrdiff_t>(newIndex);
    m_columns.insert(m_columns.begin() + newPos, std::move(column));

    m_scroll.offset(-columnOffset(m_activeColumnIndex) + currentColX);
    m_columns[newIndex].slideXFrom(currentColX - columnOffset(newIndex));

    const double othersOffset = nextColX - currentColX;
    if (m_activeColumnIndex < newIndex) {
        for (std::size_t i = m_activeColumnIndex; i < newIndex; ++i) {
            m_columns[i].slideXFrom(othersOffset);
        }
    } else {
        for (std::size_t i = newIndex + 1; i <= m_activeColumnIndex; ++i) {
            m_columns[i].slideXFrom(-othersOffset);
        }
    }
    selectColumnWith(newIndex, m_options->animations.windowMovement);
}

bool ColumnStrip::moveLeft()
{
    if (m_activeColumnIndex == 0) {
        return false;
    }
    moveColumnTo(m_activeColumnIndex - 1);
    return true;
}

bool ColumnStrip::moveRight()
{
    if (m_activeColumnIndex + 1 >= m_columns.size()) {
        return false;
    }
    moveColumnTo(m_activeColumnIndex + 1);
    return true;
}

void ColumnStrip::moveColumnToFirst()
{
    moveColumnTo(0);
}

void ColumnStrip::moveColumnToLast()
{
    if (!m_columns.empty()) {
        moveColumnTo(m_columns.size() - 1);
    }
}

bool ColumnStrip::moveDown()
{
    return !m_columns.empty() && m_columns[m_activeColumnIndex].moveDown();
}

bool ColumnStrip::moveUp()
{
    return !m_columns.empty() && m_columns[m_activeColumnIndex].moveUp();
}

}
