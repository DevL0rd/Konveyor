#include "framerate.h"

#include <algorithm>
#include <cmath>

namespace ProcessMonitor
{

namespace
{

constexpr qint64 Second = 1000000000;
constexpr qint64 History = 30 * Second;
constexpr qint64 ResetGap = 2 * Second;
constexpr qint64 DuplicateThreshold = 500000;

}

std::optional<double> FrameRateTracker::addFrame(qint64 timestampNs)
{
    std::optional<double> frametime;
    if (!m_frames.isEmpty()) {
        const qint64 gap = timestampNs - m_frames.constLast();
        if (gap < DuplicateThreshold) {
            return std::nullopt;
        }
        if (gap > ResetGap) {
            m_frames.clear();
        } else {
            frametime = static_cast<double>(gap) / 1000000.0;
        }
    }
    m_frames.append(timestampNs);
    const auto first = std::lower_bound(m_frames.cbegin(), m_frames.cend(), timestampNs - History);
    if (first != m_frames.cbegin()) {
        m_frames.erase(m_frames.begin(), m_frames.begin() + std::distance(m_frames.cbegin(), first));
    }
    return frametime;
}

std::optional<FrameRate> FrameRateTracker::rateAt(qint64 timestampNs) const
{
    if (m_frames.size() < 2 || timestampNs - m_frames.constLast() > Second) {
        return std::nullopt;
    }
    const auto recent = std::lower_bound(m_frames.cbegin(), m_frames.cend(), timestampNs - Second);
    if (recent == m_frames.cend() || std::distance(recent, m_frames.cend()) < 2) {
        return std::nullopt;
    }
    const qint64 elapsed = m_frames.constLast() - *recent;
    if (elapsed <= 0) {
        return std::nullopt;
    }
    const auto count = std::distance(recent, m_frames.cend()) - 1;
    const double fps = static_cast<double>(count) * Second / elapsed;
    QList<qint64> intervals;
    intervals.reserve(m_frames.size() - 1);
    for (qsizetype i = 1; i < m_frames.size(); ++i) {
        intervals.append(m_frames[i] - m_frames[i - 1]);
    }
    std::sort(intervals.begin(), intervals.end());
    const qsizetype lowIndex = std::min(intervals.size() - 1, static_cast<qsizetype>(std::ceil(intervals.size() * 0.99)) - 1);
    const qint64 lowInterval = intervals[lowIndex];
    return FrameRate {qRound(fps), 1000.0 / fps, lowInterval > 0 ? qRound(static_cast<double>(Second) / lowInterval) : 0};
}

}
