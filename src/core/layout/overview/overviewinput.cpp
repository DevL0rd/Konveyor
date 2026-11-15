#include "layout/overview/overviewinput.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace Konveyor::Layout
{

namespace
{

bool pointsInDirection(const QPointF &delta, OverviewDirection direction)
{
    switch (direction) {
    case OverviewDirection::Left:
        return delta.x() < -0.5;
    case OverviewDirection::Right:
        return delta.x() > 0.5;
    case OverviewDirection::Up:
        return delta.y() < -0.5;
    case OverviewDirection::Down:
        return delta.y() > 0.5;
    }
    return false;
}

double primaryDistance(const QPointF &delta, OverviewDirection direction)
{
    return direction == OverviewDirection::Left || direction == OverviewDirection::Right ? std::abs(delta.x()) : std::abs(delta.y());
}

double crossDistance(const QPointF &delta, OverviewDirection direction)
{
    return direction == OverviewDirection::Left || direction == OverviewDirection::Right ? std::abs(delta.y()) : std::abs(delta.x());
}

int edgeTarget(const QList<OverviewTarget> &targets, OverviewDirection direction)
{
    int best = 0;
    double bestValue = std::numeric_limits<double>::max();
    for (qsizetype index = 0; index < targets.size(); ++index) {
        const QPointF &center = targets.at(index).center;
        double value = 0.0;
        switch (direction) {
        case OverviewDirection::Left:
            value = center.x();
            break;
        case OverviewDirection::Right:
            value = -center.x();
            break;
        case OverviewDirection::Up:
            value = center.y();
            break;
        case OverviewDirection::Down:
            value = -center.y();
            break;
        }
        if (value < bestValue) {
            bestValue = value;
            best = static_cast<int>(index);
        }
    }
    return best;
}

}

std::optional<int> overviewNextTarget(const QList<OverviewTarget> &targets, std::optional<int> current, OverviewDirection direction)
{
    if (targets.isEmpty()) {
        return std::nullopt;
    }
    if (!current || *current < 0 || *current >= targets.size()) {
        return edgeTarget(targets, direction);
    }
    const QPointF origin = targets.at(*current).center;
    std::optional<int> best;
    double bestScore = std::numeric_limits<double>::max();
    for (qsizetype index = 0; index < targets.size(); ++index) {
        if (static_cast<int>(index) == *current) {
            continue;
        }
        const QPointF delta = targets.at(index).center - origin;
        if (!pointsInDirection(delta, direction)) {
            continue;
        }
        const double score = primaryDistance(delta, direction) + crossDistance(delta, direction) * 1.5;
        if (score < bestScore) {
            bestScore = score;
            best = static_cast<int>(index);
        }
    }
    return best;
}

std::optional<int> overviewCycledTarget(int current, int count, bool forward)
{
    if (count <= 0) {
        return std::nullopt;
    }
    if (current < 0 || current >= count) {
        return forward ? 0 : count - 1;
    }
    return forward ? (current + 1) % count : (current + count - 1) % count;
}

std::optional<int> overviewCardIndexAt(const QList<QRectF> &cards, const QPointF &point)
{
    for (qsizetype index = 0; index < cards.size(); ++index) {
        if (cards.at(index).contains(point)) {
            return static_cast<int>(index);
        }
    }
    return std::nullopt;
}

int overviewNearestCardIndex(const QList<QRectF> &cards, const QPointF &point)
{
    if (cards.isEmpty()) {
        return -1;
    }
    int best = 0;
    double bestDistance = std::numeric_limits<double>::max();
    for (qsizetype index = 0; index < cards.size(); ++index) {
        const QRectF &card = cards.at(index);
        double distance = 0.0;
        if (point.y() < card.top()) {
            distance = card.top() - point.y();
        } else if (point.y() > card.bottom()) {
            distance = point.y() - card.bottom();
        }
        if (distance < bestDistance) {
            bestDistance = distance;
            best = static_cast<int>(index);
        }
    }
    return best;
}

int overviewReorderIndex(const QList<QRectF> &cards, double y)
{
    if (cards.isEmpty()) {
        return -1;
    }
    int slot = 0;
    for (const QRectF &card : cards) {
        if (y >= card.center().y()) {
            ++slot;
        }
    }
    return std::clamp(slot, 0, static_cast<int>(cards.size()) - 1);
}

QPointF overviewPanDelta(Qt::Orientation orientation, double delta, qint32 deltaV120, bool finger, bool shift)
{
    if (delta == 0.0 && deltaV120 == 0) {
        return QPointF();
    }
    constexpr double kWheelStep = 140.0;
    constexpr double kFingerStep = 45.0;
    constexpr double kNotchDegrees = 15.0;
    const double notches = deltaV120 != 0 ? deltaV120 / 120.0 : delta / kNotchDegrees;
    const double distance = notches * (finger ? kFingerStep : kWheelStep);
    const bool horizontal = orientation == Qt::Horizontal || shift;
    return horizontal ? QPointF(distance, 0.0) : QPointF(0.0, distance);
}

}
