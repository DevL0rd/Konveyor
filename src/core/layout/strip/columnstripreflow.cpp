#include "layout/strip/columnstrip.h"

#include <algorithm>

namespace Konveyor::Layout
{

namespace
{

std::size_t indexOf(const std::vector<Column> &columns, quint64 id)
{
    return static_cast<std::size_t>(std::ranges::find(columns, id, &Column::id) - columns.begin());
}

}

void ColumnStrip::markPlacementStack(WindowId window)
{
    for (Column &column : m_columns) {
        if (column.tiles.size() > 1 && std::ranges::any_of(column.tiles, [window](const Tile &tile) { return tile.id() == window; })) {
            column.stackedByPlacement = true;
        }
    }
}

void ColumnStrip::reflowForLayout(const Config::Layout &from, const Config::Layout &to)
{
    const std::optional<WindowId> focus = activeTile() ? std::optional(activeTile()->id()) : std::nullopt;
    const bool wasStacking = from.newWindowPlacement == Config::NewWindowPlacement::Stack;
    const bool stacks = to.newWindowPlacement == Config::NewWindowPlacement::Stack;
    if (wasStacking && !stacks) {
        unstackPlacementColumns(to.defaultColumnWidth);
    } else if (stacks && !wasStacking) {
        stackLooseColumns(static_cast<std::size_t>(std::max(1, to.maxRowsPerColumn)));
    }
    if (focus) {
        activateWindow(*focus);
    }
}

bool ColumnStrip::isLooseColumn(const Column &column) const
{
    if (column.tiles.size() != 1 || column.fillsWidth || column.isTabbed() || column.pinnedPosition()
        || column.requestedMode() != WindowMode::Normal) {
        return false;
    }
    const std::optional<Config::NewWindowPlacement> rule = column.tiles.front().window().rules().newWindowPlacement;
    return !rule || *rule == Config::NewWindowPlacement::Stack;
}

void ColumnStrip::stackLooseColumns(std::size_t maxRows)
{
    std::size_t start = 0;
    while (start < m_columns.size()) {
        std::vector<quint64> run;
        for (std::size_t idx = start; idx < m_columns.size() && isLooseColumn(m_columns[idx]); ++idx) {
            run.push_back(m_columns[idx].id());
        }
        if (run.empty()) {
            ++start;
            continue;
        }
        start = indexOf(m_columns, run.front()) + stackRun(run, maxRows);
    }
}

std::size_t ColumnStrip::stackRun(const std::vector<quint64> &run, std::size_t maxRows)
{
    const std::size_t groups = (run.size() + maxRows - 1) / maxRows;
    std::size_t next = 0;
    for (std::size_t group = 0; group < groups; ++group) {
        const std::size_t size = run.size() / groups + (group < run.size() % groups ? 1 : 0);
        for (std::size_t member = 1; member < size; ++member) {
            DetachedTile moved = detachTileAt(indexOf(m_columns, run[next + member]), 0, std::nullopt);
            insertIntoColumn(indexOf(m_columns, run[next]), std::nullopt, std::move(moved.tile), false);
        }
        m_columns[indexOf(m_columns, run[next])].stackedByPlacement = size > 1;
        next += size;
    }
    return groups;
}

void ColumnStrip::unstackPlacementColumns(const std::optional<Config::PresetSize> &width)
{
    for (std::size_t idx = 0; idx < m_columns.size(); ++idx) {
        if (!m_columns[idx].stackedByPlacement || m_columns[idx].tiles.size() < 2) {
            continue;
        }
        const ColumnWidth columnWidth = width ? ColumnWidth::fromPreset(*width) : m_columns[idx].widthSetting;
        while (m_columns[idx].tiles.size() > 1) {
            DetachedTile moved = detachTileAt(idx, m_columns[idx].tiles.size() - 1, std::nullopt);
            addTile(idx + 1, std::move(moved.tile), false, columnWidth, false, std::nullopt);
        }
        m_columns[idx].stackedByPlacement = false;
    }
}

}
