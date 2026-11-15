#pragma once

#include <QList>
#include <QPointF>
#include <QRectF>
#include <Qt>

#include <optional>

namespace Konveyor::Layout
{

enum class OverviewDirection
{
    Left,
    Right,
    Up,
    Down
};

struct OverviewTarget
{
    QPointF center;
};

// Nearest target strictly in the given direction, or nothing when the direction is empty.
std::optional<int> overviewNextTarget(const QList<OverviewTarget> &targets, std::optional<int> current, OverviewDirection direction);
std::optional<int> overviewCycledTarget(int current, int count, bool forward);

std::optional<int> overviewCardIndexAt(const QList<QRectF> &cards, const QPointF &point);
int overviewNearestCardIndex(const QList<QRectF> &cards, const QPointF &point);
// Card index a pointer at the given vertical position would drop into.
int overviewReorderIndex(const QList<QRectF> &cards, double y);

// Content-space pan for one axis event. Wheel notches move further than the
// smaller touchpad deltas so both devices feel natural.
QPointF overviewPanDelta(Qt::Orientation orientation, double delta, qint32 deltaV120, bool finger, bool shift);

}
