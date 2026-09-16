#include "layout/engine/engineprivate.h"

#include "layout/common/sizechange.h"

#include <algorithm>
#include <utility>

namespace Konveyor::Layout
{

namespace
{

struct WorkspaceLocation
{
    std::size_t monitor = 0;
    std::size_t workspace = 0;
};

const QStringList &directionNames()
{
    static const QStringList names {QStringLiteral("left"), QStringLiteral("right"), QStringLiteral("down"), QStringLiteral("up"),
        QStringLiteral("previous"), QStringLiteral("next")};
    return names;
}

void addNamed(ActionTable &table, const QString &name, ActionHandler handler)
{
    table.insert(name, std::move(handler));
}

using DirectionHandler = std::function<void(Engine::Private &, std::size_t, const Config::Action &)>;

void addDirectionalActions(ActionTable &table, const QString &prefix, const DirectionHandler &handler)
{
    for (const QString &direction : directionNames()) {
        addNamed(
            table, prefix + direction, [direction, handler](Engine::Private &d, const Config::Action &action, std::optional<WindowId>) {
                if (const auto idx = d.monitorInDirection(direction)) {
                    handler(d, *idx, action);
                }
                return ActionResult();
            });
    }
}

std::optional<WorkspaceLocation> findWorkspace(Engine::Private &d, const Config::WorkspaceReference &reference)
{
    if (d.monitors.empty()) {
        return std::nullopt;
    }
    if (reference.kind == Config::WorkspaceReferenceKind::Index) {
        const std::size_t monitorIndex = std::min(d.activeMonitorIndex, d.monitors.size() - 1);
        const std::size_t count = d.monitors[monitorIndex].workspaces().size();
        const std::size_t index = reference.index > 0 ? static_cast<std::size_t>(reference.index - 1) : 0;
        return WorkspaceLocation {monitorIndex, std::min(index, count - 1)};
    }
    for (std::size_t idx = 0; idx < d.monitors.size(); ++idx) {
        const auto found = reference.kind == Config::WorkspaceReferenceKind::Name ? d.monitors[idx].workspaceNamed(reference.name)
                                                                                  : d.monitors[idx].indexOfWorkspace(reference.index);
        if (found) {
            return WorkspaceLocation {idx, *found};
        }
    }
    return std::nullopt;
}

std::expected<Config::WorkspaceReference, QString> referenceArgument(const Config::Action &action, int index)
{
    const QString text = actionArgument(action, index);
    if (text.isEmpty()) {
        return std::unexpected(QStringLiteral("a workspace reference is required"));
    }
    return parseWorkspaceReference(text);
}

std::optional<WorkspaceLocation> referenceLocation(Engine::Private &d, const Config::Action &action, const QString &property)
{
    const auto text = actionProperty(action, property);
    if (!text) {
        if (d.monitors.empty()) {
            return std::nullopt;
        }
        const std::size_t monitorIndex = std::min(d.activeMonitorIndex, d.monitors.size() - 1);
        return WorkspaceLocation {monitorIndex, d.monitors[monitorIndex].activeWorkspaceIndex()};
    }
    const auto reference = parseWorkspaceReference(*text);
    return reference ? findWorkspace(d, *reference) : std::nullopt;
}

void registerWorkspaceSwitching(ActionTable &table)
{
    addMonitorAction(table, "focus-workspace-down", +[](Monitor &m) { m.switchWorkspaceDown(); });
    addMonitorAction(table, "focus-workspace-up", +[](Monitor &m) { m.switchWorkspaceUp(); });
    addMonitorAction(table, "focus-workspace-previous", +[](Monitor &m) { m.switchWorkspacePrevious(); });
    addMonitorAction(table, "move-workspace-down", +[](Monitor &m) { m.moveWorkspaceDown(); });
    addMonitorAction(table, "move-workspace-up", +[](Monitor &m) { m.moveWorkspaceUp(); });
    addNamed(table, QStringLiteral("focus-workspace"), [](Engine::Private &d, const Config::Action &action, std::optional<WindowId>) {
        const auto reference = referenceArgument(action, 0);
        if (!reference) {
            return actionError(reference.error());
        }
        const auto location = findWorkspace(d, *reference);
        if (!location) {
            return actionError(QStringLiteral("no such workspace"));
        }
        Monitor &monitor = d.monitors[location->monitor];
        if (d.config.input.workspaceAutoBackAndForth) {
            monitor.goToWorkspaceOrBack(location->workspace);
        } else {
            monitor.switchWorkspace(location->workspace);
        }
        d.activeMonitorIndex = location->monitor;
        return ActionResult();
    });
}

void registerWorkspaceMoves(ActionTable &table)
{
    addNamed(table, QStringLiteral("move-window-to-workspace-down"),
        [](Engine::Private &d, const Config::Action &action, std::optional<WindowId>) {
            const bool focus = actionFlag(action, QStringLiteral("focus"), true);
            if (Monitor *monitor = d.activeMonitor()) {
                monitor->moveToWorkspaceDown(focus ? Activation::Smart : Activation::Never);
            }
            return ActionResult();
        });
    addNamed(table, QStringLiteral("move-window-to-workspace-up"),
        [](Engine::Private &d, const Config::Action &action, std::optional<WindowId>) {
            const bool focus = actionFlag(action, QStringLiteral("focus"), true);
            if (Monitor *monitor = d.activeMonitor()) {
                monitor->moveToWorkspaceUp(focus ? Activation::Smart : Activation::Never);
            }
            return ActionResult();
        });
    addNamed(table, QStringLiteral("move-column-to-workspace-down"),
        [](Engine::Private &d, const Config::Action &action, std::optional<WindowId>) {
            if (Monitor *monitor = d.activeMonitor()) {
                monitor->moveColumnToWorkspaceDown(actionFlag(action, QStringLiteral("focus"), true));
            }
            return ActionResult();
        });
    addNamed(table, QStringLiteral("move-column-to-workspace-up"),
        [](Engine::Private &d, const Config::Action &action, std::optional<WindowId>) {
            if (Monitor *monitor = d.activeMonitor()) {
                monitor->moveColumnToWorkspaceUp(actionFlag(action, QStringLiteral("focus"), true));
            }
            return ActionResult();
        });
}

struct WorkspaceTargetRequest
{
    std::optional<WorkspaceLocation> location;
    bool focus = true;
    QString error;
};

WorkspaceTargetRequest resolveWorkspaceTarget(Engine::Private &d, const Config::Action &action)
{
    const auto reference = referenceArgument(action, 0);
    if (!reference) {
        return {std::nullopt, true, reference.error()};
    }
    const auto location = findWorkspace(d, *reference);
    if (!location) {
        return {std::nullopt, true, QStringLiteral("no such workspace")};
    }
    return {location, actionFlag(action, QStringLiteral("focus"), true), QString()};
}

ActionResult moveWindowToWorkspace(Engine::Private &d, const Config::Action &action, std::optional<WindowId> target)
{
    const WorkspaceTargetRequest request = resolveWorkspaceTarget(d, action);
    if (!request.location) {
        return actionError(request.error);
    }
    const auto location = request.location;
    const bool focus = request.focus;
    const auto window = actionWindowId(action, target);
    const auto sourceIndex = window ? d.monitorIndexOf(*window) : std::optional<std::size_t>(d.activeMonitorIndex);
    if (!sourceIndex) {
        return actionError(QStringLiteral("no such window"));
    }
    if (*sourceIndex != location->monitor) {
        d.moveWindowToMonitor(window, location->monitor, focus);
        return {};
    }
    d.monitors[location->monitor].moveToWorkspace(window, location->workspace, focus ? Activation::Smart : Activation::Never);
    return {};
}

ActionResult moveColumnToWorkspace(Engine::Private &d, const Config::Action &action)
{
    const WorkspaceTargetRequest request = resolveWorkspaceTarget(d, action);
    if (!request.location) {
        return actionError(request.error);
    }
    const auto location = request.location;
    const bool focus = request.focus;
    if (location->monitor != d.activeMonitorIndex) {
        d.moveColumnToMonitor(location->monitor, focus);
        return {};
    }
    d.monitors[location->monitor].moveColumnToWorkspace(location->workspace, focus);
    return {};
}

ActionResult moveWorkspaceToIndex(Engine::Private &d, const Config::Action &action)
{
    const auto index = parseIndex(actionArgument(action, 0));
    if (!index) {
        return actionError(index.error());
    }
    const auto location = referenceLocation(d, action, QStringLiteral("reference"));
    if (!location) {
        return actionError(QStringLiteral("no such workspace"));
    }
    const std::size_t target = *index > 0 ? static_cast<std::size_t>(*index - 1) : 0;
    d.monitors[location->monitor].moveWorkspaceToIndex(location->workspace, target);
    return {};
}

ActionResult setWorkspaceName(Engine::Private &d, const Config::Action &action)
{
    const QString name = actionArgument(action, 0);
    if (name.isEmpty()) {
        return actionError(QStringLiteral("set-workspace-name requires a name"));
    }
    const auto location = referenceLocation(d, action, QStringLiteral("workspace"));
    if (!location) {
        return actionError(QStringLiteral("no such workspace"));
    }
    d.monitors[location->monitor].workspaces()[location->workspace].setName(name);
    return {};
}

ActionResult unsetWorkspaceName(Engine::Private &d, const Config::Action &action)
{
    std::optional<WorkspaceLocation> location;
    const QString text = actionArgument(action, 0);
    if (text.isEmpty()) {
        location = referenceLocation(d, action, QStringLiteral("__none"));
    } else if (const auto reference = parseWorkspaceReference(text)) {
        location = findWorkspace(d, *reference);
    }
    if (!location) {
        return actionError(QStringLiteral("no such workspace"));
    }
    Monitor &monitor = d.monitors[location->monitor];
    monitor.clearWorkspaceName(monitor.workspaces()[location->workspace].id());
    return {};
}

}

void registerWorkspaceActions(ActionTable &table)
{
    registerWorkspaceSwitching(table);
    registerWorkspaceMoves(table);
    addNamed(table, QStringLiteral("move-window-to-workspace"), moveWindowToWorkspace);
    addNamed(table, QStringLiteral("move-column-to-workspace"),
        [](Engine::Private &d, const Config::Action &action, std::optional<WindowId>) { return moveColumnToWorkspace(d, action); });
    addNamed(table, QStringLiteral("move-workspace-to-index"),
        [](Engine::Private &d, const Config::Action &action, std::optional<WindowId>) { return moveWorkspaceToIndex(d, action); });
    addNamed(table, QStringLiteral("set-workspace-name"),
        [](Engine::Private &d, const Config::Action &action, std::optional<WindowId>) { return setWorkspaceName(d, action); });
    addNamed(table, QStringLiteral("unset-workspace-name"),
        [](Engine::Private &d, const Config::Action &action, std::optional<WindowId>) { return unsetWorkspaceName(d, action); });
}

namespace
{

ActionResult withNamedMonitor(Engine::Private &d, const Config::Action &action, const DirectionHandler &handler)
{
    const QString name = actionArgument(action, 0);
    const auto idx = d.monitorIndexByName(name);
    if (!idx) {
        return actionError(QStringLiteral("no output named %1").arg(name));
    }
    handler(d, *idx, action);
    return {};
}

void focusMonitorAt(Engine::Private &d, std::size_t idx, const Config::Action &)
{
    d.activeMonitorIndex = idx;
}

void moveWindowToMonitorAt(Engine::Private &d, std::size_t idx, const Config::Action &action)
{
    d.moveWindowToMonitor(actionWindowId(action, d.focused), idx, true);
}

void moveColumnToMonitorAt(Engine::Private &d, std::size_t idx, const Config::Action &)
{
    d.moveColumnToMonitor(idx, true);
}

void moveWorkspaceToMonitorAt(Engine::Private &d, std::size_t idx, const Config::Action &)
{
    d.moveWorkspaceToMonitor(idx);
}

void addNamedMonitorAction(ActionTable &table, const QString &name, const DirectionHandler &handler)
{
    addNamed(table, name, [handler](Engine::Private &d, const Config::Action &action, std::optional<WindowId>) {
        return withNamedMonitor(d, action, handler);
    });
}

}

void registerMonitorActions(ActionTable &table)
{
    addDirectionalActions(table, QStringLiteral("focus-monitor-"), focusMonitorAt);
    addDirectionalActions(table, QStringLiteral("move-window-to-monitor-"), moveWindowToMonitorAt);
    addDirectionalActions(table, QStringLiteral("move-column-to-monitor-"), moveColumnToMonitorAt);
    addDirectionalActions(table, QStringLiteral("move-workspace-to-monitor-"), moveWorkspaceToMonitorAt);
    addNamedMonitorAction(table, QStringLiteral("focus-monitor"), focusMonitorAt);
    addNamedMonitorAction(table, QStringLiteral("move-window-to-monitor"), moveWindowToMonitorAt);
    addNamedMonitorAction(table, QStringLiteral("move-column-to-monitor"), moveColumnToMonitorAt);
    addNamedMonitorAction(table, QStringLiteral("move-workspace-to-monitor"), moveWorkspaceToMonitorAt);
}

}
