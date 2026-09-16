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

void Engine::Private::ensureNamedWorkspaces()
{
    for (const Config::NamedWorkspace &named : config.workspaces) {
        bool exists = false;
        for (Workspace *workspace : allWorkspaces()) {
            if (workspace->name().compare(named.name, Qt::CaseInsensitive) == 0) {
                exists = true;
                break;
            }
        }
        if (exists) {
            continue;
        }
        if (monitors.empty()) {
            orphanWorkspaces.insert(orphanWorkspaces.begin(), Workspace(OutputArea(), clock, options, named));
            continue;
        }
        std::size_t monitorIndex = activeMonitorIndex;
        if (named.openOnOutput) {
            monitorIndex = monitorIndexByName(*named.openOnOutput).value_or(0);
        }
        Monitor &monitor = monitors[std::min(monitorIndex, monitors.size() - 1)];
        monitor.insertWorkspace(Workspace(monitor.area(), clock, options, named), 0, false);
    }
}

}
