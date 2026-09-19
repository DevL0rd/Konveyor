#include "layout/engine/engineprivate.h"

#include "layout/common/sizechange.h"

#include <utility>

namespace Konveyor::Layout
{

namespace
{

using SizeSetter = void (*)(Workspace &, std::optional<WindowId>, SizeChange);

void addSizeAction(ActionTable &table, const char *name, SizeSetter setter)
{
    addEngineAction(table, name, [setter](Engine::Private &d, const Config::Action &action, std::optional<WindowId> target) {
        const auto change = parseSizeChange(actionArgument(action, 0));
        if (!change) {
            return actionError(change.error());
        }
        const auto window = d.target(actionWindowId(action, target));
        if (Workspace *workspace = d.workspaceForTarget(window)) {
            setter(*workspace, window, *change);
        }
        return ActionResult();
    });
}

void registerPresetActions(ActionTable &table)
{
    const auto addColumnWidthAction = [&table](const char *name, bool forwards) {
        addEngineAction(table, name, [forwards](Engine::Private &d, const Config::Action &action, std::optional<WindowId>) {
            if (Workspace *workspace = d.activeWorkspace()) {
                workspace->toggleWidth(forwards, actionFlag(action, QStringLiteral("from-native"), false));
            }
            return ActionResult();
        });
    };
    addColumnWidthAction("switch-preset-column-width", true);
    addColumnWidthAction("switch-preset-column-width-back", false);
    addTargetAction(
        table, "switch-preset-window-width", +[](Workspace &ws, std::optional<WindowId> id) { ws.toggleWindowWidth(id, true); });
    addTargetAction(
        table, "switch-preset-window-width-back", +[](Workspace &ws, std::optional<WindowId> id) { ws.toggleWindowWidth(id, false); });
    addTargetAction(
        table, "switch-preset-window-height", +[](Workspace &ws, std::optional<WindowId> id) { ws.toggleWindowHeight(id, true); });
    addTargetAction(
        table, "switch-preset-window-height-back", +[](Workspace &ws, std::optional<WindowId> id) { ws.toggleWindowHeight(id, false); });
    addTargetAction(table, "reset-window-height", +[](Workspace &ws, std::optional<WindowId> id) { ws.resetWindowHeight(id); });
    addWorkspaceAction(table, "maximize-column", +[](Workspace &ws) { ws.toggleFillWidth(); });
    addWorkspaceAction(table, "expand-column-to-available-width", +[](Workspace &ws) { ws.expandColumnToAvailableWidth(); });
}

ActionResult toggleWindowedFullscreen(Engine::Private &d, const Config::Action &action, std::optional<WindowId> target)
{
    const auto window = d.target(actionWindowId(action, target));
    if (!window) {
        return actionError(QStringLiteral("no window to toggle windowed fullscreen for"));
    }
    Workspace *workspace = d.workspaceOf(*window);
    if (!workspace) {
        return actionError(QStringLiteral("no such window"));
    }
    Tile *tile = workspace->tileFor(*window);
    if (tile && tile->window().requestedMode() == WindowMode::Fullscreen) {
        workspace->setFullscreen(*window, false);
    }
    if (Tile *updated = workspace->tileFor(*window)) {
        LayoutWindow &layoutWindow = updated->window();
        layoutWindow.requestWindowedFullscreen(!layoutWindow.windowedFullscreenRequested());
    }
    return {};
}

}

void registerSizeActions(ActionTable &table)
{
    registerPresetActions(table);
    addSizeAction(table, "set-column-width", +[](Workspace &ws, std::optional<WindowId>, SizeChange change) { ws.setColumnWidth(change); });
    addSizeAction(
        table, "set-window-width", +[](Workspace &ws, std::optional<WindowId> id, SizeChange change) { ws.setWindowWidth(id, change); });
    addSizeAction(
        table, "set-window-height", +[](Workspace &ws, std::optional<WindowId> id, SizeChange change) { ws.setWindowHeight(id, change); });
    addTargetAction(
        table, "maximize-window-to-edges", +[](Workspace &ws, std::optional<WindowId> id) {
            if (id) {
                ws.toggleMaximized(*id);
            }
        });
    addTargetAction(
        table, "cycle-window-expansion", +[](Workspace &ws, std::optional<WindowId> id) {
            if (id) {
                ws.cycleExpansion(*id);
            }
        });
    addTargetAction(
        table, "fullscreen-window", +[](Workspace &ws, std::optional<WindowId> id) {
            if (id) {
                ws.toggleFullscreen(*id);
            }
        });
    addEngineAction(table, "toggle-windowed-fullscreen", toggleWindowedFullscreen);
}

namespace
{

ActionResult moveFloatingWindow(Engine::Private &d, const Config::Action &action, std::optional<WindowId> target)
{
    const auto window = d.target(actionWindowId(action, target));
    PositionChange x {ChangeKind::AdjustFixed, 0.0};
    PositionChange y {ChangeKind::AdjustFixed, 0.0};
    if (const auto text = actionProperty(action, QStringLiteral("x"))) {
        const auto parsed = parsePositionChange(*text);
        if (!parsed) {
            return actionError(parsed.error());
        }
        x = *parsed;
    }
    if (const auto text = actionProperty(action, QStringLiteral("y"))) {
        const auto parsed = parsePositionChange(*text);
        if (!parsed) {
            return actionError(parsed.error());
        }
        y = *parsed;
    }
    if (Workspace *workspace = d.workspaceForTarget(window)) {
        workspace->moveFloatingWindow(window, x, y, true);
    }
    return {};
}

ActionResult setUrgency(Engine::Private &d, const Config::Action &action, std::optional<WindowId> target, int mode)
{
    const auto window = d.target(actionWindowId(action, target));
    if (!window) {
        return actionError(QStringLiteral("no window given"));
    }
    Workspace *workspace = d.workspaceOf(*window);
    if (!workspace) {
        return actionError(QStringLiteral("no such window"));
    }
    Tile *tile = workspace->tileFor(*window);
    if (!tile) {
        return actionError(QStringLiteral("no such window"));
    }
    const bool urgent = mode < 0 ? !tile->window().isUrgent() : mode > 0;
    tile->window().setUrgent(urgent);
    return {};
}

void registerOverviewActions(ActionTable &table)
{
    addEngineAction(table, "toggle-overview", [](Engine::Private &d, const Config::Action &, std::optional<WindowId>) {
        d.overviewOpen = !d.overviewOpen;
        for (Monitor &monitor : d.monitors) {
            monitor.overviewOpen = d.overviewOpen;
        }
        if (d.hooks.toggleOverview) {
            d.hooks.toggleOverview();
        }
        return ActionResult();
    });
    for (const bool open : {true, false}) {
        const char *name = open ? "open-overview" : "close-overview";
        addEngineAction(table, name, [open](Engine::Private &d, const Config::Action &, std::optional<WindowId>) {
            d.overviewOpen = open;
            for (Monitor &monitor : d.monitors) {
                monitor.overviewOpen = open;
            }
            if (d.hooks.setOverviewOpen) {
                d.hooks.setOverviewOpen(open);
            }
            return ActionResult();
        });
    }
}

}

void registerWindowActions(ActionTable &table)
{
    registerOverviewActions(table);
    addTargetAction(table, "toggle-window-floating", +[](Workspace &ws, std::optional<WindowId> id) { ws.toggleWindowFloating(id); });
    addTargetAction(table, "move-window-to-floating", +[](Workspace &ws, std::optional<WindowId> id) { ws.placeWindowFloating(id, true); });
    addTargetAction(table, "move-window-to-tiling", +[](Workspace &ws, std::optional<WindowId> id) { ws.placeWindowFloating(id, false); });
    addEngineAction(table, "move-floating-window", moveFloatingWindow);
    addEngineAction(table, "close-window", [](Engine::Private &d, const Config::Action &action, std::optional<WindowId> target) {
        const auto window = d.target(actionWindowId(action, target));
        if (!window) {
            return actionError(QStringLiteral("no window to close"));
        }
        if (d.hooks.closeWindow) {
            d.hooks.closeWindow(*window);
        }
        return ActionResult();
    });
    addEngineAction(
        table, "toggle-window-rule-opacity", [](Engine::Private &d, const Config::Action &action, std::optional<WindowId> target) {
            const auto window = d.target(actionWindowId(action, target));
            Workspace *workspace = window ? d.workspaceOf(*window) : nullptr;
            Tile *tile = workspace ? workspace->tileFor(*window) : nullptr;
            if (!tile) {
                return actionError(QStringLiteral("no such window"));
            }
            tile->window().toggleIgnoreOpacityRule();
            return ActionResult();
        });
    addEngineAction(table, "toggle-window-urgent",
        [](Engine::Private &d, const Config::Action &a, std::optional<WindowId> t) { return setUrgency(d, a, t, -1); });
    addEngineAction(table, "set-window-urgent",
        [](Engine::Private &d, const Config::Action &a, std::optional<WindowId> t) { return setUrgency(d, a, t, 1); });
    addEngineAction(table, "unset-window-urgent",
        [](Engine::Private &d, const Config::Action &a, std::optional<WindowId> t) { return setUrgency(d, a, t, 0); });
}

void registerCompositorActions(ActionTable &table)
{
    static const char *const names[] = {"spawn", "spawn-sh", "show-hotkey-overlay"};
    for (const char *name : names) {
        addForwardedAction(table, name);
    }
}

}
