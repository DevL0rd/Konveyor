#include "layout/floating/floatinglayer.h"

#include "layout/common/geometry.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace Konveyor::Layout
{

FloatingLayer::FloatingLayer(QSizeF viewSize, QRectF workingArea, double scale, const Anim::Clock &clock, OptionsPtr options)
    : m_viewSize(viewSize)
    , m_workingArea(workingArea)
    , m_scale(scale)
    , m_clock(clock)
    , m_options(std::move(options))
{ }

void FloatingLayer::updateConfig(QSizeF viewSize, QRectF workingArea, double scale, OptionsPtr options)
{
    for (std::size_t i = 0; i < m_tiles.size(); ++i) {
        m_tiles[i].updateConfig(viewSize, scale, options);
        m_data[i].update(m_tiles[i]);
        m_data[i].updateWorkingArea(workingArea);
    }
    m_viewSize = viewSize;
    m_workingArea = workingArea;
    m_scale = scale;
    m_options = std::move(options);
}

void FloatingLayer::refresh(bool isActive)
{
    for (Tile &tile : m_tiles) {
        LayoutWindow &window = tile.window();
        window.setActiveInColumn(true);
        window.setFloating(true);
        window.setActivated(isActive && m_activeWindow == window.id());
        std::optional<quint8> resizeData;
        if (m_resize && m_resize->window == window.id()) {
            resizeData = m_resize->edges;
        }
        window.setInteractiveResize(resizeData);
    }
}

void FloatingLayer::tickAnimations()
{
    for (Tile &tile : m_tiles) {
        tile.tickAnimations();
    }
}

bool FloatingLayer::isAnimating() const
{
    return std::ranges::any_of(m_tiles, &Tile::isAnimating);
}

bool FloatingLayer::hasWindow(WindowId id) const
{
    return indexOf(id).has_value();
}

std::optional<std::size_t> FloatingLayer::indexOf(WindowId id) const
{
    const auto it = std::ranges::find_if(m_tiles, [id](const Tile &tile) { return tile.id() == id; });
    if (it == m_tiles.end()) {
        return std::nullopt;
    }
    return static_cast<std::size_t>(std::distance(m_tiles.begin(), it));
}

std::optional<std::size_t> FloatingLayer::targetIndex(std::optional<WindowId> window) const
{
    if (window) {
        return indexOf(*window);
    }
    if (!m_activeWindow) {
        return std::nullopt;
    }
    return indexOf(*m_activeWindow);
}

Tile *FloatingLayer::tileFor(WindowId id)
{
    const auto idx = indexOf(id);
    return idx ? &m_tiles[*idx] : nullptr;
}

void FloatingLayer::addTile(Tile tile, bool activate)
{
    insertTile(0, std::move(tile), activate);
}

void FloatingLayer::insertTile(std::size_t idx, Tile tile, bool activate)
{
    tile.updateConfig(m_viewSize, m_scale, m_options);
    prepareTileSize(tile);

    if (activate || m_tiles.empty()) {
        m_activeWindow = tile.id();
    }

    for (std::size_t i = 0; i < idx && i < m_tiles.size(); ++i) {
        if (tile.window().isChildOf(m_tiles[i].window())) {
            idx = i;
            break;
        }
    }

    const QPointF pos = savedOrDefaultPosition(tile).value_or(centerInArea(m_workingArea, tile.outerSize()));
    FloatingData data {QPointF(), QPointF(), QSizeF(), m_workingArea};
    data.update(tile);
    data.setAbsolutePos(pos);

    const auto offset = static_cast<std::ptrdiff_t>(idx);
    m_data.insert(m_data.begin() + offset, data);
    m_tiles.insert(m_tiles.begin() + offset, std::move(tile));
    raiseChildrenOf(idx);
}

DetachedTile FloatingLayer::removeTile(WindowId id)
{
    const std::size_t idx = *indexOf(id);
    const auto offset = static_cast<std::ptrdiff_t>(idx);
    Tile tile = std::move(m_tiles[idx]);
    const FloatingData data = m_data[idx];
    m_tiles.erase(m_tiles.begin() + offset);
    m_data.erase(m_data.begin() + offset);

    if (m_tiles.empty()) {
        m_activeWindow.reset();
    } else if (m_activeWindow == id) {
        m_activeWindow = m_tiles[0].id();
    }
    if (m_resize && m_resize->window == id) {
        m_resize.reset();
    }

    if (const auto size = tile.window().pendingSize()) {
        tile.savedFloatingSize = size;
    }
    tile.savedFloatingPosition = data.pos;
    const ColumnWidth width = ColumnWidth::fixed(tile.pendingOuterSize().width());
    return DetachedTile {std::move(tile), width, false, true};
}

bool FloatingLayer::activateWindow(WindowId id)
{
    const auto idx = indexOf(id);
    if (!idx) {
        return false;
    }
    raiseWindow(*idx, 0);
    m_activeWindow = id;
    raiseChildrenOf(0);
    return true;
}

bool FloatingLayer::animateOpening(WindowId id)
{
    const auto idx = indexOf(id);
    if (!idx) {
        return false;
    }
    m_tiles[*idx].animateOpening();
    return true;
}

bool FloatingLayer::updateWindow(WindowId id)
{
    const auto idx = indexOf(id);
    if (!idx) {
        return false;
    }
    Tile &tile = m_tiles[*idx];
    FloatingData &data = m_data[*idx];
    const auto edges = tile.window().interactiveResizeEdges();
    const QSizeF prevSize = data.size;

    tile.updateWindow();
    data.update(tile);

    if (!edges) {
        return true;
    }
    QPointF offset;
    if ((*edges & static_cast<quint8>(ResizeEdge::Left)) != 0) {
        offset.rx() += prevSize.width() - data.size.width();
    }
    if ((*edges & static_cast<quint8>(ResizeEdge::Top)) != 0) {
        offset.ry() += prevSize.height() - data.size.height();
    }
    data.setAbsolutePos(data.absolutePos + offset);
    return true;
}

bool FloatingLayer::childrenAdded(WindowId id)
{
    const auto idx = indexOf(id);
    if (!idx) {
        return false;
    }
    raiseChildrenOf(*idx);
    return true;
}

std::vector<ConstTileRef> FloatingLayer::placedTiles(bool animated) const
{
    std::vector<ConstTileRef> result;
    result.reserve(m_tiles.size());
    for (std::size_t i = 0; i < m_tiles.size(); ++i) {
        const QPointF offset = animated ? m_tiles[i].animationOffset() : QPointF();
        result.push_back({&m_tiles[i], roundPoint(m_data[i].absolutePos + offset, m_scale), true});
    }
    return result;
}

std::vector<TileRef> FloatingLayer::renderedTilesMut(bool round)
{
    std::vector<TileRef> result;
    result.reserve(m_tiles.size());
    for (std::size_t i = 0; i < m_tiles.size(); ++i) {
        const QPointF raw = m_data[i].absolutePos + m_tiles[i].animationOffset();
        result.push_back({&m_tiles[i], round ? roundPoint(raw, m_scale) : raw, true});
    }
    return result;
}

std::optional<WindowId> FloatingLayer::windowUnder(QPointF pos) const
{
    for (std::size_t i = 0; i < m_tiles.size(); ++i) {
        const QPointF tilePos = roundPoint(m_data[i].absolutePos + m_tiles[i].animationOffset(), m_scale);
        if (QRectF(tilePos, m_tiles[i].outerSize()).contains(pos)) {
            return m_tiles[i].id();
        }
    }
    return std::nullopt;
}

QString FloatingLayer::verifyTile(std::size_t index) const
{
    if (m_tiles[index].window().requestedMode() != WindowMode::Normal) {
        return QStringLiteral("floating: windows cannot be maximized or fullscreen");
    }
    FloatingData expected = m_data[index];
    expected.update(m_tiles[index]);
    expected.updateWorkingArea(m_workingArea);
    if (!(expected == m_data[index])) {
        return QStringLiteral("floating: tile data must be up to date");
    }
    for (std::size_t other = index + 1; other < m_tiles.size(); ++other) {
        if (m_tiles[other].window().isChildOf(m_tiles[index].window())) {
            return QStringLiteral("floating: children must be stacked above parents");
        }
    }
    return {};
}

QString FloatingLayer::checkConsistency() const
{
    if (!(m_scale > 0.0) || !std::isfinite(m_scale)) {
        return QStringLiteral("floating: scale must be positive and finite");
    }
    if (m_tiles.size() != m_data.size()) {
        return QStringLiteral("floating: tiles and data must have the same length");
    }
    if (m_activeWindow.has_value() != !m_tiles.empty()) {
        return QStringLiteral("floating: active window must be set exactly when tiles are present");
    }
    if (m_activeWindow && !hasWindow(*m_activeWindow)) {
        return QStringLiteral("floating: active window must be present in tiles");
    }
    if (m_resize && !hasWindow(m_resize->window)) {
        return QStringLiteral("floating: interactive resize window must be present in tiles");
    }
    for (std::size_t index = 0; index < m_tiles.size(); ++index) {
        if (const QString error = verifyTile(index); !error.isEmpty()) {
            return error;
        }
    }
    return {};
}

void FloatingLayer::insertAbove(WindowId above, Tile tile, bool activate)
{
    const std::size_t idx = *indexOf(above);
    const QSizeF outerSize = tile.outerSize();
    const QSizeF delta = m_data[idx].size - outerSize;
    const QPointF pos = m_data[idx].absolutePos + QPointF(delta.width() / 2.0, delta.height() / 2.0);
    tile.savedFloatingPosition = absoluteToRelative(keepInsideWorkArea(pos, outerSize));
    insertTile(idx, std::move(tile), activate);
}

void FloatingLayer::raiseChildrenOf(std::size_t idx)
{
    std::vector<std::size_t> descendants;
    for (std::size_t i = m_tiles.size(); i > idx + 1; --i) {
        const LayoutWindow &below = m_tiles[i - 1].window();
        const bool isDescendant = below.isChildOf(m_tiles[idx].window())
            || std::ranges::any_of(descendants, [&](std::size_t other) { return below.isChildOf(m_tiles[other].window()); });
        if (isDescendant) {
            descendants.push_back(i - 1);
        }
    }

    std::ranges::reverse(descendants);
    std::size_t target = idx;
    for (const std::size_t descendant : descendants) {
        raiseWindow(descendant, target);
        target += 1;
    }
}

void FloatingLayer::raiseWindow(std::size_t fromIndex, std::size_t toIndex)
{
    if (fromIndex == toIndex) {
        return;
    }
    const auto from = static_cast<std::ptrdiff_t>(fromIndex);
    const auto to = static_cast<std::ptrdiff_t>(toIndex);
    Tile tile = std::move(m_tiles[fromIndex]);
    const FloatingData data = m_data[fromIndex];
    m_tiles.erase(m_tiles.begin() + from);
    m_data.erase(m_data.begin() + from);
    m_tiles.insert(m_tiles.begin() + to, std::move(tile));
    m_data.insert(m_data.begin() + to, data);
}

}
