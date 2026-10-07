.pragma library

function act(text, icon, action, extra) {
    return Object.assign({ text: text, icon: icon, action: action }, extra || {})
}

function konveyor(name, args, focus) {
    return { konveyor: name, args: args || [], focus: !!focus }
}

function task(name) {
    return { task: name }
}

function separator() {
    return { separator: true }
}

function section(text) {
    return { section: true, text: text }
}

function layoutEntries(context, i18n) {
    const list = [
        act(i18n("Wider"), "zoom-in", konveyor("set-column-width", ["+10%"])),
        act(i18n("Narrower"), "zoom-out", konveyor("set-column-width", ["-10%"])),
        act(i18n("Next Preset Width"), "distribute-horizontal-x", konveyor("switch-preset-column-width")),
        act(i18n("Full Width"), "zoom-fit-width", konveyor("maximize-column")),
        act(i18n("Maximize"), "window-maximize", konveyor("maximize-window-to-edges")),
        act(i18n("Center"), "align-horizontal-center", konveyor("center-window"))
    ]
    if (context.column)
        list.push(act(i18n("Show Column as Tabs"), "tab-new", konveyor("toggle-column-tabbed-display"), { checkable: true, checked: context.tabbed }))
    return list
}

function columnMoves(context, i18n) {
    const column = context.column
    const list = [
        act(i18n("Move Left"), "go-previous", konveyor("move-column-left"), { enabled: column.index > 1 }),
        act(i18n("Move Right"), "go-next", konveyor("move-column-right"), { enabled: column.index < column.count }),
        act(i18n("Move to the Start"), "go-first", konveyor("move-column-to-first"), { enabled: column.index > 1 }),
        act(i18n("Move to the End"), "go-last", konveyor("move-column-to-last"), { enabled: column.index < column.count }),
        separator()
    ]
    if (column.rows > 1) {
        list.push(act(i18n("Move Out to Its Own Column"), "view-split-left-right", konveyor("consume-or-expel-window-right")))
    } else {
        list.push(act(i18n("Join the Column on the Left"), "view-split-top-bottom", konveyor("consume-or-expel-window-left"), { enabled: column.index > 1 }))
        list.push(act(i18n("Join the Column on the Right"), "view-split-top-bottom", konveyor("consume-or-expel-window-right"),
            { enabled: column.index < column.count }))
    }
    return list
}

function placeEntries(context, i18n) {
    const list = []
    const workspaces = context.workspaces.map(workspace => act(workspace.name || i18n("Workspace %1", workspace.idx), "virtual-desktops",
        konveyor("move-window-to-workspace", [workspace.idx]), { enabled: !workspace.current }))
    if (workspaces.length > 0)
        list.push({ text: i18n("Move to Workspace"), icon: "virtual-desktops", children: workspaces })
    const outputs = context.outputs.filter(output => output !== context.output)
    if (outputs.length > 0)
        list.push({ text: i18n("Move to Monitor"), icon: "video-display", children: outputs.map(output => act(output, "video-display", konveyor("move-window-to-monitor", [output]))) })
    if (context.panelOutput && context.output && context.panelOutput !== context.output)
        list.push(act(i18n("Bring to This Screen"), "go-jump", konveyor("move-window-to-monitor", [context.panelOutput])))
    return list
}

function rememberEntries(context, i18n) {
    const place = context.rules
    const rules = place.rules
    const rule = (option, text, icon, checked, enabled) => act(text, icon, { rule: option, enabled: !checked },
        { checkable: true, checked: checked, enabled: enabled !== false })
    const column = rules.column !== null ? rules.column : place.column
    const workspace = rules.workspace !== null ? rules.workspace : (place.workspace_name || place.workspace)
    return [
        rule("column", i18n("Always Open as Column %1", column === null ? "–" : column), "view-split-left-right", rules.column !== null, column !== null),
        rule("workspace", i18n("Always Open on Workspace %1", workspace), "virtual-desktops", rules.workspace !== null),
        rule("monitor", i18n("Always Open on %1", rules.monitor || place.output), "video-display", rules.monitor !== null),
        rule("size", i18n("Open at This Size on %1", place.output), "transform-scale", rules.size !== null),
        rule("float", i18n("Always Float"), "window-keep-above", rules.float),
        rule("all-workspaces", i18n("Always Show on All Workspaces"), "window-pin", rules["all-workspaces"]),
        rule("stack", i18n("Always Stack With Its Other Windows"), "window-duplicate", rules.stack),
        rule("row", i18n("Always Open as a Row in the Current Column"), "view-split-top-bottom", rules.row),
        separator(),
        act(i18n("Edit Rules for %1…", context.appName), "configure", { editRules: true })
    ]
}

function konveyorEntries(context, i18n) {
    const list = [section(i18n("Konveyor")), { text: i18n("Size and Layout"), icon: "view-split-left-right", children: layoutEntries(context, i18n) }]
    const moves = (context.column ? columnMoves(context, i18n) : []).concat(placeEntries(context, i18n))
    if (moves.length > 0)
        list.push({ text: i18n("Move"), icon: "transform-move", children: moves })
    list.push(act(i18n("Float"), "window-keep-above", konveyor("toggle-window-floating"), { checkable: true, checked: context.floating }))
    list.push(act(i18n("Fullscreen"), "view-fullscreen", konveyor("fullscreen-window")))
    if (context.rules)
        list.push({ text: i18n("Remember for %1", context.appName), icon: "document-save", children: rememberEntries(context, i18n) })
    return list
}

function windowEntries(context, i18n) {
    const window = context.window
    const list = [
        act(window.onAllDesktops ? i18n("Only on This Workspace") : i18n("On All Workspaces"), "window-pin", task("allDesktops"),
            { checkable: true, checked: window.onAllDesktops }),
        act(window.minimized ? i18n("Restore") : i18n("Minimize"), "window-minimize", task("toggleMinimized"))
    ]
    return list
}

function newWindowEntries(context, i18n) {
    if (!context.window || !context.konveyor)
        return [act(i18n("Open New Window"), "window-new", task("newInstance"))]
    return [{ text: i18n("Open New Window"), icon: "window-new", children: [
        act(i18n("New Window"), "window-new", task("newInstance")),
        act(i18n("To the Right of This One"), "go-next", task("openRight")),
        act(i18n("As a Row Below This One"), "go-down", task("openBelow"))
    ] }]
}

function entries(context, i18n) {
    const list = []
    if (context.entry.windows.length > 1) {
        for (const window of context.entry.windows)
            list.push(act(window.title, window.icon, { task: "activate", window: window }, { checkable: true, checked: window.active }))
        list.push(separator())
    }
    list.push(...newWindowEntries(context, i18n))
    list.push(act(context.entry.pinned ? i18n("Unpin from Taskbar") : i18n("Pin to Taskbar"), context.entry.pinned ? "window-unpin" : "window-pin",
        task("togglePin")))
    if (!context.window)
        return list
    if (context.konveyor)
        list.push(...konveyorEntries(context, i18n))
    list.push(separator())
    list.push(...windowEntries(context, i18n))
    if (context.entry.windows.length > 1 && context.entry.kind === "group")
        list.push(act(i18n("Close All"), "window-close", task("closeAll")))
    list.push(act(i18n("Close"), "window-close", task("close")))
    return list
}
