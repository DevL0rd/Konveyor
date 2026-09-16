#include "layout/engine/engineprivate.h"

#include "layout/common/geometry.h"
#include "layout/monitor/monitorprofiles.h"

#include <algorithm>
#include <chrono>
#include <utility>

namespace Konveyor::Layout
{

namespace
{

constexpr auto StartupWindow = std::chrono::seconds(60);

std::optional<Config::Layout> layoutForOutput(const Config::Config &config, const QString &name, QSizeF size)
{
    for (const Config::OutputConfig &output : config.outputs) {
        if (output.name.compare(name, Qt::CaseInsensitive) == 0 && output.layout) {
            return output.layout;
        }
    }
    const Config::MonitorProfile *profile = monitorProfileFor(config, name, size);
    return profile ? profile->layout : std::nullopt;
}

}

Engine::Engine(Anim::Clock &clock, Hooks hooks)
    : d(std::make_unique<Private>(clock, std::move(hooks)))
{ }

Engine::~Engine() = default;

void Engine::setConfig(const Config::Config &config)
{
    d->config = config;
    d->applyOptions();
    d->ensureNamedWorkspaces();
    for (Workspace *workspace : d->allWorkspaces()) {
        for (const TileRef &ref : workspace->renderedTilesMut(false)) {
            ref.tile->window().markRulesDirty();
        }
    }
    d->refresh();
}

Engine::Private::Private(Anim::Clock &clockRef, Hooks engineHooks)
    : clock(clockRef)
    , hooks(std::move(engineHooks))
    , options(makeOptions(optionsFromConfig(Config::Config())))
    , startupTime(clockRef.rawNow())
{ }

bool Engine::Private::atStartup() const
{
    return Anim::saturatingSub(clock.rawNow(), startupTime) < Anim::Duration(StartupWindow);
}

void Engine::addOutput(const OutputInfo &output)
{
    if (d->monitorIndexByName(output.name)) {
        updateOutput(output);
        return;
    }
    d->outputInfos.insert(output.name, output);
    const OutputArea area = d->areaFor(output);

    std::vector<Workspace> taken;
    if (d->monitors.empty()) {
        taken = std::move(d->orphanWorkspaces);
        d->orphanWorkspaces.clear();
    } else {
        taken = d->monitors[0].takeWorkspacesForOutput(area);
    }

    std::optional<WorkspaceId> toActivate;
    if (const auto it = d->lastActiveWorkspace.constFind(output.name); it != d->lastActiveWorkspace.constEnd()) {
        toActivate = *it;
        d->lastActiveWorkspace.erase(d->lastActiveWorkspace.find(output.name));
    }

    d->monitors.emplace_back(
        area, std::move(taken), toActivate, d->clock, d->options, layoutForOutput(d->config, output.name, output.geometry.size()));
    d->monitors.back().overviewOpen = d->overviewOpen;
    d->refresh();
}

void Engine::updateOutput(const OutputInfo &output)
{
    if (!d->monitorIndexByName(output.name)) {
        addOutput(output);
        return;
    }
    d->outputInfos.insert(output.name, output);
    Monitor *monitor = d->monitorByName(output.name);
    monitor->setArea(d->areaFor(output));
    monitor->setLayoutOverride(layoutForOutput(d->config, output.name, output.geometry.size()));
    monitor->updateConfig(d->options);
    d->refresh();
}

void Engine::removeOutput(const QString &name)
{
    const auto idx = d->monitorIndexByName(name);
    if (!idx) {
        return;
    }
    d->lastActiveWorkspace.insert(name, d->monitors[*idx].activeWorkspace().id());
    std::vector<Workspace> workspaces = d->monitors[*idx].releaseWorkspaces();
    d->monitors.erase(d->monitors.begin() + static_cast<std::ptrdiff_t>(*idx));
    d->outputInfos.remove(name);
    if (d->activeMonitorIndex >= *idx && d->activeMonitorIndex > 0) {
        d->activeMonitorIndex -= 1;
    }

    if (d->monitors.empty()) {
        for (Workspace &workspace : workspaces) {
            workspace.updateConfig(d->options);
        }
        d->orphanWorkspaces = std::move(workspaces);
    } else {
        d->monitors[0].appendWorkspaces(std::move(workspaces));
    }
    d->refresh();
}

void Engine::focusOutput(const QString &name)
{
    if (const auto idx = d->monitorIndexByName(name)) {
        d->activeMonitorIndex = *idx;
        d->refresh();
    }
}

std::optional<QString> Engine::focusedOutput() const
{
    if (d->monitors.empty()) {
        return std::nullopt;
    }
    return d->monitors[std::min(d->activeMonitorIndex, d->monitors.size() - 1)].outputName();
}

void Engine::focusWorkspace(const QString &output, int index)
{
    Monitor *monitor = d->monitorByName(output);
    if (!monitor || index < 1) {
        return;
    }
    monitor->switchWorkspace(static_cast<std::size_t>(index - 1));
    if (const auto idx = d->monitorIndexByName(output)) {
        d->activeMonitorIndex = *idx;
    }
    d->refresh();
}

bool Engine::isOverviewOpen() const
{
    return d->overviewOpen;
}

void Engine::setOverviewOpen(bool open)
{
    if (d->overviewOpen == open) {
        return;
    }
    d->overviewOpen = open;
    for (Monitor &monitor : d->monitors) {
        monitor.overviewOpen = open;
    }
    if (d->hooks.setOverviewOpen) {
        d->hooks.setOverviewOpen(open);
    }
}

void Engine::tickAnimations()
{
    for (Monitor &monitor : d->monitors) {
        monitor.tickAnimations();
    }
    for (Workspace &workspace : d->orphanWorkspaces) {
        workspace.tickAnimations();
    }
    d->refresh();
}

bool Engine::isAnimating() const
{
    return std::ranges::any_of(d->monitors, &Monitor::isAnimating);
}

QString monitorProfileName(const Config::Config &config, const OutputArea &area)
{
    const Config::MonitorProfile *profile = monitorProfileFor(config, area.outputName, area.viewSize);
    return profile ? profile->name : QString();
}

void Engine::Private::applyOptions()
{
    options = makeOptions(optionsFromConfig(config));
    clock.applyConfig(config.animations);
    for (Monitor &monitor : monitors) {
        monitor.setLayoutOverride(layoutForOutput(config, monitor.outputName(), monitor.area().viewSize));
        monitor.updateConfig(options);
    }
    for (Workspace &workspace : orphanWorkspaces) {
        workspace.updateConfig(options);
    }
}

}
