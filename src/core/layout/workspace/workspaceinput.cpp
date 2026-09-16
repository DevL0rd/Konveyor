#include "layout/workspace/workspace.h"

#include "layout/common/geometry.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace Konveyor::Layout
{

void Workspace::beginSwipe(bool isTouchpad)
{
    m_strip.beginSwipe(isTouchpad);
}

std::optional<bool> Workspace::updateSwipe(double deltaX, Anim::Duration timestamp, bool isTouchpad)
{
    return m_strip.updateSwipe(deltaX, timestamp, isTouchpad);
}

bool Workspace::endSwipe(std::optional<bool> isTouchpad, std::optional<WindowId> keepActive)
{
    return m_strip.endSwipe(isTouchpad, keepActive);
}

void Workspace::beginEdgeScroll()
{
    m_strip.beginEdgeScroll();
}

bool Workspace::edgeScrollBy(QPointF pos, double speed)
{
    const double width = m_area.workingArea.width();
    const double x = std::clamp(pos.x() - m_area.workingArea.x(), 0.0, width);
    const double trigger = std::clamp(m_options->gestures.dndEdgeViewScroll.triggerSize, 0.0, width / 2.0);
    if (trigger < 0.01) {
        return m_strip.edgeScrollBy(0.0);
    }
    double delta = 0.0;
    if (x < trigger) {
        delta = -(trigger - x);
    } else if (width - x < trigger) {
        delta = trigger - (width - x);
    }
    return m_strip.edgeScrollBy(delta / trigger * speed);
}

void Workspace::endEdgeScroll()
{
    m_strip.endEdgeScroll();
}

void Workspace::setFloatingFrame(WindowId window, QPointF tilePos, QSizeF windowSize)
{
    m_floating.setFrame(window, tilePos, windowSize);
}

bool Workspace::beginResize(WindowId window, quint8 edges)
{
    if (m_floating.hasWindow(window)) {
        return m_floating.beginResize(window, edges);
    }
    return m_strip.beginResize(window, edges);
}

bool Workspace::updateResize(WindowId window, QPointF delta)
{
    if (m_floating.hasWindow(window)) {
        return m_floating.updateResize(window, delta);
    }
    return m_strip.updateResize(window, delta);
}

void Workspace::endResize(std::optional<WindowId> window)
{
    if (window && m_floating.hasWindow(*window)) {
        m_floating.endResize(window);
        return;
    }
    if (window) {
        m_strip.endResize(window);
        return;
    }
    m_floating.endResize(std::nullopt);
    m_strip.endResize(std::nullopt);
}

}
