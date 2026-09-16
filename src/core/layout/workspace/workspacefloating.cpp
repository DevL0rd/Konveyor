#include "layout/workspace/workspace.h"

#include "layout/common/geometry.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace Konveyor::Layout
{

void Workspace::focusFloating()
{
    if (!isFloatingFocused()) {
        switchFocusFloatingTiling();
    }
}

void Workspace::focusTiling()
{
    if (isFloatingFocused()) {
        switchFocusFloatingTiling();
    }
}

void Workspace::switchFocusFloatingTiling()
{
    if (m_floating.isEmpty() || m_strip.isEmpty()) {
        return;
    }
    m_floatingFocus = isFloatingFocused() ? FloatingFocus::No : FloatingFocus::Yes;
}

void Workspace::toggleWindowFloating(std::optional<WindowId> window)
{
    const auto active = activeWindow();
    const bool targetIsActive = !window || window == active;
    const auto target = window ? window : active;
    if (!target || !hasWindow(*target)) {
        return;
    }
    const QPointF renderPos = tileRenderPosition(*target).value_or(QPointF());

    if (m_floating.hasWindow(*target)) {
        DetachedTile removed = m_floating.removeTile(*target);
        m_strip.addTile(std::nullopt, std::move(removed.tile), targetIsActive, removed.width, removed.fillsWidth, std::nullopt);
        if (targetIsActive) {
            m_floatingFocus = FloatingFocus::No;
        }
    } else {
        moveTileToFloating(*target, targetIsActive, renderPos);
    }

    const QPointF newRenderPos = tileRenderPosition(*target).value_or(QPointF());
    for (const TileRef &ref : renderedTilesMut(false)) {
        if (ref.tile->id() == *target) {
            ref.tile->slideFrom(renderPos - newRenderPos);
        }
    }
}

void Workspace::placeWindowFloating(std::optional<WindowId> window, bool floating)
{
    if (targetIsFloating(window) != floating) {
        toggleWindowFloating(window);
    }
}

void Workspace::moveTileToFloating(WindowId window, bool activate, QPointF renderPos)
{
    DetachedTile removed = m_strip.removeTile(window);
    removed.tile.stopSlides();
    if (!m_floating.savedOrDefaultPosition(removed.tile)) {
        const bool centered = m_options->layout.centerFocusedColumn == Config::CenterFocusedColumn::Always;
        const QPointF offset = centered ? QPointF(0.0, 0.0) : QPointF(50.0, 50.0);
        const QPointF pos = m_floating.keepInsideWorkArea(renderPos + offset, removed.tile.outerSize());
        removed.tile.savedFloatingPosition = m_floating.absoluteToRelative(pos);
    }
    m_floating.addTile(std::move(removed.tile), activate);
    if (activate) {
        m_floatingFocus = FloatingFocus::Yes;
    }
}

bool Workspace::shouldRestoreToFloating(WindowId window, bool maximize) const
{
    const Column *column = m_strip.columnFor(window);
    const Tile *tile = m_strip.tileFor(window);
    if (!column || !tile || !tile->returnsToFloating) {
        return false;
    }
    if (maximize) {
        return tile->window().requestedMode() == WindowMode::Maximized;
    }
    return column->fullscreenPending && !column->maximizePending;
}

void Workspace::rememberRestoreToFloating(WindowId window, bool wasNormal, bool restore)
{
    Tile *tile = m_strip.tileFor(window);
    if (tile && wasNormal && tile->window().requestedMode() != WindowMode::Normal) {
        tile->returnsToFloating = restore;
    }
}

void Workspace::storeFloatingPosition(std::optional<WindowId> window, PositionChange x, PositionChange y)
{
    Tile *tile = window ? m_strip.tileFor(*window) : m_strip.activeTile();
    if (!tile) {
        return;
    }
    auto pos = m_floating.savedOrDefaultPosition(*tile);
    if (!pos) {
        if (!isAbsolutePositionChange(x) || !isAbsolutePositionChange(y)) {
            return;
        }
        pos = QPointF(0.0, 0.0);
    }
    tile->savedFloatingPosition = m_floating.absoluteToRelative(applyPositionChange(*pos, m_floating.workingArea(), x, y));
}

void Workspace::moveFloatingWindow(std::optional<WindowId> window, PositionChange x, PositionChange y, bool animate)
{
    if (targetIsFloating(window)) {
        m_floating.moveWindow(window, x, y, animate);
    } else {
        storeFloatingPosition(window, x, y);
    }
}

}
