.pragma library

const categories = [
    { id: "focus", label: "Focus", icon: "crosshairs" },
    { id: "move", label: "Move", icon: "transform-move" },
    { id: "size", label: "Size", icon: "zoom-fit-width" },
    { id: "windows", label: "Windows and tabs", icon: "window" },
    { id: "floating", label: "Floating", icon: "window-duplicate" },
    { id: "workspaces", label: "Workspaces", icon: "virtual-desktops" },
    { id: "monitors", label: "Monitors", icon: "video-display" },
    { id: "overview", label: "Overview", icon: "view-app-grid" },
    { id: "apps", label: "Apps and commands", icon: "system-run" }
];

function a(id, category, label, icon, argument, extra) {
    return Object.assign({ id: id, category: category, label: label, icon: icon, argument: argument || "none", followProperty: false }, extra || {});
}

const directions = [
    { suffix: "left", word: "on the left", icon: "go-previous" },
    { suffix: "right", word: "on the right", icon: "go-next" },
    { suffix: "up", word: "above", icon: "go-up" },
    { suffix: "down", word: "below", icon: "go-down" },
    { suffix: "previous", word: "used before", icon: "go-previous-view" },
    { suffix: "next", word: "next in order", icon: "go-next-view" }
];

function monitorFamily(prefix, verb) {
    return directions.map(direction => a(prefix + "-" + direction.suffix, "monitors", verb + " the monitor " + direction.word, direction.icon))
        .concat([a(prefix, "monitors", verb + " a specific monitor", "video-display", "monitor")]);
}

