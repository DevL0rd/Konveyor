#include "layout/engine/engineprivate.h"

#include "config/log.h"
#include "layout/common/geometry.h"

#include <algorithm>
#include <utility>

namespace Konveyor::Layout
{

namespace
{

bool wantsExpanded(bool windowWants, const std::optional<bool> &rule)
{
    return (windowWants && !rule.has_value()) || rule == true;
}

Activation activationFor(const std::optional<bool> &openFocused, ActivationPolicy policy)
{
    if (openFocused) {
        return *openFocused ? Activation::Always : Activation::Never;
    }
    switch (policy) {
    case ActivationPolicy::Focus:
        return Activation::Always;
    case ActivationPolicy::NoFocus:
        return Activation::Never;
    case ActivationPolicy::Smart:
        break;
    }
    return Activation::Smart;
}

MatchContext contextFor(const WindowProperties &properties, bool isFloating, const QString &monitorProfile = {}, const QString &output = {})
{
    return {properties.appId, properties.title, monitorProfile, false, false, true, isFloating, properties.isUrgent, output};
}

}

bool Engine::wantsWindow(const WindowProperties &properties) const
{
    return resolveWindowRules(d->config.windowRules, contextFor(properties, false), d->atStartup()).manage.value_or(true);
}

std::optional<std::size_t> Engine::Private::monitorForNewWindow(
    const NewWindowPlan &plan, const WindowProperties &properties, const QString &preferredOutput) const
{
    if (plan.rules.openOnWorkspace) {
        for (std::size_t idx = 0; idx < monitors.size(); ++idx) {
            if (monitors[idx].workspaceNamed(*plan.rules.openOnWorkspace)) {
                return idx;
            }
        }
    }
    if (plan.rules.openOnOutput) {
        if (const auto idx = monitorIndexByName(*plan.rules.openOnOutput)) {
            return idx;
        }
    }
    if (!preferredOutput.isEmpty()) {
        if (const auto idx = monitorIndexByName(preferredOutput)) {
            return idx;
        }
    }
    return properties.parent ? monitorIndexOf(*properties.parent) : std::nullopt;
}

Workspace *Engine::Private::workspaceForNewWindow(NewWindowPlan &plan)
{
    if (plan.monitorIndex >= monitors.size()) {
        return activeWorkspace();
    }
    Monitor &monitor = monitors[plan.monitorIndex];
    std::size_t workspaceIndex = monitor.activeWorkspaceIndex();
    if (plan.rules.openOnWorkspace) {
        workspaceIndex = monitor.workspaceNamed(*plan.rules.openOnWorkspace).value_or(workspaceIndex);
        plan.workspace = monitor.workspaces()[workspaceIndex].id();
    } else if (plan.rules.openOnWorkspaceIndex) {
        workspaceIndex = std::min(static_cast<std::size_t>(*plan.rules.openOnWorkspaceIndex - 1), monitor.workspaces().size() - 1);
        plan.workspace = monitor.workspaces()[workspaceIndex].id();
    }
    return &monitor.workspaces()[workspaceIndex];
}

NewWindowPlan Engine::Private::planNewWindow(
    const WindowProperties &properties, const QString &preferredOutput, ActivationPolicy policy) const
{
    auto &self = const_cast<Private &>(*this);
    NewWindowPlan plan;
    plan.rules = resolveWindowRules(config.windowRules, contextFor(properties, false), atStartup());
    plan.monitorIndex = monitorForNewWindow(plan, properties, preferredOutput).value_or(activeMonitorIndex);
    NewWindowPlan probe = plan;
    const Workspace *target = self.workspaceForNewWindow(probe);
    const Config::Layout &layout = target ? target->options()->layout : options->layout;

    const bool childWindow = plan.rules.floatChildWindows.value_or(layout.floatChildWindows) && appHasWindow(properties.appId);
    const bool autoFloat = properties.parent.has_value() || properties.isDialog || childWindow;
    plan.isFloating = plan.rules.openFloating.value_or(autoFloat);
    plan.rules = resolveWindowRules(config.windowRules, contextFor(properties, plan.isFloating), atStartup());

    const auto monitorIndex = monitorForNewWindow(plan, properties, preferredOutput);
    if (properties.parent && self.workspaceOf(*properties.parent)
        && (!monitorIndex || monitorIndex == monitorIndexOf(*properties.parent))) {
        plan.parent = properties.parent;
    }
    plan.monitorIndex = monitorIndex.value_or(activeMonitorIndex);
    if (plan.monitorIndex < monitors.size()) {
        const OutputArea &area = monitors[plan.monitorIndex].area();
        plan.rules = resolveWindowRules(
            config.windowRules, contextFor(properties, plan.isFloating, monitorProfileName(config, area), area.outputName), atStartup());
    }

    plan.wantsFullscreen = wantsExpanded(properties.wantsFullscreen, plan.rules.openFullscreen);
    plan.wantsMaximized = plan.rules.openMaximizedToEdges.value_or(false);
    plan.fillsWidth = plan.rules.openMaximized.value_or(properties.wantsMaximized);
    plan.activate = activationFor(plan.rules.openFocused, policy);
    if (Workspace *workspace = self.workspaceForNewWindow(plan)) {
        plan.width = workspace->defaultWidthFor(plan.rules.defaultWidth, plan.isFloating);
        plan.height = workspace->defaultHeightFor(plan.rules.defaultHeight, plan.isFloating);
        applyRememberedSize(plan, properties.appId, workspace->options()->layout);
    }
    return plan;
}

namespace
{

WindowMode initialSizing(const NewWindowPlan &plan, const Workspace &workspace, QSize &size)
{
    if (plan.wantsFullscreen) {
        size = roundedSize(workspace.area().viewSize);
        return WindowMode::Fullscreen;
    }
    if (plan.wantsMaximized) {
        size = roundedSize(workspace.area().workingArea.size());
        return WindowMode::Maximized;
    }
    return WindowMode::Normal;
}

LayoutWindow makeNewWindow(WindowId id, const WindowProperties &properties, const NewWindowPlan &plan, const Workspace &workspace)
{
    LayoutWindow window(id, properties, plan.nativeSize);
    window.setRules(plan.rules);

    std::optional<Config::PresetSize> configureWidth = plan.width;
    if (plan.fillsWidth && !plan.isFloating) {
        configureWidth = Config::PresetSize(Config::Proportion {1.0});
    }
    QSize size = workspace.initialWindowSize(configureWidth, plan.height, plan.isFloating, plan.rules, window.minSize(), window.maxSize());
    const WindowMode mode = initialSizing(plan, workspace, size);
    window.requestSize(size, mode, false);
    window.commit(QSize(size.width() > 0 ? size.width() : roundToInt(properties.frameSize.width()),
        size.height() > 0 ? size.height() : roundToInt(properties.frameSize.height())));
    return window;
}

MonitorAddRequest makeAddRequest(const NewWindowPlan &plan, ColumnWidth width)
{
    MonitorAddRequest request;
    request.activate = plan.activate;
    request.width = width;
    request.fillsWidth = plan.fillsWidth;
    request.isFloating = plan.isFloating;
    if (plan.parent) {
        request.target = MonitorAddTarget::besideWindow(*plan.parent);
    } else if (plan.workspace) {
        request.target = MonitorAddTarget::onWorkspace(*plan.workspace);
    }
    return request;
}

}

Workspace *Engine::Private::workspaceForPlacement(const NewWindowPlan &plan)
{
    if (Workspace *workspace = plan.workspace ? workspaceById(*plan.workspace) : nullptr) {
        return workspace;
    }
    if (plan.monitorIndex < monitors.size()) {
        return &monitors[plan.monitorIndex].activeWorkspace();
    }
    if (Workspace *workspace = plan.parent ? workspaceOf(*plan.parent) : nullptr) {
        return workspace;
    }
    return activeWorkspace();
}

void Engine::Private::finishPlacement(WindowId id, const NewWindowPlan &plan)
{
    Workspace *placed = workspaceOf(id);
    if (!placed) {
        return;
    }
    if (!plan.isFloating && plan.height) {
        placed->setWindowHeight(id, sizeChangeFromPreset(*plan.height));
    }
    if (plan.wantsFullscreen && plan.wantsMaximized) {
        placed->setMaximized(id, true);
    }
    placed->animateOpening(id);
    if (resolveActivation(plan.activate, false) && plan.monitorIndex < monitors.size()) {
        activeMonitorIndex = plan.monitorIndex;
    }
}

void Engine::Private::placeNewWindow(
    WindowId id, const WindowProperties &properties, const NewWindowPlan &plan, const std::optional<RestorePlacement> &restore)
{
    Workspace *workspace = workspaceForPlacement(plan);
    if (!workspace) {
        return;
    }

    Tile tile = workspace->createTile(makeNewWindow(id, properties, plan, *workspace));
    const auto remembered = windowMemory.constFind(properties.appId);
    if (plan.isFloating && workspace->options()->layout.rememberWindowPositions && remembered != windowMemory.constEnd()
        && remembered->floatingPosition && !plan.rules.defaultFloatingPosition) {
        tile.savedFloatingPosition = remembered->floatingPosition;
    }
    const ColumnWidth width = workspace->tiledWidthFor(tile.window(), plan.width);
    MonitorAddRequest request = makeAddRequest(plan, width);
    if (!restore && plan.rules.openAtColumn && !plan.isFloating && !plan.parent) {
        const std::size_t columns = workspace->scrolling().columns().size();
        request.target
            = MonitorAddTarget::onWorkspace(workspace->id(), std::min(static_cast<std::size_t>(*plan.rules.openAtColumn - 1), columns));
    } else if ((restore && placeRestored(tile, plan, *restore, request))
        || (!restore && placeInAppGroup(tile, plan, *workspace, request))) {
        finishPlacement(id, plan);
        return;
    }

    if (plan.monitorIndex < monitors.size()) {
        monitors[plan.monitorIndex].addTile(std::move(tile), request);
    } else {
        const AddTarget target
            = request.target.kind == MonitorAddTarget::Kind::NextTo ? AddTarget::besideWindow(request.target.window) : AddTarget();
        workspace->addTile(std::move(tile), {target, request.activate, width, plan.fillsWidth, plan.isFloating, std::nullopt});
    }
    finishPlacement(id, plan);
}

bool Engine::hasWindow(WindowId id) const
{
    return d->workspaceOf(id) != nullptr;
}

void Engine::updateWindowProperties(WindowId id, const WindowProperties &properties)
{
    Tile *tile = d->tileOf(id);
    if (!tile) {
        return;
    }
    LayoutWindow &window = tile->window();
    const bool parentChanged = window.properties().parent != properties.parent;
    const bool wasLocked = d->isLockedToOutputSize(id);
    window.setProperties(properties);
    window.setUrgent(properties.isUrgent);
    window.markRulesDirty();
    if (Workspace *workspace = d->workspaceOf(id)) {
        if (parentChanged && properties.parent) {
            workspace->childrenAdded(*properties.parent);
        }
        workspace->updateWindow(id);
    } else {
        tile->updateWindow();
    }
    if (!wasLocked && window.requestedMode() != WindowMode::Fullscreen && d->isLockedToOutputSize(id)) {
        d->floatOverOutput(id);
    }
    d->refresh();
}

void Engine::windowSizeCommitted(WindowId id, const QSizeF &frameSize)
{
    Tile *tile = d->tileOf(id);
    if (!tile) {
        return;
    }
    if (tile->window().commit(frameSize)) {
        if (Workspace *workspace = d->workspaceOf(id)) {
            workspace->updateWindow(id);
        } else {
            tile->updateWindow();
        }
    }
    d->refresh();
}

void Engine::setFloatingFrame(WindowId id, const QRectF &frame)
{
    d->setFloatingFrame(id, frame);
    d->refresh();
}

void Engine::Private::setFloatingFrame(WindowId id, const QRectF &frame)
{
    Monitor *monitor = monitorOf(id);
    const std::optional<std::size_t> index = monitor ? monitor->workspaceOfWindow(id) : std::nullopt;
    if (!index) {
        return;
    }
    Workspace &workspace = monitor->workspaces()[*index];
    const Tile *tile = workspace.tileFor(id);
    if (!tile || !workspace.isFloating(id)) {
        return;
    }
    const QPointF origin = originOf(monitor->outputName()) + QPointF(0.0, monitor->workspaceRenderOffsets()[*index]);
    workspace.setFloatingFrame(id, frame.topLeft() - origin - tile->windowOffset(), frame.size());
}

void Engine::activateWindow(WindowId id)
{
    qCInfo(lcKonveyor) << "konveyor: layout activating window" << id;
    if (d->focusWindow(id)) {
        d->announcedFocus = id;
        d->refresh();
    }
}

bool Engine::Private::focusWindow(WindowId id)
{
    layoutFocused = true;
    for (std::size_t monitorIndex = 0; monitorIndex < monitors.size(); ++monitorIndex) {
        Monitor &monitor = monitors[monitorIndex];
        const auto workspaceIndex = monitor.workspaceOfWindow(id);
        if (!workspaceIndex) {
            continue;
        }
        monitor.workspaces()[*workspaceIndex].activateWindow(id);
        monitor.activateWorkspace(*workspaceIndex);
        activeMonitorIndex = monitorIndex;
        return true;
    }
    return std::ranges::any_of(orphanWorkspaces, [id](Workspace &workspace) { return workspace.activateWindow(id); });
}

void Engine::setLayoutFocused(bool focused)
{
    if (d->layoutFocused != focused) {
        d->layoutFocused = focused;
        d->refresh();
    }
}

void Engine::setWindowFullscreen(WindowId id, bool fullscreen)
{
    qCInfo(lcKonveyor) << "konveyor: layout fullscreen for window" << id << "=" << fullscreen
                       << "known =" << (d->workspaceOf(id) != nullptr);
    if (fullscreen) {
        d->dropDraggedWindow(id);
    }
    if (Workspace *workspace = d->workspaceOf(id)) {
        workspace->setFullscreen(id, fullscreen);
        if (!fullscreen && d->isLockedToOutputSize(id)) {
            d->floatOverOutput(id);
        }
        d->refresh();
    }
}

void Engine::toggleWindowFillWidth(WindowId id)
{
    qCInfo(lcKonveyor) << "konveyor: toggling fill width for window" << id;
    d->focusWindow(id);
    if (Workspace *workspace = d->workspaceOf(id)) {
        workspace->toggleFillWidth(id);
        d->refresh();
    }
}

void Engine::setWindowUrgent(WindowId id, bool urgent)
{
    if (Tile *tile = d->tileOf(id)) {
        tile->window().setUrgent(urgent);
        d->refresh();
    }
}

}
