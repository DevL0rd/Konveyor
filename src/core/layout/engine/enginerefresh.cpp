#include "layout/engine/engineprivate.h"

#include "layout/common/geometry.h"

#include <algorithm>
#include <chrono>
#include <utility>

namespace Konveyor::Layout
{

void Engine::Private::refreshWorkspaces()
{
    for (std::size_t idx = 0; idx < monitors.size(); ++idx) {
        monitors[idx].refresh(layoutFocused && idx == activeMonitorIndex);
    }
    for (Workspace &workspace : orphanWorkspaces) {
        workspace.refresh(false);
    }
}

void Engine::Private::refresh()
{
    followWindowsOnAllDesktops();
    refreshWorkspaces();
    updateFocus();
    if (resolveRules()) {
        refreshWorkspaces();
    }
    rememberWindows();
}

void Engine::Private::takeLayoutFocus()
{
    layoutFocused = true;
    Workspace *active = activeWorkspace();
    focused = active ? active->activeWindow() : std::nullopt;
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

bool Engine::Private::resolveRules()
{
    if (rulesAtStartup != atStartup()) {
        rulesAtStartup = atStartup();
        for (Workspace *workspace : allWorkspaces()) {
            for (const TileRef &ref : workspace->renderedTilesMut(false)) {
                ref.tile->window().markRulesDirty();
            }
        }
    }
    bool anyChanged = false;
    for (Workspace *workspace : allWorkspaces()) {
        std::vector<WindowId> changed;
        const bool reapplyWidths = resolveWorkspaceRules(*workspace, changed);
        for (const WindowId id : changed) {
            workspace->updateWindow(id);
        }
        if (reapplyWidths) {
            workspace->applyDefaultColumnWidths();
        }
        anyChanged = anyChanged || reapplyWidths || !changed.empty();
    }
    return anyChanged;
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

void Engine::Private::forgetRemovedWorkspaceNames(const QList<Config::NamedWorkspace> &previous)
{
    const auto listed = [](const QList<Config::NamedWorkspace> &named, const QString &name) {
        return std::ranges::any_of(
            named, [&name](const Config::NamedWorkspace &entry) { return entry.name.compare(name, Qt::CaseInsensitive) == 0; });
    };
    for (Workspace *workspace : allWorkspaces()) {
        if (!workspace->name().isEmpty() && listed(previous, workspace->name()) && !listed(config.workspaces, workspace->name())) {
            workspace->setName(QString());
        }
    }
}

void Engine::Private::applyNamedWorkspaceLayouts()
{
    for (Workspace *workspace : allWorkspaces()) {
        const auto named = std::ranges::find_if(config.workspaces, [workspace](const Config::NamedWorkspace &entry) {
            return !workspace->name().isEmpty() && entry.name.compare(workspace->name(), Qt::CaseInsensitive) == 0;
        });
        workspace->setLayoutOverride(named != config.workspaces.end() ? named->layout : std::nullopt);
    }
}

std::size_t Engine::Private::namedWorkspaceSlot(const std::vector<Workspace> &workspaces, const QString &name) const
{
    std::size_t slot = 0;
    for (const Config::NamedWorkspace &named : config.workspaces) {
        if (named.name.compare(name, Qt::CaseInsensitive) == 0) {
            break;
        }
        if (const auto found = namedWorkspaceIndex(workspaces, named.name)) {
            slot = *found + 1;
        }
    }
    return slot;
}

void Engine::Private::ensureNamedWorkspaces()
{
    for (const Config::NamedWorkspace &named : config.workspaces) {
        const bool exists = namedWorkspaceIndex(orphanWorkspaces, named.name)
            || std::ranges::any_of(
                monitors, [&named](const Monitor &monitor) { return namedWorkspaceIndex(monitor.workspaces(), named.name).has_value(); });
        if (exists) {
            continue;
        }
        if (monitors.empty()) {
            orphanWorkspaces.insert(
                orphanWorkspaces.begin() + static_cast<std::ptrdiff_t>(namedWorkspaceSlot(orphanWorkspaces, named.name)),
                Workspace(OutputArea(), clock, options, named));
            continue;
        }
        std::size_t monitorIndex = activeMonitorIndex;
        if (named.openOnOutput) {
            monitorIndex = monitorIndexByName(*named.openOnOutput).value_or(0);
        }
        monitorIndex = std::min(monitorIndex, monitors.size() - 1);
        Monitor &monitor = monitors[monitorIndex];
        monitor.insertWorkspace(
            Workspace(monitor.area(), clock, options, named), namedWorkspaceSlot(monitor.workspaces(), named.name), false);
    }
}

}
