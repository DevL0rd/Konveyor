#include "layout/overview/overviewpaint.h"

#include <algorithm>
#include <cmath>

namespace Konveyor::Layout
{

namespace
{

QRectF workArea(const OverviewView &view)
{
    return view.outputGeometry.adjusted(view.reserved.left(), view.reserved.top(), -view.reserved.right(), -view.reserved.bottom());
}

QPointF edgeCenter(OverviewSide side, const QRectF &area, double margin)
{
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
}

QRectF edgeRect(OverviewSide side, const QRectF &area, double span, double margin)
{
    const QPointF center = edgeCenter(side, area, margin);
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

QRectF zoomRect(const QRectF &rect, const QPointF &pivot, double factor)
{
    return QRectF(pivot + (rect.topLeft() - pivot) * factor, rect.size() * factor);
}

// The zoom has to land on what the output shows once the overview is closed,
// and the active card holds exactly that. Scaling around the output centre does
// it while the camera keeps that card centred, but a panned or clamped camera
// puts the card elsewhere, so anchor the zoom on the card itself instead.
QPointF zoomAnchor(const OverviewView &view)
{
    const QRectF &output = view.outputGeometry;
    const int active = view.activeWorkspace;
    if (view.scale <= 0.0 || view.scale >= 1.0 || active < 0 || active >= view.workspaceRects.size()) {
        return output.center();
    }
    const QRectF card = view.workspaceRects.at(active);
    const double remaining = 1.0 - view.scale;
    return QPointF((card.left() - output.left() * view.scale) / remaining, (card.top() - output.top() * view.scale) / remaining);
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

OverviewPaintView overviewPaintView(
    const OverviewView &view, const OverviewNormalFrames &normalFrames, double progress, Config::OverviewIndicators indicators)
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
        const int realIdx = index < view.workspaceIndices.size() ? view.workspaceIndices.at(index) : static_cast<int>(index);
        card.empty
            = std::ranges::none_of(view.windows, [realIdx](const OverviewWindow &window) { return window.workspaceIndex == realIdx + 1; });
        card.opacity = cardOpacity(card) * paint.blend;
        paint.cards.append(card);
    }

    const double zoom = view.scale > 0.0 ? (1.0 + (view.scale - 1.0) * paint.blend) / view.scale : 1.0;
    const QPointF pivot = zoomAnchor(view);

    for (const OverviewWindow &window : view.windows) {
        OverviewPaintWindow painted;
        painted.id = window.id;
        painted.workspaceIndex = window.workspaceIndex;
        painted.selected = window.selected;
        painted.dragging = view.draggedWindow == window.id;
        const auto normal = normalFrames.constFind(window.id);
        if (normal != normalFrames.constEnd()) {
            painted.rect = overviewPaintRect(*normal, window.rect, paint.blend);
        } else if (painted.dragging) {
            painted.rect = window.rect;
        } else if (window.normalRect) {
            painted.rect = zoomRect(window.rect, pivot, zoom);
        } else {
            painted.rect = zoomRect(window.rect, pivot, zoom);
            painted.opacity = paint.blend;
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
