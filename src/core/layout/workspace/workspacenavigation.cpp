#include "layout/workspace/workspace.h"

#include "layout/common/geometry.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace Konveyor::Layout
{

bool Workspace::focusLeft()
{
    return isFloatingFocused() ? m_floating.focusLeft() : m_strip.focusLeft();
}

bool Workspace::focusRight()
{
    return isFloatingFocused() ? m_floating.focusRight() : m_strip.focusRight();
}

void Workspace::focusColumnFirst()
{
    if (isFloatingFocused()) {
        m_floating.focusLeftmost();
    } else {
        m_strip.focusColumnFirst();
    }
}

void Workspace::focusColumnLast()
{
    if (isFloatingFocused()) {
        m_floating.focusRightmost();
    } else {
        m_strip.focusColumnLast();
    }
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
    if (!isFloatingFocused()) {
        m_strip.focusWindowInColumn(index);
    }
}

bool Workspace::focusDown()
{
    return isFloatingFocused() ? m_floating.focusDown() : m_strip.focusDown();
}

bool Workspace::focusUp()
{
    return isFloatingFocused() ? m_floating.focusUp() : m_strip.focusUp();
}

void Workspace::focusDownOrLeft()
{
    if (isFloatingFocused()) {
        m_floating.focusDown();
    } else {
        m_strip.focusDownOrLeft();
    }
}

void Workspace::focusDownOrRight()
{
    if (isFloatingFocused()) {
        m_floating.focusDown();
    } else {
        m_strip.focusDownOrRight();
    }
}

void Workspace::focusUpOrLeft()
{
    if (isFloatingFocused()) {
        m_floating.focusUp();
    } else {
        m_strip.focusUpOrLeft();
    }
}

void Workspace::focusUpOrRight()
{
    if (isFloatingFocused()) {
        m_floating.focusUp();
    } else {
        m_strip.focusUpOrRight();
    }
}

void Workspace::focusWindowTop()
{
    if (isFloatingFocused()) {
        m_floating.focusTopmost();
    } else {
        m_strip.focusTop();
    }
}

void Workspace::focusWindowBottom()
{
    if (isFloatingFocused()) {
        m_floating.focusBottommost();
    } else {
        m_strip.focusBottom();
    }
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
    if (!isFloatingFocused()) {
        return m_strip.moveLeft();
    }
    m_floating.moveLeft();
    return true;
}

bool Workspace::moveRight()
{
    if (!isFloatingFocused()) {
        return m_strip.moveRight();
    }
    m_floating.moveRight();
    return true;
}

bool Workspace::moveDown()
{
    if (!isFloatingFocused()) {
        return m_strip.moveDown();
    }
    m_floating.moveDown();
    return true;
}

bool Workspace::moveUp()
{
    if (!isFloatingFocused()) {
        return m_strip.moveUp();
    }
    m_floating.moveUp();
    return true;
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
