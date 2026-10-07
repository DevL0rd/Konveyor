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

function moreEntries(context, i18n) {
    const window = context.window
    const toggle = (text, icon, name, checked) => act(text, icon, task(name), { checkable: true, checked: checked })
    return [{ text: i18n("More"), icon: "view-more-symbolic", children: [
        act(window.minimized ? i18n("Restore") : i18n("Minimize"), "window-minimize", task("toggleMinimized")),
        toggle(i18n("Maximize"), "window-maximize", "toggleMaximized", !!window.maximized),
        toggle(i18n("Keep Above Others"), "window-keep-above", "toggleKeepAbove", !!window.keepAbove),
        toggle(i18n("Keep Below Others"), "window-keep-below", "toggleKeepBelow", !!window.keepBelow),
        toggle(i18n("Shade"), "window-shade", "toggleShaded", !!window.shaded),
        toggle(i18n("On All Workspaces"), "window-pin", "allDesktops", !!window.onAllDesktops)
    ] }]
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

function appEntries(context, i18n) {
    const list = []
    const items = context.appActions || []
    for (const item of items.filter(each => each.actionId === "_kicker_jumpListAction"))
        list.push(act(item.text, item.icon, { appAction: item }))
    const recent = items.filter(each => each.actionId === "_kicker_recentDocument")
    if (recent.length > 0) {
        const forget = items.filter(each => each.actionId === "_kicker_forgetRecentDocuments")
        list.push({ text: i18n("Recent Files"), icon: "document-open-recent", children: recent.map(each => act(each.text, each.icon, { appAction: each }))
            .concat(forget.length ? [separator()].concat(forget.map(each => act(each.text, each.icon || "edit-clear-history", { appAction: each }))) : []) })
    }
    const player = context.player
    if (player && player.canControl) {
        list.push(section(player.track || i18n("Media")))
        list.push(act(player.playing ? i18n("Pause") : i18n("Play"), player.playing ? "media-playback-pause" : "media-playback-start", { media: "PlayPause" }))
        list.push(act(i18n("Previous Track"), "media-skip-backward", { media: "Previous" }, { enabled: !!player.canGoPrevious }))
        list.push(act(i18n("Next Track"), "media-skip-forward", { media: "Next" }, { enabled: !!player.canGoNext }))
    }
    return list.length > 0 ? list.concat([separator()]) : list
}

function keyLabel(key) {
    const names = { super: "Meta", mod: "Meta", win: "Meta", bracketleft: "[", bracketright: "]", comma: ",", period: ".", minus: "-", equal: "=",
        page_up: "PgUp", page_down: "PgDn", return: "Enter", space: "Space", escape: "Esc", slash: "/" }
    return String(key).split(/\+(?!$)/).map(part => names[part.toLowerCase()] || part).join("+")
}

function hintFor(binds, name, args) {
    const wanted = (args || []).map(String).join("\n")
    const bind = (binds || []).find(each => each.action && each.action.name === name && (each.action.arguments || []).map(String).join("\n") === wanted)
    return bind ? keyLabel(bind.key) : ""
}

function withHints(list, binds) {
    return list.map(entry => {
        const named = entry.action && (entry.action.konveyor || entry.action.bind)
        const hinted = named ? Object.assign({}, entry, { hint: hintFor(binds, named, entry.action.args) }) : Object.assign({}, entry)
        if (entry.children)
            hinted.children = withHints(entry.children, binds)
        return hinted
    })
}

function entries(context, i18n) {
    const list = appEntries(context, i18n)
    if (context.entry.windows.length > 1) {
        for (const window of context.entry.windows)
            list.push(act(window.title, window.icon, { task: "activate", window: window }, { checkable: true, checked: window.active }))
        list.push(separator())
    }
    if (context.window && context.konveyor)
        list.push(...konveyorEntries(context, i18n), separator())
    list.push(act(context.entry.pinned ? i18n("Unpin from Taskbar") : i18n("Pin to Taskbar"), context.entry.pinned ? "window-unpin" : "window-pin",
        task("togglePin")))
    list.push(...newWindowEntries(context, i18n))
    if (!context.window)
        return withHints(list, context.binds)
    list.push(...moreEntries(context, i18n), separator())
    if (context.entry.windows.length > 1 && context.entry.kind === "group")
        list.push(act(i18n("Close All"), "window-close", task("closeAll")))
    list.push(act(i18n("Close"), "window-close", { task: "close", bind: "close-window" }))
    return withHints(list, context.binds)
}
