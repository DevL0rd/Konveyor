#pragma once

#include "layout/engine/engine.h"

#include <QHash>
#include <QList>
#include <QRectF>

namespace Konveyor::Layout
{

// Style knobs the painter applies on top of the geometry plan.
inline constexpr double kOverviewScrimAlpha = 0.18;
inline constexpr double kOverviewCardAlpha = 0.14;
inline constexpr double kOverviewCardActiveAlpha = 0.24;
inline constexpr double kOverviewCardHoverAlpha = 0.34;
inline constexpr double kOverviewCardDraggedAlpha = 0.10;
inline constexpr double kOverviewOutlineAlpha = 0.95;

struct OverviewPaintCard
{
    int index = 0;
    QRectF rect;
    bool active = false;
    bool hovered = false;
    bool dragged = false;
    bool empty = false;
    double opacity = 0.0;
    bool operator==(const OverviewPaintCard &) const = default;
};

struct OverviewPaintWindow
{
    WindowId id = 0;
    QRectF rect;
    int workspaceIndex = 0;
    bool selected = false;
    bool dragging = false;
    double opacity = 1.0;
    bool operator==(const OverviewPaintWindow &) const = default;
};

struct OverviewPaintEdge
{
    OverviewSide side = OverviewSide::Left;
    QRectF rect;
    bool operator==(const OverviewPaintEdge &) const = default;
};

struct OverviewPaintView
{
    double blend = 0.0;
    double scrimOpacity = 0.0;
    QList<OverviewPaintCard> cards;
    QList<OverviewPaintWindow> windows;
    QList<OverviewPaintEdge> edges;
    std::optional<QRectF> dropHintRect;
    bool operator==(const OverviewPaintView &) const = default;
};

// Global logical frames of the windows as they would paint without the overview.
using OverviewNormalFrames = QHash<WindowId, QRectF>;

double overviewPaintBlend(double progress);
QRectF overviewPaintRect(const QRectF &from, const QRectF &to, double blend);
QRectF overviewOutlineRect(const QRectF &windowRect, double thickness);
QList<OverviewPaintEdge> overviewPaintEdges(
    const OverviewView &view, Config::OverviewIndicators indicators = Config::OverviewIndicators::Chevrons);
OverviewPaintView overviewPaintView(const OverviewView &view, const OverviewNormalFrames &normalFrames, double progress,
    Config::OverviewIndicators indicators = Config::OverviewIndicators::Chevrons);

}
