#include "layout/monitor/monitor.h"

#include "layout/common/geometry.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace Konveyor::Layout
{

void Monitor::focusWindowOrWorkspaceDown()
{
    if (!activeWorkspace().focusDown()) {
        switchWorkspaceDown();
    }
}

void Monitor::focusWindowOrWorkspaceUp()
{
    if (!activeWorkspace().focusUp()) {
        switchWorkspaceUp();
    }
}

void Monitor::switchWorkspaceUp()
{
    const WorkspaceSwipe *gesture = m_transition ? m_transition->swipe() : nullptr;
    if (gesture && gesture->edgeScrollLastTime) {
        const double value = std::clamp(std::ceil(gesture->currentIndex) - 1.0, 0.0, static_cast<double>(m_workspaces.size() - 1));
        activateWorkspace(static_cast<std::size_t>(value));
        return;
    }
    activateWorkspace(m_activeWorkspaceIndex > 0 ? m_activeWorkspaceIndex - 1 : 0);
}

void Monitor::switchWorkspaceDown()
{
    const WorkspaceSwipe *gesture = m_transition ? m_transition->swipe() : nullptr;
    if (gesture && gesture->edgeScrollLastTime) {
        const double value = std::clamp(std::floor(gesture->currentIndex) + 1.0, 0.0, static_cast<double>(m_workspaces.size() - 1));
        activateWorkspace(static_cast<std::size_t>(value));
        return;
    }
    activateWorkspace(std::min(m_activeWorkspaceIndex + 1, m_workspaces.size() - 1));
}

std::optional<std::size_t> Monitor::previousWorkspaceIndex() const
{
    if (!m_previousWorkspaceId) {
        return std::nullopt;
    }
    return indexOfWorkspace(*m_previousWorkspaceId);
}

void Monitor::switchWorkspace(std::size_t idx)
{
    activateWorkspace(std::min(idx, m_workspaces.size() - 1));
}

void Monitor::goToWorkspaceOrBack(std::size_t idx)
{
    const std::size_t target = std::min(idx, m_workspaces.size() - 1);
    if (target != m_activeWorkspaceIndex) {
        switchWorkspace(target);
        return;
    }
    if (const auto previous = previousWorkspaceIndex()) {
        switchWorkspace(*previous);
    }
}

void Monitor::switchWorkspacePrevious()
{
    if (const auto previous = previousWorkspaceIndex()) {
        switchWorkspace(*previous);
    }
}

void Monitor::moveDownOrToWorkspaceDown()
{
    if (!activeWorkspace().moveDown()) {
        moveToWorkspaceDown(Activation::Smart);
    }
}

void Monitor::moveUpOrToWorkspaceUp()
{
    if (!activeWorkspace().moveUp()) {
        moveToWorkspaceUp(Activation::Smart);
    }
}

void Monitor::moveToWorkspaceUp(Activation activate)
{
    moveToWorkspace(std::nullopt, m_activeWorkspaceIndex > 0 ? m_activeWorkspaceIndex - 1 : 0, activate);
}

void Monitor::moveToWorkspaceDown(Activation activate)
{
    moveToWorkspace(std::nullopt, std::min(m_activeWorkspaceIndex + 1, m_workspaces.size() - 1), activate);
}

void Monitor::animateTileBetweenWorkspaces(
    WindowId window, QPointF oldRenderPos, std::size_t sourceIndex, std::size_t newIndex, const Config::AnimationParams &config)
{
    oldRenderPos.ry() += workspaceHeightWithGap() * (static_cast<double>(sourceIndex) - static_cast<double>(newIndex));
    for (const TileRef &ref : m_workspaces[newIndex].renderedTilesMut(false)) {
        if (ref.tile->id() == window) {
            ref.tile->slideFromWith(oldRenderPos - ref.pos, config);
            return;
        }
    }
}

void Monitor::moveToWorkspace(std::optional<WindowId> window, std::size_t idx, Activation activate)
{
    const auto sourceIndex = window ? workspaceOfWindow(*window) : std::optional<std::size_t>(m_activeWorkspaceIndex);
    if (!sourceIndex) {
        return;
    }
    const std::size_t newIndex = std::min(idx, m_workspaces.size() - 1);
    if (newIndex == *sourceIndex) {
        return;
    }
    const WorkspaceId sourceId = m_workspaces[*sourceIndex].id();
    const WorkspaceId newId = m_workspaces[newIndex].id();
    const auto target = window ? window : m_workspaces[*sourceIndex].activeWindow();
    if (!target) {
        return;
    }
    const bool shouldActivate = resolveActivation(activate, !window || activeWorkspace().activeWindow() == window);

    const QPointF oldRenderPos = m_workspaces[*sourceIndex].tileRenderPosition(*target).value_or(QPointF());
    DetachedTile removed = m_workspaces[*sourceIndex].removeTile(*target);
    const Config::AnimationParams config = shouldActivate ? m_options->animations.workspaceSwitch : m_options->animations.windowMovement;

    MonitorAddRequest request;
    request.target = MonitorAddTarget::onWorkspace(newId);
    request.activate = shouldActivate ? Activation::Always : Activation::Never;
    request.width = removed.width;
    request.fillsWidth = removed.fillsWidth;
    request.isFloating = removed.isFloating;
    request.anim = config;
    addTile(std::move(removed.tile), request);
    pruneWorkspaces();

    const auto finalIndex = indexOfWorkspace(newId);
    const auto finalSourceIndex = indexOfWorkspace(sourceId);
    if (finalIndex && finalSourceIndex) {
        animateTileBetweenWorkspaces(*target, oldRenderPos, *finalSourceIndex, *finalIndex, config);
    }
}

void Monitor::moveColumnToWorkspaceUp(bool activate)
{
    moveColumnToWorkspace(m_activeWorkspaceIndex > 0 ? m_activeWorkspaceIndex - 1 : 0, activate);
}

void Monitor::moveColumnToWorkspaceDown(bool activate)
{
    moveColumnToWorkspace(std::min(m_activeWorkspaceIndex + 1, m_workspaces.size() - 1), activate);
}

void Monitor::moveColumnToWorkspace(std::size_t idx, bool activate)
{
    const std::size_t sourceIndex = m_activeWorkspaceIndex;
    const std::size_t newIndex = std::min(idx, m_workspaces.size() - 1);
    if (newIndex == sourceIndex) {
        return;
    }
    if (m_workspaces[sourceIndex].isFloatingFocused()) {
        moveToWorkspace(std::nullopt, idx, activate ? Activation::Smart : Activation::Never);
        return;
    }

    const Column *active = m_workspaces[sourceIndex].scrolling().activeColumn();
    if (!active) {
        return;
    }
    const quint64 columnId = active->id();
    QPointF oldRenderPos = m_workspaces[sourceIndex].scrolling().columnRenderPosition(columnId).value_or(QPointF());
    auto column = m_workspaces[sourceIndex].removeActiveColumn();
    if (!column) {
        return;
    }
    oldRenderPos.ry() += workspaceHeightWithGap() * (static_cast<double>(sourceIndex) - static_cast<double>(newIndex));

    const Config::AnimationParams config = activate ? m_options->animations.workspaceSwitch : m_options->animations.windowMovement;
    const WorkspaceId newId = m_workspaces[newIndex].id();
    addColumn(newIndex, std::move(*column), activate, config);

    const auto finalIndex = indexOfWorkspace(newId);
    if (!finalIndex) {
        return;
    }
    ColumnStrip &scrolling = m_workspaces[*finalIndex].scrolling();
    Column *moved = scrolling.columnById(columnId);
    const auto newPos = scrolling.columnRenderPosition(columnId);
    if (moved && newPos) {
        moved->slideFromWith(oldRenderPos - *newPos, config);
    }
}

void Monitor::moveWorkspaceDown()
{
    std::size_t newIndex = std::min(m_activeWorkspaceIndex + 1, m_workspaces.size() - 1);
    if (newIndex == m_activeWorkspaceIndex) {
        return;
    }
    std::swap(m_workspaces[m_activeWorkspaceIndex], m_workspaces[newIndex]);
    if (newIndex == m_workspaces.size() - 1) {
        appendEmptyWorkspace();
    }
    if (m_options->layout.emptyWorkspaceAboveFirst && m_activeWorkspaceIndex == 0) {
        prependEmptyWorkspace();
        newIndex += 1;
    }

    const auto previous = m_previousWorkspaceId;
    activateWorkspace(newIndex);
    m_transition.reset();
    m_previousWorkspaceId = previous;
    pruneWorkspaces();
}

void Monitor::moveWorkspaceUp()
{
    std::size_t newIndex = m_activeWorkspaceIndex > 0 ? m_activeWorkspaceIndex - 1 : 0;
    if (newIndex == m_activeWorkspaceIndex) {
        return;
    }
    std::swap(m_workspaces[m_activeWorkspaceIndex], m_workspaces[newIndex]);
    if (m_activeWorkspaceIndex == m_workspaces.size() - 1) {
        appendEmptyWorkspace();
    }
    if (m_options->layout.emptyWorkspaceAboveFirst && newIndex == 0) {
        prependEmptyWorkspace();
        newIndex += 1;
    }

    const auto previous = m_previousWorkspaceId;
    activateWorkspace(newIndex);
    m_transition.reset();
    m_previousWorkspaceId = previous;
    pruneWorkspaces();
}

void Monitor::moveWorkspaceToIndex(std::size_t oldIndex, std::size_t newIndex)
{
    if (oldIndex >= m_workspaces.size()) {
        return;
    }
    newIndex = std::min(newIndex, m_workspaces.size() - 1);
    if (oldIndex == newIndex) {
        return;
    }

    Workspace workspace = std::move(m_workspaces[oldIndex]);
    m_workspaces.erase(m_workspaces.begin() + static_cast<std::ptrdiff_t>(oldIndex));
    m_workspaces.insert(m_workspaces.begin() + static_cast<std::ptrdiff_t>(newIndex), std::move(workspace));

    const bool movedDown = newIndex > oldIndex;
    if ((movedDown && newIndex == m_workspaces.size() - 1) || (!movedDown && oldIndex == m_workspaces.size() - 1)) {
        appendEmptyWorkspace();
    }
    if (m_options->layout.emptyWorkspaceAboveFirst && (movedDown ? oldIndex == 0 : newIndex == 0)) {
        prependEmptyWorkspace();
        newIndex += 1;
    }

    if (m_activeWorkspaceIndex == oldIndex) {
        m_activeWorkspaceIndex = newIndex;
    } else if (newIndex <= m_activeWorkspaceIndex && oldIndex > m_activeWorkspaceIndex) {
        m_activeWorkspaceIndex += 1;
    } else if (newIndex >= m_activeWorkspaceIndex && oldIndex < m_activeWorkspaceIndex && m_activeWorkspaceIndex > 0) {
        m_activeWorkspaceIndex -= 1;
    }

    m_transition.reset();
    pruneWorkspaces();
}

}
