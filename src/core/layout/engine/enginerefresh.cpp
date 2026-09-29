#include "layout/engine/engineprivate.h"

#include "layout/common/geometry.h"

#include <algorithm>
#include <chrono>
#include <utility>

namespace Konveyor::Layout
{

void Engine::Private::refresh()
{
    for (std::size_t idx = 0; idx < monitors.size(); ++idx) {
        monitors[idx].refresh(layoutFocused && idx == activeMonitorIndex);
    }
    for (Workspace &workspace : orphanWorkspaces) {
        workspace.refresh(false);
    }
    updateFocus();
    resolveRules();
    rememberWindows();
}

void Engine::Private::updateFocus()
{
    Workspace *active = activeWorkspace();
    focused = active && layoutFocused ? active->activeWindow() : std::nullopt;
    for (Workspace *workspace : allWorkspaces()) {
        for (const TileRef &ref : workspace->renderedTilesMut(false)) {
            LayoutWindow &window = ref.tile->window();
            const bool isFocused = focused && *focused == window.id();
            window.setFocused(isFocused);
            if (isFocused) {
                window.setFocusTimestamp(clock.now());
                focusOrder.insert(window.id(), ++focusCounter);
            }
        }
    }
    if (focused == announcedFocus) {
        return;
    }
    announcedFocus = focused;
    if (focused && hooks.focusWindow) {
        hooks.focusWindow(*focused);
    }
}

void Engine::Private::resolveRules()
{
    for (Workspace *workspace : allWorkspaces()) {
        std::vector<WindowId> changed;
        const bool reapplyWidths = resolveWorkspaceRules(*workspace, changed);
        for (const WindowId id : changed) {
            workspace->updateWindow(id);
        }
        if (reapplyWidths) {
            workspace->applyDefaultColumnWidths();
        }
    }
}

bool Engine::Private::resolveWorkspaceRules(Workspace &workspace, std::vector<WindowId> &changed)
{
    const QString profile = monitorProfileName(config, workspace.area());
    const bool force = workspace.defaultWidthsPending();
    for (const TileRef &ref : workspace.renderedTilesMut(false)) {
        LayoutWindow &window = ref.tile->window();
        if (!force && !window.needsRuleRecompute()) {
            continue;
        }
        const MatchContext context {window.properties().appId, window.properties().title, profile, window.isActivated(), window.isFocused(),
            window.isActiveInColumn(), window.isFloating(), window.isUrgent()};
        if (window.setRules(resolveWindowRules(config.windowRules, context, atStartup()))) {
            changed.push_back(window.id());
        }
    }
    return force;
}

namespace
{

std::optional<std::size_t> namedWorkspaceIndex(const std::vector<Workspace> &workspaces, const QString &name)
{
    for (std::size_t idx = 0; idx < workspaces.size(); ++idx) {
        if (workspaces[idx].name().compare(name, Qt::CaseInsensitive) == 0) {
            return idx;
        }
    }
    return std::nullopt;
}

}

void Engine::Private::ensureNamedWorkspaces()
{
    // A missing named workspace goes right below the named workspace listed
    // before it, so the workspaces keep the order of the config.
    std::vector<std::size_t> nextIndex(monitors.size(), 0);
    std::size_t nextOrphanIndex = 0;
    for (const Config::NamedWorkspace &named : config.workspaces) {
        bool exists = false;
        for (std::size_t idx = 0; idx < monitors.size(); ++idx) {
            if (const auto found = namedWorkspaceIndex(monitors[idx].workspaces(), named.name)) {
                nextIndex[idx] = *found + 1;
                exists = true;
            }
        }
        if (const auto found = namedWorkspaceIndex(orphanWorkspaces, named.name)) {
            nextOrphanIndex = *found + 1;
            exists = true;
        }
        if (exists) {
            continue;
        }
        if (monitors.empty()) {
            orphanWorkspaces.insert(
                orphanWorkspaces.begin() + static_cast<std::ptrdiff_t>(nextOrphanIndex), Workspace(OutputArea(), clock, options, named));
            nextOrphanIndex += 1;
            continue;
        }
        std::size_t monitorIndex = activeMonitorIndex;
        if (named.openOnOutput) {
            monitorIndex = monitorIndexByName(*named.openOnOutput).value_or(0);
        }
        monitorIndex = std::min(monitorIndex, monitors.size() - 1);
        Monitor &monitor = monitors[monitorIndex];
        monitor.insertWorkspace(Workspace(monitor.area(), clock, options, named), nextIndex[monitorIndex], false);
        nextIndex[monitorIndex] = namedWorkspaceIndex(monitor.workspaces(), named.name).value_or(0) + 1;
    }
}

}
