#include "layout/engine/engineprivate.h"

#include "layout/common/geometry.h"

#include <algorithm>
#include <utility>

namespace Konveyor::Layout
{

namespace
{

constexpr int FullscreenStackingIndex = 1000000;
constexpr int BackgroundFullscreenStackingIndex = -1;

ResolvedPaint paintFor(const Config::Border &border, bool urgent, bool active)
{
    const Config::Paint &paint = urgent ? border.urgent : (active ? border.active : border.inactive);
    return {paint.source, paint.color, paint.gradient};
}

DecorationState decorationFor(const Config::Border &border, bool enabled, bool urgent, bool active)
{
    DecorationState state;
    state.enabled = enabled && border.enabled;
    state.width = border.width;
    state.paint = paintFor(border, urgent, active);
    return state;
}

struct TileIndex
{
    int column = 0;
    int tile = 0;
    int stacking = 0;
};

Config::Border tabIndicatorBorder(const Config::TabIndicator &config, const Config::Border &focusRing)
{
    Config::Border border = focusRing;
    border.active = config.active.value_or(focusRing.active);
    border.inactive = config.inactive.value_or(focusRing.inactive);
    border.urgent = config.urgent.value_or(focusRing.urgent);
    return border;
}

std::optional<QSizeF> nonZeroSize(QSize size)
{
    if (size.width() <= 0 && size.height() <= 0) {
        return std::nullopt;
    }
    return QSizeF(size);
}

}

namespace
{

QHash<WindowId, TileIndex> collectIndices(const Workspace &workspace)
{
    QHash<WindowId, TileIndex> indices;
    int stacking = 0;
    const std::vector<Column> &columns = workspace.scrolling().columns();
    for (std::size_t c = 0; c < columns.size(); ++c) {
        for (std::size_t t = 0; t < columns[c].tiles.size(); ++t) {
            indices.insert(columns[c].tiles[t].id(), {static_cast<int>(c), static_cast<int>(t), stacking});
            stacking += 1;
        }
    }
    const std::vector<Tile> &floating = workspace.floating().tiles();
    for (std::size_t i = floating.size(); i > 0; --i) {
        indices.insert(floating[i - 1].id(), {0, static_cast<int>(i - 1), stacking});
        stacking += 1;
    }
    return indices;
}

TabBarState tabBarFor(const Column &column, const Workspace &workspace, QPointF columnOrigin)
{
    TabBarState state;
    if (!column.isTabbed() || !column.tabBar.isShown(column.tiles.size())) {
        return state;
    }
    const double scale = workspace.area().scale;
    const QRectF area = column.tabBarRect();
    state.visible = true;
    state.rect = area.translated(columnOrigin);
    const Config::Border border = tabIndicatorBorder(column.tabBar.config(), workspace.options()->layout.focusRing);
    for (const QRectF &rect : column.tabBar.tabRects(area, column.tiles.size(), scale)) {
        state.tabRects.append(rect.translated(columnOrigin));
    }
    for (std::size_t i = 0; i < column.tiles.size(); ++i) {
        const LayoutWindow &window = column.tiles[i].window();
        state.tabPaints.append(paintFor(border, window.isUrgent(), i == column.activeTileIndex && window.isActivated()));
    }
    return state;
}

}

namespace
{

WindowState buildWindowState(
    const Tile &tile, const Workspace &workspace, const WorkspaceRenderContext &context, QPointF renderPos, QPointF targetPos)
{
    const LayoutWindow &window = tile.window();
    WindowState state;
    state.id = window.id();
    state.output = context.output;
    state.workspace = workspace.id();
    state.workspaceIndex = context.index;
    state.renderFrame = QRectF(context.origin + QPointF(0.0, context.renderY) + renderPos + tile.windowOffset(), tile.visibleWindowSize());
    state.targetFrame
        = QRectF(context.origin + QPointF(0.0, context.targetY) + targetPos + tile.targetWindowOffset(), tile.targetWindowSize());
    const double openProgress = tile.openingAnimation() ? std::clamp(tile.openingAnimation()->valueWithoutOvershoot(), 0.0, 1.0) : 1.0;
    state.renderAlpha = tile.alpha() * openProgress;
    state.ruleOpacity = window.isIgnoringOpacityRule() ? 1.0 : window.rules().opacity.value_or(1.0);
    state.onActiveWorkspace = context.onActive;
    state.isFloating = workspace.isFloating(window.id());
    state.isForceResizable = window.isForceResizable();
    state.isForceResizableByRule = window.isForceResizableByRule();
    state.isExpansionForceResizable = window.isExpansionForceResizable();
    state.isActive = window.isActivated();
    state.isFocused = window.isFocused();
    state.isUrgent = window.isUrgent();
    state.sizingMode = tile.sizingMode();
    state.requestedSizingMode = window.requestedMode();
    state.isWindowedFullscreen = window.windowedFullscreenRequested();
    state.requestedMinSize = nonZeroSize(window.minSize());
    state.requestedMaxSize = nonZeroSize(window.maxSize());
    const bool coversOutput = !state.isWindowedFullscreen
        && (state.sizingMode == WindowMode::Fullscreen || state.requestedSizingMode == WindowMode::Fullscreen);
    state.focusRing = decorationFor(tile.focusRingConfig(), window.isActivated() && !coversOutput, window.isUrgent(), true);
    state.border
        = decorationFor(tile.borderConfig(), tile.borderThickness().has_value() && !coversOutput, window.isUrgent(), window.isActivated());
    state.cornerRadius = coversOutput ? Config::CornerRadius() : window.rules().geometryCornerRadius.value_or(Config::CornerRadius());
    state.clipToGeometry = !coversOutput && window.rules().clipToGeometry.value_or(false);
    return state;
}

WindowState buildMovedWindowState(const Tile &tile, const Workspace &workspace, const WorkspaceRenderContext &context, QPointF location)
{
    WindowState state = buildWindowState(tile, workspace, context, location, location);
    state.visible = true;
    state.isActive = true;
    return state;
}

}

QList<WindowState> Engine::windowStates() const
{
    QList<WindowState> states;
    for (std::size_t monitorIndex = 0; monitorIndex < d->monitors.size(); ++monitorIndex) {
        const Monitor &monitor = d->monitors[monitorIndex];
        const std::vector<double> renderOffsets = monitor.workspaceRenderOffsets();
        const double heightWithGap = monitor.workspaceHeightWithGap();
        const QPointF origin = d->originOf(monitor.outputName());
        const double viewHeight = monitor.area().viewSize.height();

        for (std::size_t idx = 0; idx < monitor.workspaces().size(); ++idx) {
            const Workspace &workspace = monitor.workspaces()[idx];
            WorkspaceRenderContext context;
            context.origin = origin;
            context.renderY = renderOffsets[idx];
            context.targetY = (static_cast<double>(idx) - static_cast<double>(monitor.activeWorkspaceIndex())) * heightWithGap;
            context.output = monitor.outputName();
            context.index = static_cast<int>(idx) + 1;
            context.onActive = idx == monitor.activeWorkspaceIndex();
            context.visible = context.onActive || (context.renderY > -viewHeight && context.renderY < viewHeight);
            d->appendWorkspaceStates(states, workspace, context);
        }
    }
    d->appendMovedWindowState(states);
    return states;
}

void Engine::Private::appendMovedWindowState(QList<WindowState> &states) const
{
    if (!windowDrag || !windowDrag->moving || !windowDrag->tile) {
        return;
    }
    const Tile &tile = *windowDrag->tile;
    const Monitor *monitor = nullptr;
    for (const Monitor &each : monitors) {
        if (each.outputName() == windowDrag->output) {
            monitor = &each;
        }
    }
    WorkspaceRenderContext context;
    context.origin = originOf(windowDrag->output);
    context.output = windowDrag->output;
    context.index = monitor ? static_cast<int>(monitor->activeWorkspaceIndex()) + 1 : 1;
    context.onActive = true;

    const Workspace *workspace = monitor ? &monitor->activeWorkspace() : nullptr;
    if (!workspace) {
        return;
    }
    WindowState state = buildMovedWindowState(tile, *workspace, context, windowDrag->renderLocation());
    state.isFloating = windowDrag->isFloating;
    state.stackingIndex = FullscreenStackingIndex + 1;
    states.append(state);
}

namespace
{

void applyTabBars(QList<WindowState> &states, const Workspace &workspace, const WorkspaceRenderContext &context)
{
    for (const Column &column : workspace.scrolling().columns()) {
        const auto position = column.isTabbed() ? workspace.scrolling().columnRenderPosition(column.id()) : std::nullopt;
        if (!position) {
            continue;
        }
        const QPointF origin = context.origin + QPointF(0.0, context.renderY) + *position;
        const TabBarState indicator = tabBarFor(column, workspace, origin);
        const WindowId active = column.tiles[column.activeTileIndex].id();
        for (WindowState &state : states) {
            if (state.id == active) {
                state.tabBar = indicator;
            }
        }
    }
}

void applyWidthPresetState(WindowState &state, const Tile &tile, const Workspace &workspace, const TileIndex &index)
{
    state.widthPresetCount = workspace.options()->layout.presetColumnWidths.size();
    if (state.isFloating) {
        state.widthPresetIndex
            = tile.floatingWidthPresetIndex ? std::optional(static_cast<int>(*tile.floatingWidthPresetIndex)) : std::nullopt;
        if (const auto closest = workspace.floating().closestWidthPresetIndex(static_cast<std::size_t>(index.tile))) {
            state.nativeWidthSuccessorIndex = static_cast<int>(*closest);
        }
        return;
    }
    const Column *column = workspace.scrolling().columnFor(tile.id());
    if (!column) {
        return;
    }
    if (column->presetWidthIndex) {
        state.widthPresetIndex = static_cast<int>(*column->presetWidthIndex);
    }
    if (const auto closest = column->closestWidthPresetIndex(static_cast<std::size_t>(index.tile))) {
        state.nativeWidthSuccessorIndex = static_cast<int>(*closest);
    }
}

void applyStackingState(WindowState &state, const Tile &tile, const TileIndex &index)
{
    const bool fullscreen = tile.sizingMode() == WindowMode::Fullscreen;
    const int ownIndex = fullscreen ? BackgroundFullscreenStackingIndex : index.stacking;
    state.stackingIndex = fullscreen && state.isActive ? FullscreenStackingIndex : ownIndex;
}

}

void Engine::Private::appendWorkspaceStates(
    QList<WindowState> &states, const Workspace &workspace, const WorkspaceRenderContext &context) const
{
    const QHash<WindowId, TileIndex> indices = collectIndices(workspace);
    QHash<WindowId, QPointF> targets;
    for (const ConstTileRef &ref : workspace.placedTiles(false)) {
        targets.insert(ref.tile->id(), ref.pos);
    }

    for (const ConstTileRef &ref : workspace.placedTiles(true)) {
        const Tile &tile = *ref.tile;
        WindowState state = buildWindowState(tile, workspace, context, ref.pos, targets.value(tile.id(), ref.pos));
        state.visible = context.visible && ref.visible;
        const TileIndex index = indices.value(tile.id());
        state.columnIndex = index.column;
        state.tileIndex = index.tile;
        applyWidthPresetState(state, tile, workspace, index);
        applyStackingState(state, tile, index);
        states.append(state);
    }

    applyTabBars(states, workspace, context);
}

std::optional<WindowState> Engine::windowState(WindowId id) const
{
    const QList<WindowState> states = windowStates();
    const auto it = std::ranges::find(states, id, &WindowState::id);
    if (it == states.end()) {
        return std::nullopt;
    }
    return *it;
}

QList<WorkspaceState> Engine::workspaceStates() const
{
    QList<WorkspaceState> states;
    for (std::size_t monitorIndex = 0; monitorIndex < d->monitors.size(); ++monitorIndex) {
        const Monitor &monitor = d->monitors[monitorIndex];
        for (std::size_t idx = 0; idx < monitor.workspaces().size(); ++idx) {
            const Workspace &workspace = monitor.workspaces()[idx];
            WorkspaceState state;
            state.id = workspace.id();
            state.index = static_cast<int>(idx) + 1;
            state.name = workspace.name();
            state.output = monitor.outputName();
            state.isActive = idx == monitor.activeWorkspaceIndex();
            state.isFocused = state.isActive && monitorIndex == d->activeMonitorIndex;
            state.isUrgent = workspace.isUrgent();
            state.activeWindow = workspace.activeWindow();
            states.append(state);
        }
    }
    return states;
}

QList<OutputState> Engine::outputStates() const
{
    QList<OutputState> states;
    for (const Monitor &monitor : d->monitors) {
        OutputState state;
        state.name = monitor.outputName();
        state.transitionProgress = monitor.transitionProgress();
        const Config::InsertHint &hint = monitor.options()->layout.insertHint;
        state.dropHintPaint = {hint.paint.source, hint.paint.color, hint.paint.gradient};
        state.backgroundColor = monitor.options()->layout.backgroundColor;
        state.dropHint = d->dropHintRect(monitor);
        states.append(state);
    }
    return states;
}

std::optional<QRectF> Engine::Private::dropHintRect(const Monitor &monitor) const
{
    if (!monitor.dropHint || !monitor.options()->layout.insertHint.enabled) {
        return std::nullopt;
    }
    const DropHint &hint = *monitor.dropHint;
    if (!hint.workspace.existing) {
        return std::nullopt;
    }
    const auto idx = monitor.indexOfWorkspace(hint.workspace.id);
    if (!idx) {
        return std::nullopt;
    }
    const auto area = monitor.workspaces()[*idx].dropSlotRect(hint.position);
    if (!area) {
        return std::nullopt;
    }
    const std::vector<double> offsets = monitor.workspaceRenderOffsets();
    return area->translated(originOf(monitor.outputName()) + QPointF(0.0, offsets[*idx]));
}

std::optional<WindowId> Engine::focusedWindow() const
{
    return d->focused;
}

std::optional<WindowId> Engine::windowAt(const QPointF &globalPos) const
{
    for (const Monitor &monitor : d->monitors) {
        const QPointF origin = d->originOf(monitor.outputName());
        const QRectF geometry(origin, monitor.area().viewSize);
        if (!geometry.contains(globalPos)) {
            continue;
        }
        const QPointF local = globalPos - origin;
        const std::vector<double> offsets = monitor.workspaceRenderOffsets();
        for (std::size_t idx = 0; idx < monitor.workspaces().size(); ++idx) {
            const auto id = monitor.workspaces()[idx].windowUnder(local - QPointF(0.0, offsets[idx]));
            if (id) {
                return id;
            }
        }
    }
    return std::nullopt;
}

QString Engine::checkConsistency() const
{
    for (const Monitor &monitor : d->monitors) {
        if (const QString error = monitor.checkConsistency(); !error.isEmpty()) {
            return error;
        }
    }
    if (!d->monitors.empty() && d->activeMonitorIndex >= d->monitors.size()) {
        return QStringLiteral("engine: active monitor index out of range");
    }
    for (const Workspace &workspace : d->orphanWorkspaces) {
        if (const QString error = workspace.checkConsistency(); !error.isEmpty()) {
            return error;
        }
    }
    return {};
}

}
