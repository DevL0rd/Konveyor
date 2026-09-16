#include "layout/strip/columnstrip.h"

#include <algorithm>

namespace Konveyor::Layout
{

namespace
{

std::optional<std::size_t> indexOfColumn(const std::vector<Column> &columns, quint64 id)
{
    const auto it = std::ranges::find(columns, id, &Column::id);
    if (it == columns.end()) {
        return std::nullopt;
    }
    return static_cast<std::size_t>(it - columns.begin());
}

}

std::vector<std::size_t> ColumnStrip::appColumnIndices(const QString &appId) const
{
    std::vector<std::size_t> indices;
    if (appId.isEmpty()) {
        return indices;
    }
    for (std::size_t idx = 0; idx < m_columns.size(); ++idx) {
        const std::vector<Tile> &tiles = m_columns[idx].tiles;
        const bool allFromApp
            = std::ranges::all_of(tiles, [&appId](const Tile &tile) { return tile.window().properties().appId == appId; });
        if (allFromApp && !tiles.empty()) {
            indices.push_back(idx);
        }
    }
    return indices;
}

void ColumnStrip::addToAppStack(const std::vector<std::size_t> &appColumns, Tile tile, bool activate, std::size_t maxRows)
{
    for (const std::size_t idx : appColumns) {
        if (m_columns[idx].tiles.size() < maxRows) {
            insertIntoColumn(idx, std::nullopt, std::move(tile), activate);
            return;
        }
    }

    const WindowId added = tile.id();
    const std::optional<WindowId> previouslyActive = activeTile() ? std::optional(activeTile()->id()) : std::nullopt;
    std::vector<quint64> ids;
    ids.reserve(appColumns.size() + 1);
    for (const std::size_t idx : appColumns) {
        ids.push_back(m_columns[idx].id());
    }
    const ColumnWidth width = m_columns[appColumns.back()].widthSetting;
    addTile(appColumns.back() + 1, std::move(tile), activate, width, false, std::nullopt);
    ids.push_back(columnFor(added)->id());

    std::size_t total = 0;
    for (const quint64 id : ids) {
        total += m_columns[*indexOfColumn(m_columns, id)].tiles.size();
    }
    for (std::size_t i = 0; i + 1 < ids.size(); ++i) {
        const std::size_t target = total / ids.size() + (i < total % ids.size() ? 1 : 0);
        while (m_columns[*indexOfColumn(m_columns, ids[i])].tiles.size() > target) {
            const std::size_t from = *indexOfColumn(m_columns, ids[i]);
            DetachedTile moved = detachTileAt(from, m_columns[from].tiles.size() - 1, std::nullopt);
            insertIntoColumn(*indexOfColumn(m_columns, ids[i + 1]), 0, std::move(moved.tile), false);
        }
    }

    const std::optional<WindowId> focus = activate ? std::optional(added) : previouslyActive;
    if (focus) {
        activateWindow(*focus);
    }
}

}
