#include "layout/monitor/monitor.h"

#include <algorithm>
#include <utility>

namespace Konveyor::Layout
{

namespace
{

void mergeWorkspace(Workspace &source, Workspace &target)
{
    while (!source.scrolling().isEmpty()) {
        target.addColumn(source.scrolling().detachColumnAt(0, std::nullopt), false, std::nullopt);
    }
    while (!source.floating().isEmpty()) {
        const WindowId id = source.floating().tiles().front().id();
        DetachedTile removed = source.removeTile(id);
        AddTileRequest request;
        request.activate = Activation::Never;
        request.isFloating = true;
        request.width = removed.width;
        request.fillsWidth = removed.fillsWidth;
        target.addTile(std::move(removed.tile), request);
    }
}

}

void Monitor::maintainFixedWorkspaces()
{
    const std::size_t named = static_cast<std::size_t>(
        std::ranges::count_if(m_workspaces, [](const Workspace &workspace) { return !workspace.name().isEmpty(); }));
    const std::size_t count = std::max(static_cast<std::size_t>(m_options->layout.fixedWorkspaceCount), named);
    while (m_workspaces.size() > count) {
        auto removed = std::find_if(
            m_workspaces.rbegin(), m_workspaces.rend(), [](const Workspace &workspace) { return !workspace.isOccupiedOrNamed(); });
        if (removed == m_workspaces.rend()) {
            removed = std::find_if(
                m_workspaces.rbegin(), m_workspaces.rend(), [](const Workspace &workspace) { return workspace.name().isEmpty(); });
        }
        const std::size_t source = static_cast<std::size_t>(std::distance(m_workspaces.begin(), removed.base()) - 1);
        std::size_t target = std::min(count - 1, m_workspaces.size() - 1);
        if (source == target) {
            target = source == 0 ? 1 : source - 1;
        }
        const bool activeRemoved = source == m_activeWorkspaceIndex;
        const auto focused = activeRemoved ? m_workspaces[source].activeWindow() : std::nullopt;
        const WorkspaceId active = activeRemoved ? m_workspaces[target].id() : activeWorkspace().id();
        mergeWorkspace(m_workspaces[source], m_workspaces[target]);
        m_workspaces.erase(m_workspaces.begin() + static_cast<std::ptrdiff_t>(source));
        m_activeWorkspaceIndex = indexOfWorkspace(active).value_or(0);
        if (focused) {
            activeWorkspace().activateWindow(*focused);
        }
        m_transition.reset();
    }
    while (m_workspaces.size() < count) {
        appendEmptyWorkspace();
    }
}

}
