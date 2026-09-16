#include "layout/strip/column.h"

#include "layout/common/geometry.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <utility>

namespace Konveyor::Layout
{

namespace
{

std::atomic<quint64> s_nextColumnId {1};

}

Column::Column(Tile tile, const AreaInfo &area, ColumnWidth width, bool fillsWidth)
    : widthSetting(width)
    , fillsWidth(fillsWidth)
    , displayStyle(tile.window().rules().defaultColumnDisplay.value_or(tile.options()->layout.defaultColumnDisplay))
    , tabBar(tile.options()->layout.tabIndicator)
    , m_area(area)
    , m_clock(tile.clock())
    , m_options(tile.options())
    , m_id(s_nextColumnId++)
{
    const auto &presets = m_options->layout.presetColumnWidths;
    const auto match
        = std::ranges::find_if(presets, [&](const Config::PresetSize &preset) { return ColumnWidth::fromPreset(preset) == width; });
    if (match != presets.end()) {
        presetWidthIndex = static_cast<std::size_t>(std::distance(presets.begin(), match));
    }

    const WindowMode pending = tile.window().requestedMode();
    insertTile(0, std::move(tile));
    if (pending == WindowMode::Maximized) {
        setMaximized(true);
    } else if (pending == WindowMode::Fullscreen) {
        setFullscreen(true);
    }

    if (isTabbed() && !m_options->layout.tabIndicator.hideWhenSingleTab && sizingMode() == WindowMode::Normal) {
        tabBar.animateOpening(m_clock, m_options->animations.windowMovement);
    }
}

void TileSizing::update(const Tile &tile)
{
    const auto edges = tile.window().interactiveResizeEdges();
    resizingFromLeft = edges && (*edges & static_cast<quint8>(ResizeEdge::Left)) != 0;
}

void Column::updateConfig(const AreaInfo &area, OptionsPtr options)
{
    const auto &oldLayout = m_options->layout;
    const auto &newLayout = options->layout;
    bool updateSizes = area.viewSize != m_area.viewSize || area.workingArea != m_area.workingArea || area.parentArea != m_area.parentArea;

    if (oldLayout.presetColumnWidths != newLayout.presetColumnWidths) {
        presetWidthIndex.reset();
    }
    if (oldLayout.presetWindowHeights != newLayout.presetWindowHeights) {
        resetHeightsToAuto();
        updateSizes = true;
    }
    updateSizes = updateSizes || oldLayout.gaps != newLayout.gaps || oldLayout.border.enabled != newLayout.border.enabled
        || oldLayout.border.width != newLayout.border.width || oldLayout.tabIndicator != newLayout.tabIndicator;

    for (std::size_t i = 0; i < tiles.size(); ++i) {
        tiles[i].updateConfig(area.viewSize, area.scale, options);
        data[i].update(tiles[i]);
    }
    tabBar.updateConfig(newLayout.tabIndicator);
    m_area = area;
    m_options = std::move(options);

    if (updateSizes) {
        layoutTiles(false);
    }
}

void Column::tickAnimations()
{
    clearIfDone(m_slideX);
    clearIfDone(m_slideY);
    for (Tile &tile : tiles) {
        tile.tickAnimations();
    }
    tabBar.tickAnimations();
}

bool Column::isAnimating() const
{
    return m_slideX || m_slideY || tabBar.isAnimating() || std::ranges::any_of(tiles, &Tile::isAnimating);
}

WindowMode Column::requestedMode() const
{
    if (fullscreenPending) {
        return WindowMode::Fullscreen;
    }
    return maximizePending ? WindowMode::Maximized : WindowMode::Normal;
}

WindowMode Column::sizingMode() const
{
    const auto has
        = [&](WindowMode mode) { return std::ranges::any_of(tiles, [mode](const Tile &tile) { return tile.sizingMode() == mode; }); };
    if (has(WindowMode::Fullscreen)) {
        return WindowMode::Fullscreen;
    }
    return has(WindowMode::Maximized) ? WindowMode::Maximized : WindowMode::Normal;
}

bool Column::animateOpening(WindowId id)
{
    const auto idx = position(id);
    if (!idx) {
        return false;
    }
    tiles[*idx].animateOpening();
    if (isTabbed() && sizingMode() == WindowMode::Normal && tiles.size() == 1 && !tabBar.config().hideWhenSingleTab) {
        tabBar.animateOpening(m_clock, m_options->animations.windowOpen);
    }
    return true;
}

QPointF Column::animationOffset() const
{
    return {optionalOffset(m_slideX), optionalOffset(m_slideY)};
}

void Column::slideFromWith(QPointF from, const Config::AnimationParams &config)
{
    slideXFromWith(from.x(), config);
    m_slideY = startSlide(m_slideY, m_clock, from.y(), config, false);
}

void Column::slideXFrom(double from)
{
    slideXFromWith(from, m_options->animations.windowMovement);
}

void Column::slideXFromWith(double from, const Config::AnimationParams &config)
{
    m_slideX = startSlide(m_slideX, m_clock, from, config, false);
}

void Column::shiftSlideX(double offset)
{
    shiftSlide(m_slideX, offset);
}

std::optional<Config::ColumnPosition> Column::pinnedPosition() const
{
    for (const Tile &tile : tiles) {
        if (const auto &position = tile.window().rules().columnPosition) {
            return position;
        }
    }
    return std::nullopt;
}

bool Column::contains(WindowId id) const
{
    return position(id).has_value();
}

std::optional<std::size_t> Column::position(WindowId id) const
{
    const auto it = std::ranges::find_if(tiles, [id](const Tile &tile) { return tile.id() == id; });
    if (it == tiles.end()) {
        return std::nullopt;
    }
    return static_cast<std::size_t>(std::distance(tiles.begin(), it));
}

bool Column::selectTile(std::size_t idx)
{
    if (activeTileIndex == idx) {
        return false;
    }
    activeTileIndex = idx;
    tiles[idx].ensureFadesToOpaque();
    return true;
}

void Column::activateWindow(WindowId id)
{
    if (const auto idx = position(id)) {
        selectTile(*idx);
    }
}

void Column::insertTile(std::size_t idx, Tile tile)
{
    tile.updateConfig(m_area.viewSize, m_area.scale, m_options);

    std::vector<QPointF> prevOffsets = tilePositions();
    prevOffsets.resize(tiles.size());

    if (!isTabbed()) {
        fullscreenPending = false;
        maximizePending = false;
    }

    TileSizing tileData {WindowHeight::autoWeight(1.0), false};
    tileData.update(tile);
    const auto offset = static_cast<std::ptrdiff_t>(idx);
    data.insert(data.begin() + offset, tileData);
    tiles.insert(tiles.begin() + offset, std::move(tile));
    layoutTiles(true);

    prevOffsets.insert(prevOffsets.begin() + offset, QPointF());
    const std::vector<QPointF> newOffsets = tilePositions();
    for (std::size_t i = 0; i < tiles.size(); ++i) {
        if (i != idx) {
            tiles[i].slideFrom(prevOffsets[i] - newOffsets[i]);
        }
    }
}

void Column::updateWindow(WindowId id)
{
    const auto idx = position(id);
    if (!idx) {
        return;
    }
    Tile &tile = tiles[*idx];
    const double prevHeight = tiles[*idx].pendingOuterSize().height();
    tile.updateWindow();
    data[*idx].update(tile);
    const double offset = prevHeight - tiles[*idx].pendingOuterSize().height();
    if (isTabbed() || offset == 0.0) {
        return;
    }
    const bool resizing = tile.resizeAnimation() != nullptr;
    for (std::size_t i = *idx + 1; i < tiles.size(); ++i) {
        if (resizing) {
            tiles[i].slideYFromWith(offset, m_options->animations.windowResize);
        } else {
            tiles[i].shiftSlideY(offset);
        }
    }
}

void Column::setFullscreen(bool fullscreen)
{
    if (fullscreenPending == fullscreen) {
        return;
    }
    fullscreenPending = fullscreen;
    layoutTiles(true);
}

void Column::setMaximized(bool maximize)
{
    if (maximizePending == maximize) {
        return;
    }
    maximizePending = maximize;
    layoutTiles(true);
}

void Column::setColumnDisplay(Config::ColumnDisplay display)
{
    if (displayStyle == display) {
        return;
    }
    const QPointF prevOrigin = tileAreaOrigin();
    displayStyle = display;
    const QPointF originDelta = prevOrigin - tileAreaOrigin();

    displayStyle = Config::ColumnDisplay::Normal;
    const std::vector<QPointF> offsets = tilePositions();
    const bool toTabbed = display == Config::ColumnDisplay::Tabbed;
    for (std::size_t i = 0; i < tiles.size(); ++i) {
        const double yDelta = offsets[i].y() - prevOrigin.y();
        tiles[i].slideFrom(originDelta + QPointF(0.0, toTabbed ? yDelta : -yDelta));
        if (i != activeTileIndex) {
            tiles[i].fadeOpacity(toTabbed ? 1.0 : 0.0, toTabbed ? 0.0 : 1.0, m_options->animations.windowMovement);
        }
    }
    if (toTabbed) {
        tabBar.animateOpening(m_clock, m_options->animations.windowMovement);
    }
    displayStyle = display;
    layoutTiles(true);
}

void Column::toggleFillWidth()
{
    if (maximizePending) {
        maximizePending = false;
        fillsWidth = false;
    } else {
        fillsWidth = !fillsWidth;
    }
    layoutTiles(true);
}

}
