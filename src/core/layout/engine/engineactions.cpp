#include "layout/engine/engineprivate.h"

#include "layout/common/sizechange.h"

#include <algorithm>
#include <utility>

namespace Konveyor::Layout
{

QString actionArgument(const Config::Action &action, int index)
{
    return index < action.arguments.size() ? action.arguments.at(index) : QString();
}

std::optional<QString> actionProperty(const Config::Action &action, const QString &name)
{
    for (const auto &[key, value] : action.properties) {
        if (key == name) {
            return value;
        }
    }
    return std::nullopt;
}

ActionResult actionError(const QString &message)
{
    return {false, message};
}

std::optional<WindowId> actionWindowId(const Config::Action &action, std::optional<WindowId> fallback)
{
    auto text = actionProperty(action, QStringLiteral("id"));
    if (!text) {
        text = actionProperty(action, QStringLiteral("window-id"));
    }
    if (!text) {
        return fallback;
    }
    const auto parsed = parseIndex(*text);
    return parsed ? std::optional<WindowId>(*parsed) : fallback;
}

bool actionFlag(const Config::Action &action, const QString &name, bool fallback)
{
    const auto text = actionProperty(action, name);
    if (!text) {
        return fallback;
    }
    const auto parsed = parseBool(*text);
    return parsed ? *parsed : fallback;
}

void addEngineAction(ActionTable &table, const char *name, ActionHandler handler)
{
    table.insert(QString::fromLatin1(name), std::move(handler));
}

void addWorkspaceAction(ActionTable &table, const char *name, void (*fn)(Workspace &))
{
    addEngineAction(table, name, [fn](Engine::Private &d, const Config::Action &, std::optional<WindowId>) {
        if (Workspace *workspace = d.activeWorkspace()) {
            fn(*workspace);
        }
        return ActionResult();
    });
}

void addTargetAction(ActionTable &table, const char *name, void (*fn)(Workspace &, std::optional<WindowId>))
{
    addEngineAction(table, name, [fn](Engine::Private &d, const Config::Action &action, std::optional<WindowId> target) {
        const auto window = d.target(actionWindowId(action, target));
        if (Workspace *workspace = d.workspaceForTarget(window)) {
            fn(*workspace, window);
        }
        return ActionResult();
    });
}

void addMonitorAction(ActionTable &table, const char *name, void (*fn)(Monitor &))
{
    addEngineAction(table, name, [fn](Engine::Private &d, const Config::Action &, std::optional<WindowId>) {
        if (Monitor *monitor = d.activeMonitor()) {
            fn(*monitor);
        }
        return ActionResult();
    });
}

void addForwardedAction(ActionTable &table, const char *name)
{
    const QString actionName = QString::fromLatin1(name);
    addEngineAction(table, name, [actionName](Engine::Private &d, const Config::Action &action, std::optional<WindowId>) {
        if (actionName == QLatin1String("spawn") || actionName == QLatin1String("spawn-sh")) {
            if (d.hooks.spawn) {
                d.hooks.spawn(action.arguments.join(QLatin1Char(' ')));
            }
            return ActionResult();
        }
        if (d.hooks.compositorAction) {
            d.hooks.compositorAction(actionName);
        }
        return ActionResult();
    });
}

namespace
{

void registerColumnFocusActions(ActionTable &table)
{
    addWorkspaceAction(table, "focus-column-left", +[](Workspace &ws) { ws.focusLeft(); });
    addWorkspaceAction(table, "focus-column-right", +[](Workspace &ws) { ws.focusRight(); });
    addWorkspaceAction(table, "focus-column-first", +[](Workspace &ws) { ws.focusColumnFirst(); });
    addWorkspaceAction(table, "focus-column-last", +[](Workspace &ws) { ws.focusColumnLast(); });
    addWorkspaceAction(table, "focus-column-right-or-first", +[](Workspace &ws) { ws.focusColumnRightOrFirst(); });
    addWorkspaceAction(table, "focus-column-left-or-last", +[](Workspace &ws) { ws.focusColumnLeftOrLast(); });
    addWorkspaceAction(table, "focus-window-down", +[](Workspace &ws) { ws.focusDown(); });
    addWorkspaceAction(table, "focus-window-up", +[](Workspace &ws) { ws.focusUp(); });
    addWorkspaceAction(table, "focus-window-down-or-column-left", +[](Workspace &ws) { ws.focusDownOrLeft(); });
    addWorkspaceAction(table, "focus-window-down-or-column-right", +[](Workspace &ws) { ws.focusDownOrRight(); });
    addWorkspaceAction(table, "focus-window-up-or-column-left", +[](Workspace &ws) { ws.focusUpOrLeft(); });
    addWorkspaceAction(table, "focus-window-up-or-column-right", +[](Workspace &ws) { ws.focusUpOrRight(); });
    addWorkspaceAction(table, "focus-window-top", +[](Workspace &ws) { ws.focusWindowTop(); });
    addWorkspaceAction(table, "focus-window-bottom", +[](Workspace &ws) { ws.focusWindowBottom(); });
    addWorkspaceAction(table, "focus-window-down-or-top", +[](Workspace &ws) { ws.focusWindowDownOrTop(); });
    addWorkspaceAction(table, "focus-window-up-or-bottom", +[](Workspace &ws) { ws.focusWindowUpOrBottom(); });
    addWorkspaceAction(table, "focus-floating", +[](Workspace &ws) { ws.focusFloating(); });
    addWorkspaceAction(table, "focus-tiling", +[](Workspace &ws) { ws.focusTiling(); });
    addWorkspaceAction(table, "switch-focus-between-floating-and-tiling", +[](Workspace &ws) { ws.switchFocusFloatingTiling(); });
    addMonitorAction(table, "focus-window-or-workspace-down", +[](Monitor &m) { m.focusWindowOrWorkspaceDown(); });
    addMonitorAction(table, "focus-window-or-workspace-up", +[](Monitor &m) { m.focusWindowOrWorkspaceUp(); });
}

ActionResult focusByIndex(Engine::Private &d, const Config::Action &action, bool inColumn)
{
    const auto index = parseIndex(actionArgument(action, 0));
    if (!index) {
        return actionError(index.error());
    }
    Workspace *workspace = d.activeWorkspace();
    if (!workspace) {
        return {};
    }
    if (inColumn) {
        workspace->focusWindowInColumn(static_cast<std::size_t>(*index));
    } else {
        workspace->focusColumn(static_cast<std::size_t>(*index));
    }
    return {};
}

ActionResult focusWindowPrevious(Engine::Private &d)
{
    std::optional<WindowId> best;
    quint64 bestOrder = 0;
    for (auto it = d.focusOrder.constBegin(); it != d.focusOrder.constEnd(); ++it) {
        if (d.focused && it.key() == *d.focused) {
            continue;
        }
        if (!d.workspaceOf(it.key())) {
            continue;
        }
        if (!best || it.value() > bestOrder) {
            best = it.key();
            bestOrder = it.value();
        }
    }
    if (!best) {
        return {};
    }
    for (std::size_t idx = 0; idx < d.monitors.size(); ++idx) {
        if (const auto workspaceIndex = d.monitors[idx].workspaceOfWindow(*best)) {
            d.monitors[idx].workspaces()[*workspaceIndex].activateWindow(*best);
            d.monitors[idx].activateWorkspace(*workspaceIndex);
            d.activeMonitorIndex = idx;
            break;
        }
    }
    return {};
}

ActionResult focusDirectionalOrMonitor(Engine::Private &d, const QString &direction, bool vertical)
{
    Workspace *workspace = d.activeWorkspace();
    if (workspace) {
        const bool handled = vertical ? (direction == QLatin1String("up") ? workspace->focusUp() : workspace->focusDown())
                                      : (direction == QLatin1String("left") ? workspace->focusLeft() : workspace->focusRight());
        if (handled) {
            return {};
        }
    }
    if (const auto idx = d.monitorInDirection(direction)) {
        d.activeMonitorIndex = *idx;
    }
    return {};
}

}

void registerFocusActions(ActionTable &table)
{
    registerColumnFocusActions(table);
    addEngineAction(table, "focus-column",
        [](Engine::Private &d, const Config::Action &action, std::optional<WindowId>) { return focusByIndex(d, action, false); });
    addEngineAction(table, "focus-window-in-column",
        [](Engine::Private &d, const Config::Action &action, std::optional<WindowId>) { return focusByIndex(d, action, true); });
    addEngineAction(table, "focus-window-previous",
        [](Engine::Private &d, const Config::Action &, std::optional<WindowId>) { return focusWindowPrevious(d); });
    addEngineAction(table, "focus-window", [](Engine::Private &d, const Config::Action &action, std::optional<WindowId> target) {
        const auto window = actionWindowId(action, target);
        if (!window) {
            return actionError(QStringLiteral("focus-window requires a window id"));
        }
        for (std::size_t idx = 0; idx < d.monitors.size(); ++idx) {
            if (const auto workspaceIndex = d.monitors[idx].workspaceOfWindow(*window)) {
                d.monitors[idx].workspaces()[*workspaceIndex].activateWindow(*window);
                d.monitors[idx].activateWorkspace(*workspaceIndex);
                d.activeMonitorIndex = idx;
                return ActionResult();
            }
        }
        return actionError(QStringLiteral("no window with id %1").arg(*window));
    });
    addEngineAction(table, "focus-window-or-monitor-up", [](Engine::Private &d, const Config::Action &, std::optional<WindowId>) {
        return focusDirectionalOrMonitor(d, QStringLiteral("up"), true);
    });
    addEngineAction(table, "focus-window-or-monitor-down", [](Engine::Private &d, const Config::Action &, std::optional<WindowId>) {
        return focusDirectionalOrMonitor(d, QStringLiteral("down"), true);
    });
    addEngineAction(table, "focus-column-or-monitor-left", [](Engine::Private &d, const Config::Action &, std::optional<WindowId>) {
        return focusDirectionalOrMonitor(d, QStringLiteral("left"), false);
    });
    addEngineAction(table, "focus-column-or-monitor-right", [](Engine::Private &d, const Config::Action &, std::optional<WindowId>) {
        return focusDirectionalOrMonitor(d, QStringLiteral("right"), false);
    });
}

namespace
{

void registerSimpleMoveActions(ActionTable &table)
{
    addWorkspaceAction(table, "move-column-left", +[](Workspace &ws) { ws.moveLeft(); });
    addWorkspaceAction(table, "move-column-right", +[](Workspace &ws) { ws.moveRight(); });
    addWorkspaceAction(table, "move-column-to-first", +[](Workspace &ws) { ws.moveColumnToFirst(); });
    addWorkspaceAction(table, "move-column-to-last", +[](Workspace &ws) { ws.moveColumnToLast(); });
    addWorkspaceAction(table, "move-window-down", +[](Workspace &ws) { ws.moveDown(); });
    addWorkspaceAction(table, "move-window-up", +[](Workspace &ws) { ws.moveUp(); });
    addWorkspaceAction(table, "consume-window-into-column", +[](Workspace &ws) { ws.consumeIntoColumn(); });
    addWorkspaceAction(table, "expel-window-from-column", +[](Workspace &ws) { ws.expelFromColumn(); });
    addWorkspaceAction(table, "swap-window-left", +[](Workspace &ws) { ws.swapWindowInDirection(ScrollDirection::Left); });
    addWorkspaceAction(table, "swap-window-right", +[](Workspace &ws) { ws.swapWindowInDirection(ScrollDirection::Right); });
    addWorkspaceAction(table, "toggle-column-tabbed-display", +[](Workspace &ws) { ws.toggleColumnTabbedDisplay(); });
    addWorkspaceAction(table, "center-column", +[](Workspace &ws) { ws.centerColumn(); });
    addWorkspaceAction(table, "center-visible-columns", +[](Workspace &ws) { ws.centerVisibleColumns(); });
    addTargetAction(table, "center-window", +[](Workspace &ws, std::optional<WindowId> id) { ws.centerWindow(id); });
    addTargetAction(
        table, "consume-or-expel-window-left", +[](Workspace &ws, std::optional<WindowId> id) { ws.consumeOrExpelWindowLeft(id); });
    addTargetAction(
        table, "consume-or-expel-window-right", +[](Workspace &ws, std::optional<WindowId> id) { ws.consumeOrExpelWindowRight(id); });
    addMonitorAction(table, "move-window-down-or-to-workspace-down", +[](Monitor &m) { m.moveDownOrToWorkspaceDown(); });
    addMonitorAction(table, "move-window-up-or-to-workspace-up", +[](Monitor &m) { m.moveUpOrToWorkspaceUp(); });
}

ActionResult moveColumnOrMonitor(Engine::Private &d, const QString &direction)
{
    Workspace *workspace = d.activeWorkspace();
    if (workspace && (direction == QLatin1String("left") ? workspace->moveLeft() : workspace->moveRight())) {
        return {};
    }
    if (const auto idx = d.monitorInDirection(direction)) {
        d.moveColumnToMonitor(*idx, true);
    }
    return {};
}

}

void registerMoveActions(ActionTable &table)
{
    registerSimpleMoveActions(table);
    addEngineAction(table, "move-column-to-index", [](Engine::Private &d, const Config::Action &action, std::optional<WindowId>) {
        const auto index = parseIndex(actionArgument(action, 0));
        if (!index) {
            return actionError(index.error());
        }
        if (Workspace *workspace = d.activeWorkspace()) {
            workspace->moveColumnToIndex(static_cast<std::size_t>(*index));
        }
        return ActionResult();
    });
    addEngineAction(table, "set-column-display", [](Engine::Private &d, const Config::Action &action, std::optional<WindowId>) {
        const auto display = parseColumnDisplay(actionArgument(action, 0));
        if (!display) {
            return actionError(display.error());
        }
        if (Workspace *workspace = d.activeWorkspace()) {
            workspace->setColumnDisplay(*display);
        }
        return ActionResult();
    });
    addEngineAction(table, "move-column-left-or-to-monitor-left",
        [](Engine::Private &d, const Config::Action &, std::optional<WindowId>) { return moveColumnOrMonitor(d, QStringLiteral("left")); });
    addEngineAction(
        table, "move-column-right-or-to-monitor-right", [](Engine::Private &d, const Config::Action &, std::optional<WindowId>) {
            return moveColumnOrMonitor(d, QStringLiteral("right"));
        });
}

const ActionTable &actionTable()
{
    static const ActionTable table = [] {
        ActionTable result;
        registerFocusActions(result);
        registerMoveActions(result);
        registerWorkspaceActions(result);
        registerMonitorActions(result);
        registerSizeActions(result);
        registerWindowActions(result);
        registerCompositorActions(result);
        return result;
    }();
    return table;
}

ActionResult Engine::perform(const Config::Action &action, std::optional<WindowId> target)
{
    const ActionTable &table = actionTable();
    const auto handler = table.constFind(action.name);
    if (handler == table.constEnd()) {
        return actionError(QStringLiteral("unknown action: %1").arg(action.name));
    }
    d->layoutFocused = true;
    const ActionResult result = (*handler)(*d, action, target);
    d->refresh();
    return result;
}

}
