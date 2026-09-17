#include "store/edithistory.h"

namespace Konveyor::Settings
{

namespace
{

constexpr qint64 BurstMs = 700;
constexpr qsizetype MaxSnapshots = 100;

}

void EditHistory::record(const QString &before)
{
    const bool inBurst = m_lastRecord.isValid() && m_lastRecord.elapsed() < BurstMs;
    m_lastRecord.restart();
    if (inBurst && !m_snapshots.isEmpty()) {
        return;
    }
    if (!m_snapshots.isEmpty() && m_snapshots.last() == before) {
        return;
    }
    m_snapshots.append(before);
    if (m_snapshots.size() > MaxSnapshots) {
        m_snapshots.removeFirst();
    }
}

std::optional<QString> EditHistory::undo()
{
    if (m_snapshots.isEmpty()) {
        return std::nullopt;
    }
    m_lastRecord.invalidate();
    return m_snapshots.takeLast();
}

void EditHistory::clear()
{
    m_snapshots.clear();
    m_lastRecord.invalidate();
}

bool EditHistory::canUndo() const
{
    return !m_snapshots.isEmpty();
}

}
