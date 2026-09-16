#include "layout/workspace/workspace.h"

#include "layout/common/geometry.h"

#include <algorithm>
#include <atomic>
#include <utility>

namespace Konveyor::Layout
{

namespace
{

std::atomic<quint64> s_nextWorkspaceId {1};

std::optional<Config::Layout> layoutOf(const std::optional<Config::NamedWorkspace> &config)
{
    return config ? config->layout : std::nullopt;
}

OptionsPtr resolveOptions(const OptionsPtr &base, const std::optional<Config::Layout> &part, double scale)
{
    return makeOptions(scaledFor(withLayoutOverride(*base, part), scale));
}

}

bool outputMatches(const OutputArea &area, const QString &reference)
{
    if (reference.isEmpty()) {
        return false;
    }
    return reference.compare(area.outputName, Qt::CaseInsensitive) == 0 || reference.compare(area.outputId, Qt::CaseInsensitive) == 0;
}

Workspace::Workspace(
    const OutputArea &area, const Anim::Clock &clock, OptionsPtr globalOptions, std::optional<Config::NamedWorkspace> config)
    : m_homeOutput(config && config->openOnOutput ? *config->openOnOutput : area.outputId)
    , m_name(config ? config->name : QString())
    , m_area(area)
    , m_clock(clock)
    , m_globalOptions(std::move(globalOptions))
    , m_layoutOverride(layoutOf(config))
    , m_options(resolveOptions(m_globalOptions, m_layoutOverride, area.scale))
    , m_strip(area.viewSize, area.workingArea, area.scale, clock, m_options)
    , m_floating(area.viewSize, area.workingArea, area.scale, clock, m_options)
    , m_id(s_nextWorkspaceId++)
{ }

void Workspace::applyOptions()
{
    m_options = resolveOptions(m_globalOptions, m_layoutOverride, m_area.scale);
    m_strip.updateConfig(m_area.viewSize, m_area.workingArea, m_area.scale, m_options);
    m_floating.updateConfig(m_area.viewSize, m_area.workingArea, m_area.scale, m_options);
}

void Workspace::updateConfig(OptionsPtr globalOptions)
{
    m_globalOptions = std::move(globalOptions);
    applyOptions();
}

void Workspace::setOutput(const OutputArea &area)
{
    if (m_area == area) {
        return;
    }
    m_area = area;
    if (outputMatches(m_area, m_homeOutput)) {
        m_homeOutput = m_area.outputId;
    }
    applyOptions();
}

void Workspace::clearOutput()
{
    m_area.outputName.clear();
    m_area.outputId.clear();
}

bool Workspace::hasWindows() const
{
    return !m_strip.isEmpty() || !m_floating.isEmpty();
}

bool Workspace::isOccupiedOrNamed() const
{
    return hasWindows() || !m_name.isEmpty();
}

bool Workspace::hasWindow(WindowId id) const
{
    return m_strip.hasWindow(id) || m_floating.hasWindow(id);
}

bool Workspace::isUrgent() const
{
    return std::ranges::any_of(tilesWithRenderPositions(), [](const ConstTileRef &ref) { return ref.tile->window().isUrgent(); });
}

void Workspace::tickAnimations()
{
    m_strip.tickAnimations();
    m_floating.tickAnimations();
}

bool Workspace::isAnimating() const
{
    return m_strip.isAnimating() || m_floating.isAnimating();
}

Tile *Workspace::tileFor(WindowId id)
{
    if (Tile *tile = m_strip.tileFor(id)) {
        return tile;
    }
    return m_floating.tileFor(id);
}

Tile Workspace::createTile(LayoutWindow window) const
{
    return Tile(std::move(window), m_area.viewSize, m_area.scale, m_clock, m_options);
}

bool Workspace::isFloatingFocused() const
{
    return m_floatingFocus == FloatingFocus::Yes;
}

bool Workspace::isFloating(WindowId id) const
{
    return m_floating.hasWindow(id);
}

bool Workspace::targetIsFloating(std::optional<WindowId> window) const
{
    return window ? m_floating.hasWindow(*window) : isFloatingFocused();
}

bool Workspace::activeTileWantsFullscreen() const
{
    return m_strip.activeTileWantsFullscreen();
}

bool Workspace::showsFloating() const
{
    return m_floatingFocus != FloatingFocus::No || !m_strip.drawsAbovePanels();
}

std::optional<WindowId> Workspace::activeWindow() const
{
    if (isFloatingFocused()) {
        return m_floating.activeWindow();
    }
    const Tile *tile = m_strip.activeTile();
    return tile ? std::optional<WindowId>(tile->id()) : std::nullopt;
}

void Workspace::addTileAuto(Tile tile, const AddTileRequest &request)
{
    const bool activate = resolveActivation(request.activate, !activeTileWantsFullscreen());
    if (request.isFloating && tile.window().requestedMode() == WindowMode::Normal) {
        m_floating.addTile(std::move(tile), activate);
        if (activate || m_strip.isEmpty()) {
            m_floatingFocus = FloatingFocus::Yes;
        }
        return;
    }
    m_strip.addTile(std::nullopt, std::move(tile), activate, request.width, request.fillsWidth, request.anim);
    if (activate) {
        m_floatingFocus = FloatingFocus::No;
    }
}

void Workspace::addTileFloatingNextTo(Tile tile, const AddTileRequest &request, bool activate)
{
    const WindowId nextTo = request.target.nextTo;
    if (m_floating.hasWindow(nextTo)) {
        m_floating.insertAbove(nextTo, std::move(tile), activate);
    } else {
        const QSizeF outerSize = tile.outerSize();
        QPointF pos = tileRenderPosition(nextTo).value_or(QPointF());
        if (const Tile *other = m_strip.tileFor(nextTo)) {
            const QSizeF delta = other->outerSize() - outerSize;
            pos += QPointF(delta.width() / 2.0, delta.height() / 2.0);
        }
        tile.savedFloatingPosition = m_floating.absoluteToRelative(m_floating.keepInsideWorkArea(pos, outerSize));
        m_floating.addTile(std::move(tile), activate);
    }
    if (activate || m_strip.isEmpty()) {
        m_floatingFocus = FloatingFocus::Yes;
    }
}

void Workspace::addTileNextTo(Tile tile, const AddTileRequest &request)
{
    const WindowId nextTo = request.target.nextTo;
    const bool activate = resolveActivation(request.activate, activeWindow() == nextTo);
    if (request.isFloating && tile.window().requestedMode() == WindowMode::Normal) {
        addTileFloatingNextTo(std::move(tile), request, activate);
        return;
    }
    if (m_floating.hasWindow(nextTo)) {
        m_strip.addTile(std::nullopt, std::move(tile), activate, request.width, request.fillsWidth, request.anim);
    } else {
        m_strip.insertTileAfter(nextTo, std::move(tile), activate, request.width, request.fillsWidth);
    }
    if (activate) {
        m_floatingFocus = FloatingFocus::No;
    }
}

void Workspace::addTile(Tile tile, const AddTileRequest &request)
{
    tile.returnsToFloating = request.isFloating;
    switch (request.target.kind) {
    case AddTarget::Kind::Auto:
        addTileAuto(std::move(tile), request);
        return;
    case AddTarget::Kind::NextTo:
        addTileNextTo(std::move(tile), request);
        return;
    case AddTarget::Kind::NewColumnAt:
        break;
    }
    const bool activate = resolveActivation(request.activate, false);
    m_strip.addTile(request.target.columnIndex, std::move(tile), activate, request.width, request.fillsWidth, request.anim);
    if (activate) {
        m_floatingFocus = FloatingFocus::No;
    }
}

void Workspace::insertIntoColumn(std::size_t columnIndex, std::optional<std::size_t> tileIndex, Tile tile, bool activate)
{
    m_strip.insertIntoColumn(columnIndex, tileIndex, std::move(tile), activate);
    if (activate) {
        m_floatingFocus = FloatingFocus::No;
    }
}

void Workspace::addColumn(Column column, bool activate, std::optional<Config::AnimationParams> anim)
{
    m_strip.addColumn(std::nullopt, std::move(column), activate, anim);
    if (activate) {
        m_floatingFocus = FloatingFocus::No;
    }
}

void Workspace::updateFocusAfterRemoving(bool removedFromFloating)
{
    if (removedFromFloating) {
        if (m_floating.isEmpty()) {
            m_floatingFocus = FloatingFocus::No;
        }
        return;
    }
    if (m_strip.isEmpty() && !m_floating.isEmpty()) {
        m_floatingFocus = FloatingFocus::Yes;
    }
}

DetachedTile Workspace::removeTile(WindowId id)
{
    const bool fromFloating = m_floating.hasWindow(id);
    DetachedTile removed = fromFloating ? m_floating.removeTile(id) : m_strip.removeTile(id);
    updateFocusAfterRemoving(fromFloating);
    return removed;
}

std::optional<Column> Workspace::removeActiveColumn()
{
    if (isFloatingFocused()) {
        return std::nullopt;
    }
    auto column = m_strip.removeActiveColumn();
    if (column) {
        updateFocusAfterRemoving(false);
    }
    return column;
}

bool Workspace::activateWindow(WindowId id)
{
    if (m_floating.activateWindow(id)) {
        m_floatingFocus = FloatingFocus::Yes;
        return true;
    }
    if (m_strip.activateWindow(id)) {
        m_floatingFocus = FloatingFocus::No;
        return true;
    }
    return false;
}

bool Workspace::animateOpening(WindowId id)
{
    return m_strip.animateOpening(id) || m_floating.animateOpening(id);
}

void Workspace::updateWindow(WindowId id)
{
    if (!m_floating.updateWindow(id)) {
        m_strip.updateWindow(id);
    }
}

bool Workspace::childrenAdded(WindowId id)
{
    return m_floating.childrenAdded(id);
}

std::vector<ConstTileRef> Workspace::tilesWithRenderPositions() const
{
    return placedTiles(true);
}

std::vector<ConstTileRef> Workspace::placedTiles(bool animated) const
{
    std::vector<ConstTileRef> result = m_floating.placedTiles(animated);
    const bool visible = showsFloating();
    for (ConstTileRef &ref : result) {
        ref.visible = visible;
    }
    const std::vector<ConstTileRef> scrolling = m_strip.placedTiles(animated);
    result.insert(result.end(), scrolling.begin(), scrolling.end());
    return result;
}

std::vector<TileRef> Workspace::renderedTilesMut(bool round)
{
    std::vector<TileRef> result = m_floating.renderedTilesMut(round);
    const std::vector<TileRef> scrolling = m_strip.renderedTilesMut(round);
    result.insert(result.end(), scrolling.begin(), scrolling.end());
    return result;
}

std::optional<QPointF> Workspace::tileRenderPosition(WindowId id) const
{
    auto &self = const_cast<Workspace &>(*this);
    for (const TileRef &ref : self.renderedTilesMut(false)) {
        if (ref.tile->id() == id) {
            return ref.pos;
        }
    }
    return std::nullopt;
}

std::optional<WindowId> Workspace::windowUnder(QPointF pos) const
{
    if (showsFloating()) {
        if (const auto id = m_floating.windowUnder(pos)) {
            return id;
        }
    }
    return m_strip.windowUnder(pos);
}

DropSlot Workspace::tiledDropSlotAt(QPointF pos, std::optional<Config::ColumnPosition> movingPin) const
{
    return m_strip.dropSlotAt(pos, movingPin);
}

std::optional<QRectF> Workspace::dropSlotRect(DropSlot position) const
{
    return m_strip.dropSlotRect(position);
}

void Workspace::refresh(bool isActive)
{
    m_strip.refresh(isActive && !isFloatingFocused());
    m_floating.refresh(isActive && isFloatingFocused());
}

QString Workspace::checkConsistency() const
{
    if (const QString error = m_strip.checkConsistency(); !error.isEmpty()) {
        return error;
    }
    if (const QString error = m_floating.checkConsistency(); !error.isEmpty()) {
        return error;
    }
    if (m_floating.isEmpty() && isFloatingFocused()) {
        return QStringLiteral("workspace: floating must not be active when empty");
    }
    if (!m_floating.isEmpty() && m_strip.isEmpty() && !isFloatingFocused()) {
        return QStringLiteral("workspace: floating must be active when scrolling is empty");
    }
    if (!(*m_options == *resolveOptions(m_globalOptions, m_layoutOverride, m_area.scale))) {
        return QStringLiteral("workspace: options must be derived from the base options");
    }
    return {};
}

}
