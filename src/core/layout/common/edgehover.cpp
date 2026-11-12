#include "layout/common/edgehover.h"

#include <algorithm>

namespace Konveyor::Layout
{

namespace
{

bool holds(const QRectF &rect, QPointF point)
{
    return point.x() >= rect.left() && point.x() < rect.left() + rect.width() && point.y() >= rect.top()
        && point.y() < rect.top() + rect.height();
}

bool hasNeighbourAt(const QList<QRectF> &outputs, const QRectF &own, QPointF beyond)
{
    return std::ranges::any_of(outputs, [&own, beyond](const QRectF &other) { return other != own && holds(other, beyond); });
}

}

std::optional<HoverSide> hoverSideAt(QPointF pointer, const HoverOutput &output, const QList<QRectF> &outputs,
    Config::EdgeSwitchMonitorEdges monitorEdges, double cornerSize)
{
    const QRectF &geometry = output.geometry;
    const double y = pointer.y();
    const double top = std::max(geometry.top() + cornerSize, output.workArea.top());
    const double bottom = std::min(geometry.top() + geometry.height() - cornerSize, output.workArea.top() + output.workArea.height());
    if (!holds(geometry, pointer) || y < top || y >= bottom) {
        return std::nullopt;
    }
    const double right = geometry.left() + geometry.width();
    std::optional<HoverSide> side;
    QPointF beyond;
    if (pointer.x() < geometry.left() + 1.0) {
        side = HoverSide::Left;
        beyond = QPointF(geometry.left() - 0.5, y);
    } else if (pointer.x() >= right - 1.0) {
        side = HoverSide::Right;
        beyond = QPointF(right + 0.5, y);
    }
    if (side && monitorEdges == Config::EdgeSwitchMonitorEdges::Outer && hasNeighbourAt(outputs, geometry, beyond)) {
        return std::nullopt;
    }
    return side;
}

EdgeHoverTracker::Change EdgeHoverTracker::move(const std::optional<EdgeHoverTarget> &at)
{
    if (at == m_at) {
        return Change::None;
    }
    m_at = at;
    m_spent = false;
    return at ? Change::Entered : Change::Left;
}

std::optional<EdgeHoverTarget> EdgeHoverTracker::take()
{
    if (!m_at || m_spent) {
        return std::nullopt;
    }
    m_spent = true;
    return m_at;
}

void EdgeHoverTracker::reset()
{
    m_at.reset();
    m_spent = false;
}

}
