#include "layout/monitor/monitor.h"

#include "layout/common/geometry.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace Konveyor::Layout
{

namespace
{

OptionsPtr resolveMonitorOptions(const OptionsPtr &base, const std::optional<Config::Layout> &part)
{
    return makeOptions(withLayoutOverride(*base, part));
}

}

Monitor::Monitor(const OutputArea &area, std::vector<Workspace> workspaces, std::optional<WorkspaceId> toActivate, const Anim::Clock &clock,
    OptionsPtr globalOptions, std::optional<Config::Layout> layoutOverride)
    : m_area(area)
    , m_clock(clock)
    , m_globalOptions(std::move(globalOptions))
    , m_layoutOverride(std::move(layoutOverride))
    , m_options(resolveMonitorOptions(m_globalOptions, m_layoutOverride))
    , m_workspaces(std::move(workspaces))
{
    for (std::size_t idx = 0; idx < m_workspaces.size(); ++idx) {
        m_workspaces[idx].setOutput(m_area);
        m_workspaces[idx].updateConfig(m_options);
        if (toActivate && m_workspaces[idx].id() == *toActivate) {
            m_activeWorkspaceIndex = idx;
        }
    }
    if (m_options->layout.emptyWorkspaceAboveFirst && !m_workspaces.empty()) {
        m_workspaces.insert(m_workspaces.begin(), Workspace(m_area, m_clock, m_options, std::nullopt));
        m_activeWorkspaceIndex += 1;
    }
    m_workspaces.emplace_back(m_area, m_clock, m_options, std::nullopt);
}

std::vector<Workspace> Monitor::releaseWorkspaces()
{
    std::erase_if(m_workspaces, [](const Workspace &workspace) { return !workspace.isOccupiedOrNamed(); });
    for (Workspace &workspace : m_workspaces) {
        workspace.clearOutput();
    }
    return std::move(m_workspaces);
}

void Monitor::applyOptions()
{
    m_options = resolveMonitorOptions(m_globalOptions, m_layoutOverride);
    for (Workspace &workspace : m_workspaces) {
        workspace.updateConfig(m_options);
    }
}

void Monitor::updateConfig(OptionsPtr globalOptions)
{
    const bool wasEmptyAbove = m_options->layout.emptyWorkspaceAboveFirst;
    m_globalOptions = std::move(globalOptions);
    const OptionsPtr updated = resolveMonitorOptions(m_globalOptions, m_layoutOverride);
    if (wasEmptyAbove != updated->layout.emptyWorkspaceAboveFirst && m_workspaces.size() > 1) {
        m_options = updated;
        if (updated->layout.emptyWorkspaceAboveFirst) {
            prependEmptyWorkspace();
        } else if (!m_transition && m_activeWorkspaceIndex != 0) {
            m_workspaces.erase(m_workspaces.begin());
            m_activeWorkspaceIndex -= 1;
        }
    }
    applyOptions();
}

void Monitor::setLayoutOverride(std::optional<Config::Layout> layoutOverride)
{
    if (m_layoutOverride == layoutOverride) {
        return;
    }
    m_layoutOverride = std::move(layoutOverride);
    applyOptions();
}

void Monitor::setArea(const OutputArea &area)
{
    m_area = area;
    for (Workspace &workspace : m_workspaces) {
        workspace.setOutput(m_area);
    }
}

void Monitor::refresh(bool isActive)
{
    for (std::size_t idx = 0; idx < m_workspaces.size(); ++idx) {
        m_workspaces[idx].refresh(isActive && idx == m_activeWorkspaceIndex);
    }
}

void Monitor::tickAnimations()
{
    if (m_transition) {
        if (Anim::Animation *anim = m_transition->animation()) {
            if (anim->isFinished()) {
                m_transition.reset();
                pruneWorkspaces();
            }
        } else if (WorkspaceSwipe *gesture = m_transition->swipe()) {
            if (gesture->edgeScrollLastTime && *gesture->edgeScrollLastTime != m_clock.rawNow()) {
                gesture->edgeScrollLastTime = m_clock.rawNow();
                gesture->edgeScrollActiveSince.reset();
            }
            clearIfDone(gesture->animation);
        }
    }
    for (Workspace &workspace : m_workspaces) {
        workspace.tickAnimations();
    }
}

bool Monitor::isAnimating() const
{
    if (m_transition && m_transition->isAnimating()) {
        return true;
    }
    return std::ranges::any_of(m_workspaces, &Workspace::isAnimating);
}

QString Monitor::checkConsistency() const
{
    if (m_workspaces.empty()) {
        return QStringLiteral("monitor: must have at least one workspace");
    }
    if (m_activeWorkspaceIndex >= m_workspaces.size()) {
        return QStringLiteral("monitor: active workspace index out of range");
    }
    if (m_workspaces.back().isOccupiedOrNamed()) {
        return QStringLiteral("monitor: the last workspace must be empty");
    }
    if (m_options->layout.emptyWorkspaceAboveFirst && m_workspaces.size() > 1 && m_workspaces[0].isOccupiedOrNamed()) {
        return QStringLiteral("monitor: the first workspace must be empty");
    }
    for (const Workspace &workspace : m_workspaces) {
        if (const QString error = workspace.checkConsistency(); !error.isEmpty()) {
            return error;
        }
    }
    return {};
}

bool Monitor::hasWindow(WindowId id) const
{
    return workspaceOfWindow(id).has_value();
}

std::optional<std::size_t> Monitor::indexOfWorkspace(WorkspaceId id) const
{
    for (std::size_t idx = 0; idx < m_workspaces.size(); ++idx) {
        if (m_workspaces[idx].id() == id) {
            return idx;
        }
    }
    return std::nullopt;
}

std::optional<std::size_t> Monitor::workspaceNamed(const QString &name) const
{
    for (std::size_t idx = 0; idx < m_workspaces.size(); ++idx) {
        const QString &workspaceName = m_workspaces[idx].name();
        if (!workspaceName.isEmpty() && workspaceName.compare(name, Qt::CaseInsensitive) == 0) {
            return idx;
        }
    }
    return std::nullopt;
}

std::optional<std::size_t> Monitor::workspaceOfWindow(WindowId id) const
{
    for (std::size_t idx = 0; idx < m_workspaces.size(); ++idx) {
        if (m_workspaces[idx].hasWindow(id)) {
            return idx;
        }
    }
    return std::nullopt;
}

void Monitor::addTile(Tile tile, const MonitorAddRequest &request)
{
    AddTarget workspaceTarget;
    std::size_t workspaceIndex = resolveAddWorkspace(request.target, workspaceTarget);
    const AddTileRequest tileRequest {
        workspaceTarget, request.activate, request.width, request.fillsWidth, request.isFloating, request.anim};
    m_workspaces[workspaceIndex].addTile(std::move(tile), tileRequest);
    ownWorkspace(workspaceIndex);
    addEmptyWorkspacesAround(workspaceIndex);
    if (request.allowActivateWorkspace && resolveActivation(request.activate, false)) {
        activateWorkspace(workspaceIndex);
    }
}

void Monitor::insertIntoColumn(std::size_t workspaceIndex, std::size_t columnIndex, std::optional<std::size_t> tileIndex, Tile tile,
    bool activate, bool allowActivateWorkspace)
{
    m_workspaces[workspaceIndex].insertIntoColumn(columnIndex, tileIndex, std::move(tile), activate);
    ownWorkspace(workspaceIndex);
    if (allowActivateWorkspace && activate) {
        activateWorkspace(workspaceIndex);
    }
}

void Monitor::addColumn(std::size_t workspaceIndex, Column column, bool activate, std::optional<Config::AnimationParams> anim)
{
    m_workspaces[workspaceIndex].addColumn(std::move(column), activate, anim);
    ownWorkspace(workspaceIndex);
    addEmptyWorkspacesAround(workspaceIndex);
    if (activate) {
        activateWorkspace(workspaceIndex);
    }
}

MonitorAddTarget MonitorAddTarget::onWorkspace(WorkspaceId id, std::optional<std::size_t> columnIndex)
{
    return {Kind::Workspace, id, columnIndex, 0};
}

MonitorAddTarget MonitorAddTarget::besideWindow(WindowId id)
{
    return {Kind::NextTo, 0, std::nullopt, id};
}

}