const actions = [
    a("focus-column-left", "focus", "Focus the column on the left", "go-previous"),
    a("focus-column-right", "focus", "Focus the column on the right", "go-next"),
    a("focus-column-first", "focus", "Focus the first column", "go-first"),
    a("focus-column-last", "focus", "Focus the last column", "go-last"),
    a("focus-column-left-or-last", "focus", "Focus left, wrapping around to the last column", "go-previous"),
    a("focus-column-right-or-first", "focus", "Focus right, wrapping around to the first column", "go-next"),
    a("focus-column", "focus", "Focus a column by position", "go-jump", "index"),
    a("focus-window-up", "focus", "Focus the window above", "go-up"),
    a("focus-window-down", "focus", "Focus the window below", "go-down"),
    a("focus-window-top", "focus", "Focus the top window of the column", "go-top"),
    a("focus-window-bottom", "focus", "Focus the bottom window of the column", "go-bottom"),
    a("focus-window-up-or-bottom", "focus", "Focus above, wrapping around to the bottom", "go-up"),
    a("focus-window-down-or-top", "focus", "Focus below, wrapping around to the top", "go-down"),
    a("focus-window-in-column", "focus", "Focus a window in the column by position", "go-jump", "index"),
    a("focus-window-up-or-column-left", "focus", "Focus above, or the column on the left", "go-up"),
    a("focus-window-up-or-column-right", "focus", "Focus above, or the column on the right", "go-up"),
    a("focus-window-down-or-column-left", "focus", "Focus below, or the column on the left", "go-down"),
    a("focus-window-down-or-column-right", "focus", "Focus below, or the column on the right", "go-down"),
    a("focus-window-or-workspace-up", "focus", "Focus above, or the workspace above", "go-up"),
    a("focus-window-or-workspace-down", "focus", "Focus below, or the workspace below", "go-down"),
    a("focus-window-or-monitor-up", "focus", "Focus above, or the monitor above", "go-up"),
    a("focus-window-or-monitor-down", "focus", "Focus below, or the monitor below", "go-down"),
    a("focus-column-or-monitor-left", "focus", "Focus left, or the monitor on the left", "go-previous"),
    a("focus-column-or-monitor-right", "focus", "Focus right, or the monitor on the right", "go-next"),
    a("focus-window-previous", "focus", "Focus the previously focused window", "view-history"),

    a("move-column-left", "move", "Move the column left", "go-previous"),
    a("move-column-right", "move", "Move the column right", "go-next"),
    a("move-column-to-first", "move", "Move the column to the start", "go-first"),
    a("move-column-to-last", "move", "Move the column to the end", "go-last"),
    a("move-column-to-index", "move", "Move the column to a position", "go-jump", "index"),
    a("move-column-left-or-to-monitor-left", "move", "Move the column left, or onto the monitor on the left", "go-previous"),
    a("move-column-right-or-to-monitor-right", "move", "Move the column right, or onto the monitor on the right", "go-next"),
    a("move-window-up", "move", "Move the window up in its column", "go-up"),
    a("move-window-down", "move", "Move the window down in its column", "go-down"),
    a("move-window-up-or-to-workspace-up", "move", "Move the window up, or to the workspace above", "go-up"),
    a("move-window-down-or-to-workspace-down", "move", "Move the window down, or to the workspace below", "go-down"),
    a("consume-or-expel-window-left", "move", "Stack into the column on the left, or pop out left", "object-columns"),
    a("consume-or-expel-window-right", "move", "Stack into the column on the right, or pop out right", "object-columns"),
    a("consume-window-into-column", "move", "Pull the next column's window into this column", "object-group"),
    a("expel-window-from-column", "move", "Pop the window out into its own column", "object-ungroup"),
    a("swap-window-left", "move", "Swap the window with the column on the left", "exchange-positions"),
    a("swap-window-right", "move", "Swap the window with the column on the right", "exchange-positions"),

    a("set-column-width", "size", "Set or change the column width", "zoom-fit-width", "size", { dimension: "width" }),
    a("set-window-width", "size", "Set or change the window width", "zoom-fit-width", "size", { dimension: "width" }),
    a("set-window-height", "size", "Set or change the window height", "zoom-fit-height", "size", { dimension: "height" }),
    a("switch-preset-column-width", "size", "Cycle through preset column widths", "view-refresh"),
    a("switch-preset-column-width-back", "size", "Cycle preset column widths backwards", "view-refresh"),
    a("switch-preset-window-width", "size", "Cycle through preset window widths", "view-refresh"),
    a("switch-preset-window-width-back", "size", "Cycle preset window widths backwards", "view-refresh"),
    a("switch-preset-window-height", "size", "Cycle through preset window heights", "view-refresh"),
    a("switch-preset-window-height-back", "size", "Cycle preset window heights backwards", "view-refresh"),
    a("reset-window-height", "size", "Reset the window height", "edit-undo"),
    a("maximize-column", "size", "Make the column full width", "zoom-fit-width"),
    a("maximize-window-to-edges", "size", "Maximize the window to the screen edges", "window-maximize"),
    a("cycle-window-expansion", "size", "Cycle full width, maximized and normal", "view-fullscreen"),
    a("expand-column-to-available-width", "size", "Grow the column into the free space", "zoom-fit-best"),
    a("center-column", "size", "Center the column", "align-horizontal-center"),
    a("center-window", "size", "Center the window", "align-horizontal-center"),
    a("center-visible-columns", "size", "Center all visible columns", "align-horizontal-center"),

    a("close-window", "windows", "Close the window", "window-close"),
    a("fullscreen-window", "windows", "Toggle fullscreen", "view-fullscreen"),
    a("toggle-windowed-fullscreen", "windows", "Toggle fullscreen inside its column", "view-restore"),
    a("toggle-column-tabbed-display", "windows", "Toggle tabs for the column", "tab-new"),
    a("set-column-display", "windows", "Show the column as tabs or a stack", "view-list-details", "display"),
    a("toggle-window-rule-opacity", "windows", "Toggle the window rule's opacity", "contrast"),

    a("toggle-window-floating", "floating", "Float or tile the window", "window-duplicate"),
    a("move-window-to-floating", "floating", "Float the window", "window-new"),
    a("move-window-to-tiling", "floating", "Tile the window", "view-split-left-right"),
    a("focus-floating", "floating", "Focus the floating windows", "window-duplicate"),
    a("focus-tiling", "floating", "Focus the tiled windows", "view-split-left-right"),
    a("switch-focus-between-floating-and-tiling", "floating", "Switch focus between floating and tiled", "exchange-positions"),

    a("focus-workspace", "workspaces", "Go to a workspace", "go-jump", "workspace"),
    a("focus-workspace-up", "workspaces", "Go to the workspace above", "go-up"),
    a("focus-workspace-down", "workspaces", "Go to the workspace below", "go-down"),
    a("focus-workspace-previous", "workspaces", "Go back to the previous workspace", "go-previous-view"),
    a("move-window-to-workspace", "workspaces", "Send the window to a workspace", "go-jump", "workspace", { followProperty: true }),
    a("move-window-to-workspace-up", "workspaces", "Send the window to the workspace above", "go-up", "none", { followProperty: true }),
    a("move-window-to-workspace-down", "workspaces", "Send the window to the workspace below", "go-down", "none", { followProperty: true }),
    a("move-column-to-workspace", "workspaces", "Send the column to a workspace", "go-jump", "workspace", { followProperty: true }),
    a("move-column-to-workspace-up", "workspaces", "Send the column to the workspace above", "go-up", "none", { followProperty: true }),
    a("move-column-to-workspace-down", "workspaces", "Send the column to the workspace below", "go-down", "none", { followProperty: true }),
    a("move-workspace-up", "workspaces", "Move this workspace up", "go-up"),
    a("move-workspace-down", "workspaces", "Move this workspace down", "go-down"),
    a("move-workspace-to-index", "workspaces", "Move this workspace to a position", "go-jump", "index"),
    a("set-workspace-name", "workspaces", "Give this workspace a name", "edit-rename", "text"),
    a("unset-workspace-name", "workspaces", "Remove this workspace's name", "edit-clear"),

    a("toggle-overview", "overview", "Open or close the overview", "view-app-grid"),
    a("open-overview", "overview", "Open the overview", "view-app-grid"),
    a("close-overview", "overview", "Close the overview", "window-close"),
    a("show-hotkey-overlay", "overview", "Show the shortcut cheatsheet", "preferences-desktop-keyboard-shortcut"),

    a("spawn", "apps", "Launch an app or command", "system-run", "command"),
    a("spawn-sh", "apps", "Run a shell command", "utilities-terminal", "shell")
].concat(monitorFamily("focus-monitor", "Focus"), monitorFamily("move-window-to-monitor", "Send the window to"),
    monitorFamily("move-column-to-monitor", "Send the column to"), monitorFamily("move-workspace-to-monitor", "Send this workspace to"));

