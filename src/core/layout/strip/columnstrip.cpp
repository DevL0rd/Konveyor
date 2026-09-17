#include "layout/strip/columnstrip.h"

#include "layout/common/geometry.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace Konveyor::Layout
{

ColumnStrip::ColumnStrip(QSizeF viewSize, QRectF parentArea, double scale, const Anim::Clock &clock, OptionsPtr options)
    : m_area {viewSize, workAreaWithStruts(parentArea, scale, options->layout.struts), parentArea, scale}
    , m_clock(clock)
    , m_options(std::move(options))
{ }

void ColumnStrip::updateConfig(QSizeF viewSize, QRectF parentArea, double scale, OptionsPtr options)
{
    const AreaInfo area {viewSize, workAreaWithStruts(parentArea, scale, options->layout.struts), parentArea, scale};
    const std::optional<Config::PresetSize> previousDefaultWidth = m_options->layout.defaultColumnWidth;
    const bool defaultWidthChanged = previousDefaultWidth != options->layout.defaultColumnWidth;
    for (Column &column : m_columns) {
        column.updateConfig(area, options);
    }
    m_area = area;
    m_options = std::move(options);
    if (defaultWidthChanged && !m_columns.empty() && !m_defaultWidthsPending) {
        m_replacedDefaultWidth = previousDefaultWidth;
        m_defaultWidthsPending = true;
    }
    if (!m_columns.empty() && !m_scroll.isSwiping()) {
        scrollToColumn(std::nullopt, m_activeColumnIndex, std::nullopt);
    }
}

std::optional<std::optional<Config::PresetSize>> ColumnStrip::ruleWidthFor(const Column &column)
{
    for (const Tile &tile : column.tiles) {
        if (const auto &width = tile.window().rules().defaultWidth) {
            return width;
        }
    }
    return std::nullopt;
}

void ColumnStrip::applyDefaultColumnWidths()
{
    m_defaultWidthsPending = false;
    const std::optional<ColumnWidth> replaced
        = m_replacedDefaultWidth ? std::optional(ColumnWidth::fromPreset(*m_replacedDefaultWidth)) : std::nullopt;
    m_replacedDefaultWidth.reset();
    for (Column &column : m_columns) {
        const std::optional<std::optional<Config::PresetSize>> rule = ruleWidthFor(column);
        const std::optional<Config::PresetSize> preset = rule ? *rule : m_options->layout.defaultColumnWidth;
        const bool customized = !rule && replaced && (column.fillsWidth || column.widthSetting != *replaced);
        if (!preset || customized) {
            continue;
        }
        column.widthSetting = ColumnWidth::fromPreset(*preset);
        column.fillsWidth = false;
        column.layoutTiles(false);
    }
}

void ColumnStrip::refresh(bool isActive)
{
    for (std::size_t c = 0; c < m_columns.size(); ++c) {
        Column &column = m_columns[c];
        std::optional<quint8> resizeData;
        if (m_resize && column.contains(m_resize->window)) {
            resizeData = m_resize->edges;
        }
        for (std::size_t t = 0; t < column.tiles.size(); ++t) {
            LayoutWindow &window = column.tiles[t].window();
            const bool activeInColumn = column.activeTileIndex == t;
            window.setActiveInColumn(activeInColumn);
            window.setFloating(false);
            window.setActivated(isActive && m_activeColumnIndex == c && (activeInColumn || column.isTabbed()));
            window.setInteractiveResize(resizeData);
        }
    }
    reconcileView();
}

void ColumnStrip::tickAnimations()
{
    if (const auto *anim = m_scroll.animation(); anim && anim->isFinished()) {
        m_scroll.setIdle(anim->to());
    }
    if (auto *g = m_scroll.swipe()) {
        if (g->edgeScrollLastTime) {
            const Anim::Duration now = m_clock.rawNow();
            if (*g->edgeScrollLastTime != now) {
                g->edgeScrollLastTime = now;
                g->edgeScrollActiveSince.reset();
            }
        }
        clearIfDone(g->animation);
    }
    for (Column &column : m_columns) {
        column.tickAnimations();
    }
}

bool ColumnStrip::isAnimating() const
{
    return m_scroll.isAnimating() || std::ranges::any_of(m_columns, &Column::isAnimating);
}

bool ColumnStrip::hasWindow(WindowId id) const
{
    return locate(id).has_value();
}

std::optional<ColumnStrip::Location> ColumnStrip::locate(WindowId id) const
{
    for (std::size_t c = 0; c < m_columns.size(); ++c) {
        if (const auto t = m_columns[c].position(id)) {
            return Location {c, *t};
        }
    }
    return std::nullopt;
}

std::size_t ColumnStrip::columnIndexOf(WindowId id) const
{
    return locate(id)->column;
}

ColumnStrip::Location ColumnStrip::targetLocation(std::optional<WindowId> window) const
{
    if (window) {
        return *locate(*window);
    }
    return {m_activeColumnIndex, m_columns[m_activeColumnIndex].activeTileIndex};
}

const Column *ColumnStrip::activeColumn() const
{
    return m_columns.empty() ? nullptr : &m_columns[m_activeColumnIndex];
}

const Tile *ColumnStrip::activeTile() const
{
    if (m_columns.empty()) {
        return nullptr;
    }
    const Column &column = m_columns[m_activeColumnIndex];
    return &column.tiles[column.activeTileIndex];
}

Tile *ColumnStrip::activeTile()
{
    return const_cast<Tile *>(std::as_const(*this).activeTile());
}

const Column *ColumnStrip::columnFor(WindowId id) const
{
    const auto location = locate(id);
    return location ? &m_columns[location->column] : nullptr;
}

Column *ColumnStrip::columnById(quint64 columnId)
{
    const auto it = std::ranges::find_if(m_columns, [columnId](const Column &column) { return column.id() == columnId; });
    return it == m_columns.end() ? nullptr : &*it;
}

const Tile *ColumnStrip::tileFor(WindowId id) const
{
    const auto location = locate(id);
    return location ? &m_columns[location->column].tiles[location->tile] : nullptr;
}

Tile *ColumnStrip::tileFor(WindowId id)
{
    return const_cast<Tile *>(std::as_const(*this).tileFor(id));
}

bool ColumnStrip::activeTileWantsFullscreen() const
{
    return !m_columns.empty() && m_columns[m_activeColumnIndex].requestedMode() == WindowMode::Fullscreen;
}

void ColumnStrip::updateWindow(WindowId id)
{
    const auto location = locate(id);
    if (!location) {
        return;
    }
    const std::size_t colIndex = location->column;
    Column &column = m_columns[colIndex];
    const bool wasNormal = column.sizingMode() == WindowMode::Normal;
    const QPointF prevOrigin = column.tileAreaOrigin();
    const auto resizeEdges = column.tiles[location->tile].window().interactiveResizeEdges();
    const double prevWidth = m_columns[colIndex].width();

    column.updateWindow(id);
    column.layoutTiles(false);

    const double offset = prevWidth - m_columns[colIndex].width();
    const bool ongoingResizeAnim = column.tiles[location->tile].resizeAnimation() != nullptr;
    if (offset != 0.0) {
        moveOtherColumnsForResize(colIndex, offset, ongoingResizeAnim);
    }

    Column &updated = m_columns[colIndex];
    const QPointF originDelta = prevOrigin - updated.tileAreaOrigin();
    if (originDelta != QPointF(0.0, 0.0)) {
        for (Tile &tile : updated.tiles) {
            tile.slideFrom(originDelta);
        }
    }

    if (colIndex == m_activeColumnIndex) {
        updateActiveColumnViewAfterUpdate(colIndex, resizeEdges, offset, wasNormal, ongoingResizeAnim);
    }
    placeColumnWithinPins(colIndex);
}

bool ColumnStrip::activateWindow(WindowId id)
{
    const auto location = locate(id);
    if (!location) {
        return false;
    }
    m_columns[location->column].activateWindow(id);
    activateColumn(location->column);
    return true;
}

bool ColumnStrip::animateOpening(WindowId id)
{
    return std::ranges::any_of(m_columns, [id](Column &column) { return column.animateOpening(id); });
}

std::optional<WindowId> ColumnStrip::windowUnder(QPointF pos) const
{
    for (const ColumnRef &ref : renderedColumns()) {
        const Column &column = *ref.column;
        if (column.isTabbed() && column.sizingMode() == WindowMode::Normal) {
            const QPointF colPos = roundPoint(ref.pos, m_area.scale);
            if (const auto idx = column.tabBar.hit(column.tabBarRect(), column.tiles.size(), m_area.scale, pos - colPos)) {
                return column.tiles[*idx].id();
            }
        }
        const std::vector<QPointF> offsets = column.tilePositions();
        for (const std::size_t i : column.renderOrder()) {
            const Tile &tile = column.tiles[i];
            const bool visible = i == column.activeTileIndex || !column.isTabbed();
            const QPointF tilePos = roundPoint(ref.pos + offsets[i] + tile.animationOffset(), m_area.scale);
            if (visible && QRectF(tilePos, tile.outerSize()).contains(pos)) {
                return tile.id();
            }
        }
    }
    return std::nullopt;
}

bool ColumnStrip::drawsAbovePanels() const
{
    if (m_columns.empty() || !m_scroll.isIdle()) {
        return false;
    }
    return m_columns[m_activeColumnIndex].sizingMode() == WindowMode::Fullscreen;
}

}
