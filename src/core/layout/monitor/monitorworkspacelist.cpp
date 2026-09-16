#include "layout/monitor/monitor.h"

#include "layout/common/geometry.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace Konveyor::Layout
{

void Monitor::insertWorkspace(Workspace workspace, std::size_t idx, bool activate)
{
    workspace.setOutput(m_area);
    workspace.updateConfig(m_options);

    if (idx == m_workspaces.size()) {
        idx -= 1;
    }
    if (idx == 0 && m_options->layout.emptyWorkspaceAboveFirst) {
        prependEmptyWorkspace();
        idx += 1;
    }
    m_workspaces.insert(m_workspaces.begin() + static_cast<std::ptrdiff_t>(idx), std::move(workspace));
    if (idx <= m_activeWorkspaceIndex) {
        m_activeWorkspaceIndex += 1;
    }
    if (activate) {
        m_transition.reset();
        activateWorkspace(idx);
    }
    m_transition.reset();
    pruneWorkspaces();
}

void Monitor::insertEmptyWorkspace(std::size_t idx)
{
    m_workspaces.insert(m_workspaces.begin() + static_cast<std::ptrdiff_t>(idx), Workspace(m_area, m_clock, m_options, std::nullopt));
    if (idx <= m_activeWorkspaceIndex) {
        m_activeWorkspaceIndex += 1;
    }
    shiftForInsertion(idx);
}

void Monitor::prependEmptyWorkspace()
{
    insertEmptyWorkspace(0);
}

void Monitor::appendEmptyWorkspace()
{
    insertEmptyWorkspace(m_workspaces.size());
}

void Monitor::appendWorkspaces(std::vector<Workspace> workspaces)
{
    if (workspaces.empty()) {
        return;
    }
    for (Workspace &workspace : workspaces) {
        workspace.setOutput(m_area);
        workspace.updateConfig(m_options);
    }

    const bool emptyWasFocused = m_activeWorkspaceIndex == m_workspaces.size() - 1;
    Workspace empty = std::move(m_workspaces.back());
    m_workspaces.pop_back();
    for (Workspace &workspace : workspaces) {
        m_workspaces.push_back(std::move(workspace));
    }
    m_workspaces.push_back(std::move(empty));

    if (m_options->layout.emptyWorkspaceAboveFirst && m_workspaces[0].isOccupiedOrNamed()) {
        prependEmptyWorkspace();
    }
    if (emptyWasFocused) {
        m_activeWorkspaceIndex = m_workspaces.size() - 1;
    }
    m_transition.reset();
    pruneWorkspaces();
}

void Monitor::addEmptyWorkspacesAround(std::size_t &workspaceIndex)
{
    if (workspaceIndex == m_workspaces.size() - 1) {
        appendEmptyWorkspace();
    }
    if (m_options->layout.emptyWorkspaceAboveFirst && workspaceIndex == 0) {
        prependEmptyWorkspace();
        workspaceIndex += 1;
    }
}

std::size_t Monitor::resolveAddWorkspace(const MonitorAddTarget &target, AddTarget &workspaceTarget) const
{
    switch (target.kind) {
    case MonitorAddTarget::Kind::Workspace:
        if (target.columnIndex) {
            workspaceTarget = AddTarget::atColumn(*target.columnIndex);
        }
        return indexOfWorkspace(target.workspace).value_or(m_activeWorkspaceIndex);
    case MonitorAddTarget::Kind::NextTo:
        workspaceTarget = AddTarget::besideWindow(target.window);
        return workspaceOfWindow(target.window).value_or(m_activeWorkspaceIndex);
    case MonitorAddTarget::Kind::Auto:
        break;
    }
    return m_activeWorkspaceIndex;
}

void Monitor::ownWorkspace(std::size_t idx)
{
    if (m_workspaces[idx].name().isEmpty()) {
        m_workspaces[idx].setHomeOutput(m_area.outputId);
    }
}

std::vector<Workspace> Monitor::takeWorkspacesForOutput(const OutputArea &target)
{
    std::vector<Workspace> taken;
    bool stoppedSwitch = false;
    for (std::size_t i = m_workspaces.size(); i > 0; --i) {
        const std::size_t idx = i - 1;
        if (!outputMatches(target, m_workspaces[idx].homeOutput())) {
            continue;
        }
        Workspace workspace = std::move(m_workspaces[idx]);
        m_workspaces.erase(m_workspaces.begin() + static_cast<std::ptrdiff_t>(idx));
        stoppedSwitch = stoppedSwitch || m_transition.has_value();
        m_transition.reset();
        if (workspace.isOccupiedOrNamed()) {
            workspace.clearOutput();
            taken.push_back(std::move(workspace));
        }
        const bool keepFirstNamed = m_options->layout.emptyWorkspaceAboveFirst && m_activeWorkspaceIndex == 1;
        if (idx <= m_activeWorkspaceIndex && m_activeWorkspaceIndex > 0 && !keepFirstNamed) {
            m_activeWorkspaceIndex -= 1;
        }
    }
    if (m_workspaces.empty()) {
        m_workspaces.emplace_back(m_area, m_clock, m_options, std::nullopt);
        m_activeWorkspaceIndex = 0;
    }
    if (stoppedSwitch || (m_options->layout.emptyWorkspaceAboveFirst && m_workspaces.size() == 2)) {
        pruneWorkspaces();
    }
    std::ranges::reverse(taken);
    return taken;
}

void Monitor::shiftForInsertion(std::size_t idx)
{
    if (m_transition && static_cast<double>(idx) <= m_transition->targetIndex()) {
        m_transition->offset(1);
    }
}

void Monitor::activateWorkspace(std::size_t idx)
{
    selectWorkspaceWith(idx, std::nullopt);
}

void Monitor::selectWorkspaceWith(std::size_t idx, const std::optional<Config::AnimationParams> &config)
{
    const double currentIndex = visibleWorkspacePosition();
    if (m_activeWorkspaceIndex != idx) {
        m_previousWorkspaceId = m_workspaces[m_activeWorkspaceIndex].id();
    }
    const std::size_t previousIndex = m_activeWorkspaceIndex;
    m_activeWorkspaceIndex = idx;
    const Config::AnimationParams params = config.value_or(m_options->animations.workspaceSwitch);

    WorkspaceSwipe *gesture = m_transition ? m_transition->swipe() : nullptr;
    if (gesture && gesture->edgeScrollLastTime) {
        gesture->centerIndex = idx;
        gesture->startIndex = static_cast<double>(idx) - (gesture->currentIndex - gesture->startIndex);
        const double previousCurrent = gesture->currentIndex;
        gesture->currentIndex = static_cast<double>(idx);
        gesture->startAnimationFrom(previousCurrent - gesture->currentIndex, m_clock, params);
        return;
    }
    if (previousIndex == idx) {
        return;
    }
    m_transition = WorkspaceTransition::withAnimation(Anim::Animation(m_clock, currentIndex, static_cast<double>(idx), 0.0, params));
}

bool Monitor::clearWorkspaceName(WorkspaceId id)
{
    const auto idx = indexOfWorkspace(id);
    if (!idx) {
        return false;
    }
    m_workspaces[*idx].unname();
    pruneWorkspaces();
    return true;
}

Workspace Monitor::detachWorkspaceAt(std::size_t idx)
{
    if (idx == m_workspaces.size() - 1) {
        appendEmptyWorkspace();
    }
    if (m_options->layout.emptyWorkspaceAboveFirst && idx == 0) {
        prependEmptyWorkspace();
        idx += 1;
    }

    Workspace workspace = std::move(m_workspaces[idx]);
    m_workspaces.erase(m_workspaces.begin() + static_cast<std::ptrdiff_t>(idx));
    workspace.clearOutput();

    if (idx <= m_activeWorkspaceIndex && m_activeWorkspaceIndex > 0) {
        m_activeWorkspaceIndex -= 1;
    }
    m_transition.reset();
    pruneWorkspaces();
    return workspace;
}

void Monitor::pruneWorkspaces()
{
    if (m_transition) {
        return;
    }
    const std::size_t start = m_options->layout.emptyWorkspaceAboveFirst ? 1 : 0;
    for (std::size_t idx = m_workspaces.size() - 1; idx > start; --idx) {
        const std::size_t target = idx - 1;
        if (target == m_activeWorkspaceIndex || m_workspaces[target].isOccupiedOrNamed()) {
            continue;
        }
        m_workspaces.erase(m_workspaces.begin() + static_cast<std::ptrdiff_t>(target));
        if (m_activeWorkspaceIndex > target) {
            m_activeWorkspaceIndex -= 1;
        }
    }
    if (m_options->layout.emptyWorkspaceAboveFirst && m_workspaces.size() == 2 && !m_workspaces[0].isOccupiedOrNamed()
        && !m_workspaces[1].isOccupiedOrNamed()) {
        m_workspaces.erase(m_workspaces.begin() + 1);
        m_activeWorkspaceIndex = 0;
    }
}

}
