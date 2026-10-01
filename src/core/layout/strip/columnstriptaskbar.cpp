#include "layout/strip/columnstrip.h"

#include <algorithm>
#include <limits>

namespace Konveyor::Layout
{

namespace
{

int taskbarRank(const Column &column, const QHash<WindowId, int> &ranks)
{
    int rank = std::numeric_limits<int>::max();
    if (!column.taskbarEligible()) {
        return rank;
    }
    for (const Tile &tile : column.tiles) {
        rank = std::min(rank, ranks.value(tile.id(), std::numeric_limits<int>::max()));
    }
    return rank;
}

}

bool Column::taskbarEligible() const
{
    const QString app = tiles.front().window().properties().appId;
    return !app.isEmpty() && !pinnedPosition()
        && std::ranges::all_of(tiles, [&](const Tile &tile) { return tile.window().properties().appId == app; });
}

void ColumnStrip::orderTaskbarColumns(const QHash<WindowId, int> &ranks)
{
    if (m_columns.empty() || m_resize) {
        return;
    }
    std::vector<std::size_t> indices;
    std::vector<std::pair<int, std::size_t>> ordered;
    for (std::size_t i = 0; i < m_columns.size(); ++i) {
        const int rank = taskbarRank(m_columns[i], ranks);
        if (rank != std::numeric_limits<int>::max()) {
            indices.push_back(i);
            ordered.emplace_back(rank, i);
        }
    }
    std::ranges::stable_sort(ordered, {}, &std::pair<int, std::size_t>::first);
    bool changed = false;
    for (std::size_t i = 0; i < indices.size(); ++i) {
        changed = changed || indices[i] != ordered[i].second;
    }
    if (!changed) {
        return;
    }
    const WindowId active = activeTile()->id();
    const double activeX = columnOffset(m_activeColumnIndex);
    const auto offsets = columnOffsets();
    std::vector<double> oldX;
    std::vector<Column> moved;
    for (const auto &[rank, index] : ordered) {
        Q_UNUSED(rank)
        oldX.push_back(offsets[index]);
        moved.push_back(std::move(m_columns[index]));
    }
    for (std::size_t i = 0; i < indices.size(); ++i) {
        m_columns[indices[i]] = std::move(moved[i]);
        if (m_columns[indices[i]].contains(active)) {
            m_activeColumnIndex = indices[i];
        }
    }
    for (std::size_t i = 0; i < indices.size(); ++i) {
        m_columns[indices[i]].slideXFrom(oldX[i] - columnOffset(indices[i]));
    }
    m_scroll.offset(activeX - columnOffset(m_activeColumnIndex));
    selectColumnWith(m_activeColumnIndex, m_options->animations.windowMovement);
}

}