function byId(id) {
    return actions.find(action => action.id === id) || null;
}

function category(id) {
    return categories.find(entry => entry.id === id) || null;
}

function sizeSummary(text, dimension) {
    const percent = text.endsWith("%");
    const body = percent ? text.slice(0, -1) : text;
    const unit = percent ? "%" : " px";
    if (body.startsWith("+")) {
        return "grow " + dimension + " by " + body.slice(1) + unit;
    }
    if (body.startsWith("-")) {
        return "shrink " + dimension + " by " + body.slice(1) + unit;
    }
    return dimension + " " + body + unit;
}

function argumentSummary(action, node) {
    const args = node.args || [];
    const first = args.length ? String(args[0]) : "";
    switch (action.argument) {
    case "index":
        return "#" + first;
    case "workspace":
        return typeof args[0] === "number" || /^\d+$/.test(first) ? "workspace " + first : "“" + first + "”";
    case "size":
        return sizeSummary(first, action.dimension);
    case "monitor":
        return first;
    case "display":
        return first === "tabbed" ? "tabs" : "stacked";
    case "command":
        return args.map(String).join(" ");
    case "shell":
    case "text":
        return first;
    }
    return "";
}

function describe(node) {
    if (!node) {
        return "No action";
    }
    const action = byId(node.name);
    if (!action) {
        return node.name;
    }
    const detail = argumentSummary(action, node);
    const stays = action.followProperty && node.props && node.props.focus === false ? " (stay here)" : "";
    return action.label + (detail.length ? ": " + detail : "") + stays;
}

function search(query) {
    const words = query.toLowerCase().split(/\s+/).filter(Boolean);
    return actions.filter(action => {
        const haystack = (action.label + " " + action.id + " " + category(action.category).label).toLowerCase();
        return words.every(word => haystack.includes(word));
    });
}
