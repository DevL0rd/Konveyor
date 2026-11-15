#include "layout/overview/overviewpaint.h"

#include <algorithm>
#include <cmath>

namespace Konveyor::Layout
{

namespace
{

constexpr double kEdgeRatio = 0.012;
constexpr double kEdgeMinimum = 3.0;
constexpr double kEdgeMaximum = 10.0;

QRectF workArea(const OverviewView &view)
{
    return view.outputGeometry.adjusted(view.reserved.left(), view.reserved.top(), -view.reserved.right(), -view.reserved.bottom());
}

QRectF edgeRect(OverviewSide side, const QRectF &area, double span, double margin)
{
    const QPointF center = [&]() {
        switch (side) {
        case OverviewSide::Left:
            return QPointF(area.left() + margin, area.center().y());
        case OverviewSide::Right:
            return QPointF(area.right() - margin, area.center().y());
        case OverviewSide::Top:
            return QPointF(area.center().x(), area.top() + margin);
        case OverviewSide::Bottom:
            return QPointF(area.center().x(), area.bottom() - margin);
        }
        return QPointF();
    }();
    return QRectF(center.x() - span, center.y() - span, span * 2.0, span * 2.0);
}

double cardOpacity(const OverviewPaintCard &card)
{
    if (card.hovered) {
        return kOverviewCardHoverAlpha;
    }
    if (card.dragged) {
        return kOverviewCardDraggedAlpha;
    }
    if (card.active) {
        return kOverviewCardActiveAlpha;
    }
    return kOverviewCardAlpha;
}

}

double overviewPaintBlend(double progress)
{
    if (!std::isfinite(progress)) {
        return 0.0;
    }
    return std::clamp(progress, 0.0, 1.0);
}

QRectF overviewPaintRect(const QRectF &from, const QRectF &to, double blend)
{
    const double t = overviewPaintBlend(blend);
    return QRectF(from.x() + (to.x() - from.x()) * t, from.y() + (to.y() - from.y()) * t, from.width() + (to.width() - from.width()) * t,
        from.height() + (to.height() - from.height()) * t);
}

QRectF overviewOutlineRect(const QRectF &windowRect, double thickness)
{
    if (!std::isfinite(thickness) || thickness <= 0.0) {
        return windowRect;
    }
    return windowRect.adjusted(-thickness, -thickness, thickness, thickness);
}

QList<OverviewPaintEdge> overviewPaintEdges(const OverviewView &view, Config::OverviewIndicators indicators)
{
    QList<OverviewPaintEdge> edges;
    if (indicators == Config::OverviewIndicators::Off) {
        return edges;
    }
    const QRectF area = workArea(view);
    if (area.isEmpty()) {
        return edges;
    }
    const double basis = std::min(area.width(), area.height());
    const double span = std::clamp(basis * 0.018, 12.0, 24.0);
    const double margin = span + 16.0;
    for (const OverviewSide side : view.offscreen) {
        const QRectF rect = edgeRect(side, area, span, margin);
        if (!rect.isEmpty()) {
            edges.append({side, rect});
        }
    }
    return edges;
}

OverviewPaintView overviewPaintView(const OverviewView &view, const OverviewNormalFrames &normalFrames, double progress,
    Config::OverviewIndicators indicators)
{
    OverviewPaintView paint;
    paint.blend = overviewPaintBlend(progress);
    if (paint.blend <= 0.0) {
        return paint;
    }
    paint.scrimOpacity = paint.blend;

    for (qsizetype index = 0; index < view.workspaceRects.size(); ++index) {
        OverviewPaintCard card;
        card.index = static_cast<int>(index);
        card.rect = view.workspaceRects.at(index);
        card.active = card.index == view.activeWorkspace;
        card.hovered = card.index == view.dropWorkspace;
        card.dragged = card.index == view.draggedWorkspace;
        card.opacity = cardOpacity(card) * paint.blend;
        paint.cards.append(card);
    }

    for (const OverviewWindow &window : view.windows) {
        OverviewPaintWindow painted;
        painted.id = window.id;
        painted.workspaceIndex = window.workspaceIndex;
        painted.selected = window.selected;
        painted.dragging = view.draggedWindow == window.id;
        const auto normal = normalFrames.constFind(window.id);
        if (normal != normalFrames.constEnd()) {
            painted.rect = overviewPaintRect(*normal, window.rect, paint.blend);
        } else if (window.normalRect) {
            painted.rect = overviewPaintRect(*window.normalRect, window.rect, paint.blend);
        } else {
            painted.rect = window.rect;
            painted.opacity = painted.dragging ? 1.0 : paint.blend;
        }
        paint.windows.append(painted);
    }

    paint.edges = overviewPaintEdges(view, indicators);
    if (view.dropHintRect) {
        paint.dropHintRect = view.dropHintRect;
    }
    return paint;
}

}
