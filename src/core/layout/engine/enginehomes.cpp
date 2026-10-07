#include "layout/engine/engineprivate.h"

#include <algorithm>
#include <utility>

namespace Konveyor::Layout
{

std::optional<std::size_t> Engine::Private::homeMonitorOf(const Workspace &workspace) const
{
    for (std::size_t idx = 0; idx < monitors.size(); ++idx) {
        if (workspace.homeAffinity(monitors[idx].area()) == 2) {
            return idx;
        }
    }
    return std::nullopt;
}

void Engine::Private::sendHome(std::vector<Workspace> workspaces)
{
    if (monitors.empty()) {
        for (Workspace &workspace : workspaces) {
            workspace.markHomeUnplugged();
            workspace.updateConfig(options);
        }
        orphanWorkspaces = std::move(workspaces);
        return;
    }
    std::vector<std::vector<Workspace>> byMonitor(monitors.size());
    for (Workspace &workspace : workspaces) {
        const std::optional<std::size_t> home = homeMonitorOf(workspace);
        if (!home) {
            workspace.markHomeUnplugged();
        }
        byMonitor[home.value_or(0)].push_back(std::move(workspace));
    }
    for (std::size_t idx = 0; idx < monitors.size(); ++idx) {
        monitors[idx].appendWorkspaces(std::move(byMonitor[idx]));
    }
}

void Engine::Private::moveHome(WorkspaceId id)
{
    for (std::size_t from = 0; from < monitors.size(); ++from) {
        const std::optional<std::size_t> idx = monitors[from].indexOfWorkspace(id);
        if (!idx) {
            continue;
        }
        const std::optional<std::size_t> home = homeMonitorOf(monitors[from].workspaces()[*idx]);
        if (home && *home != from) {
            Workspace moved = monitors[from].detachWorkspaceAt(*idx);
            const std::size_t slot = namedWorkspaceSlot(monitors[*home].workspaces(), moved.name());
            monitors[*home].insertWorkspace(std::move(moved), slot, false);
        }
        return;
    }
}

void Engine::Private::applyNamedWorkspaceHomes(const QList<Config::NamedWorkspace> &previous)
{
    for (const Config::NamedWorkspace &named : config.workspaces) {
        const auto before = std::ranges::find_if(
            previous, [&named](const Config::NamedWorkspace &entry) { return entry.name.compare(named.name, Qt::CaseInsensitive) == 0; });
        if (before == previous.end() || before->openOnOutput == named.openOnOutput) {
            continue;
        }
        for (Workspace *workspace : allWorkspaces()) {
            if (workspace->name().compare(named.name, Qt::CaseInsensitive) != 0) {
                continue;
            }
            if (named.openOnOutput) {
                workspace->setConfiguredHome(*named.openOnOutput);
            } else {
                workspace->makeHome(workspace->area());
            }
            moveHome(workspace->id());
            break;
        }
    }
}

}
