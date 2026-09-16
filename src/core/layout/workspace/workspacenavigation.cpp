#include "layout/workspace/workspace.h"

#include "layout/common/geometry.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace Konveyor::Layout
{

bool Workspace::focusLeft()
{
    focusTiling();
    return m_strip.focusLeft();
}

bool Workspace::focusRight()
{
    focusTiling();
    return m_strip.focusRight();
}

void Workspace::focusColumnFirst()
{
    focusTiling();
    m_strip.focusColumnFirst();
}

void Workspace::focusColumnLast()
{
    focusTiling();
    m_strip.focusColumnLast();
}

void Workspace::focusColumnRightOrFirst()
{
    if (!focusRight()) {
        focusColumnFirst();
    }
}

void Workspace::focusColumnLeftOrLast()
{
    if (!focusLeft()) {
        focusColumnLast();
    }
}

void Workspace::focusColumn(std::size_t index)
{
    focusTiling();
    m_strip.focusColumn(index);
}

void Workspace::focusWindowInColumn(std::size_t index)
{
    focusTiling();
    m_strip.focusWindowInColumn(index);
}

bool Workspace::focusDown()
{
    focusTiling();
    return m_strip.focusDown();
}

bool Workspace::focusUp()
{
    focusTiling();
    return m_strip.focusUp();
}

void Workspace::focusDownOrLeft()
{
    focusTiling();
    m_strip.focusDownOrLeft();
}

void Workspace::focusDownOrRight()
{
    focusTiling();
    m_strip.focusDownOrRight();
}

void Workspace::focusUpOrLeft()
{
    focusTiling();
    m_strip.focusUpOrLeft();
}

void Workspace::focusUpOrRight()
{
    focusTiling();
    m_strip.focusUpOrRight();
}

void Workspace::focusWindowTop()
{
    focusTiling();
    m_strip.focusTop();
}

void Workspace::focusWindowBottom()
{
    focusTiling();
    m_strip.focusBottom();
}

void Workspace::focusWindowDownOrTop()
{
    if (!focusDown()) {
        focusWindowTop();
    }
}

void Workspace::focusWindowUpOrBottom()
{
    if (!focusUp()) {
        focusWindowBottom();
    }
}

void Workspace::centerColumn()
{
    if (isFloatingFocused()) {
        m_floating.centerWindow(std::nullopt);
    } else {
        m_strip.centerColumn();
    }
}

void Workspace::centerWindow(std::optional<WindowId> window)
{
    if (targetIsFloating(window)) {
        m_floating.centerWindow(window);
    } else {
        m_strip.centerWindow(window);
    }
}

void Workspace::centerVisibleColumns()
{
    if (!isFloatingFocused()) {
        m_strip.centerVisibleColumns();
    }
}

bool Workspace::moveLeft()
{
    return isFloatingFocused() || m_strip.moveLeft();
}

bool Workspace::moveRight()
{
    return isFloatingFocused() || m_strip.moveRight();
}

bool Workspace::moveDown()
{
    return isFloatingFocused() || m_strip.moveDown();
}

bool Workspace::moveUp()
{
    return isFloatingFocused() || m_strip.moveUp();
}

void Workspace::moveColumnToFirst()
{
    if (!isFloatingFocused()) {
        m_strip.moveColumnToFirst();
    }
}

void Workspace::moveColumnToLast()
{
    if (!isFloatingFocused()) {
        m_strip.moveColumnToLast();
    }
}

void Workspace::moveColumnToIndex(std::size_t index)
{
    if (!isFloatingFocused()) {
        m_strip.moveColumnToIndex(index);
    }
}

void Workspace::consumeOrExpelWindowLeft(std::optional<WindowId> window)
{
    if (!targetIsFloating(window)) {
        m_strip.consumeOrExpelWindowLeft(window);
    }
}

void Workspace::consumeOrExpelWindowRight(std::optional<WindowId> window)
{
    if (!targetIsFloating(window)) {
        m_strip.consumeOrExpelWindowRight(window);
    }
}

void Workspace::consumeIntoColumn()
{
    if (!isFloatingFocused()) {
        m_strip.consumeIntoColumn();
    }
}

void Workspace::expelFromColumn()
{
    if (!isFloatingFocused()) {
        m_strip.expelFromColumn();
    }
}

void Workspace::swapWindowInDirection(ScrollDirection direction)
{
    if (!isFloatingFocused()) {
        m_strip.swapWindowInDirection(direction);
    }
}

void Workspace::toggleColumnTabbedDisplay()
{
    if (!isFloatingFocused()) {
        m_strip.toggleColumnTabbedDisplay();
    }
}

void Workspace::setColumnDisplay(Config::ColumnDisplay display)
{
    if (!isFloatingFocused()) {
        m_strip.setColumnDisplay(display);
    }
}

}
