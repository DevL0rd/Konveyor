#include "layout/gestures/taptracker.h"

#include <algorithm>
#include <cmath>

namespace Konveyor::Layout
{

TapTracker::TapTracker(double maxTravel)
    : m_maxTravel(maxTravel)
{ }

void TapTracker::down(qint32 id, QPointF position, qint64 timestampMs)
{
    if (m_starts.isEmpty()) {
        m_startMs = timestampMs;
        m_fingers = 0;
        m_valid = true;
        m_pressed = false;
    }
    m_starts.insert(id, position);
    m_fingers = std::max(m_fingers, static_cast<int>(m_starts.size()));
    QPointF sum;
    for (const QPointF &start : std::as_const(m_starts)) {
        sum += start;
    }
    m_centroid = sum / static_cast<double>(m_starts.size());
}

void TapTracker::motion(qint32 id, QPointF position)
{
    const auto start = m_starts.constFind(id);
    if (start == m_starts.constEnd()) {
        return;
    }
    const QPointF moved = position - *start;
    if (std::hypot(moved.x(), moved.y()) > m_maxTravel) {
        m_valid = false;
    }
}

std::optional<int> TapTracker::up(qint32 id, qint64 timestampMs)
{
    if (!m_starts.remove(id) || !m_starts.isEmpty()) {
        return std::nullopt;
    }
    const bool tap = m_valid && timestampMs - m_startMs <= MaxTapMs;
    m_valid = false;
    return tap ? std::optional<int>(m_fingers) : std::nullopt;
}

void TapTracker::cancel()
{
    m_starts.clear();
    m_valid = false;
}

void TapTracker::invalidate()
{
    m_valid = false;
}

void TapTracker::press()
{
    m_pressed = true;
    m_valid = false;
}

}
