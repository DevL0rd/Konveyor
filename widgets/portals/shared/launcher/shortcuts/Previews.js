.pragma library

const row = (widths, focused, extra) => widths.map((width, index) => Object.assign({ width: width }, index === focused ? { focused: true } : {}, (extra || {})[index] || {}));

const simple = (before, after, glyph) => ({ kind: "row", before: before, after: after, glyph: glyph });

const previews = [
    [/^focus-column-left|^focus-column-first/, simple({ columns: row([0.22, 0.28, 0.22], 1) }, { columns: row([0.22, 0.28, 0.22], 0) }, "go-previous")],
    [/^focus-column-right|^focus-column-last/, simple({ columns: row([0.22, 0.28, 0.22], 1) }, { columns: row([0.22, 0.28, 0.22], 2) }, "go-next")],
    [/^move-column-left|^move-column-to-first/, simple({ columns: row([0.22, 0.28, 0.22], 1) }, { columns: row([0.28, 0.22, 0.22], 0) }, "go-previous")],
    [/^move-column-right|^move-column-to-last/, simple({ columns: row([0.22, 0.28, 0.22], 1) }, { columns: row([0.22, 0.22, 0.28], 2) }, "go-next")],
    [/^consume-or-expel-window-left|^consume-window-into-column/, simple({ columns: row([0.26, 0.26, 0.26], 1) }, { columns: row([0.3, 0.26], 0, { 0: { stack: 2 } }) }, "go-previous")],
    [/^consume-or-expel-window-right/, simple({ columns: row([0.26, 0.26, 0.26], 1) }, { columns: row([0.26, 0.3], 1, { 1: { stack: 2 } }) }, "go-next")],
    [/^expel-window-from-column/, simple({ columns: row([0.26, 0.3, 0.26], 1, { 1: { stack: 2 } }) }, { columns: row([0.22, 0.24, 0.22, 0.22], 2) }, "go-next")],
    [/^toggle-column-tabbed-display/, simple({ columns: row([0.26, 0.34, 0.26], 1, { 1: { stack: 3 } }) }, { columns: row([0.26, 0.34, 0.26], 1, { 1: { tabs: 3 } }) }, "tab-new")],
    [/^switch-preset-column-width|^set-column-width/, simple({ columns: row([0.2, 0.26, 0.2], 1) }, { columns: row([0.2, 0.46, 0.2], 1) }, "zoom-fit-width")],
    [/^expand-column-to-available-width/, simple({ columns: row([0.2, 0.3, 0.2], 1) }, { columns: row([0.2, 0.56, 0.2], 1) }, "zoom-fit-width")],
    [/^switch-preset-window-height|^set-window-height|^reset-window-height/, simple({ columns: row([0.26, 0.34, 0.26], 1, { 1: { stack: 2 } }) }, { columns: row([0.26, 0.34, 0.26], 1, { 1: { stack: 3 } }) }, "zoom-fit-height")],
    [/^cycle-window-expansion|^maximize/, simple({ columns: row([0.24, 0.3, 0.24], 1) }, { columns: row([1], 0) }, "window-maximize")],
    [/^fullscreen-window/, { kind: "fullscreen", glyph: "view-fullscreen" }],
    [/^center-column|^center-visible-columns/, simple({ offset: 0.14, columns: row([0.3, 0.3, 0.3], 2) }, { offset: -0.1, columns: row([0.3, 0.3, 0.3], 2) }, "align-horizontal-center")],
    [/^toggle-window-floating|^switch-focus-between-floating/, { kind: "floating", glyph: "window-keep-above" }],
    [/^close-window/, simple({ columns: row([0.24, 0.3, 0.24], 1) }, { columns: row([0.24, 0.24], -1) }, "window-close")],
    [/^spawn/, simple({ columns: row([0.26, 0.26], 1) }, { columns: row([0.26, 0.26, 0.26], 2, { 2: { ghost: true } }) }, "list-add")],
    [/workspace/, { kind: "workspace", glyph: "go-down" }],
    [/monitor/, { kind: "monitor", glyph: "go-next" }],
];

function previewFor(id) {
    if (!id) {
        return null;
    }
    for (const [pattern, preview] of previews) {
        if (pattern.test(id)) {
            const direction = /left|up|first|previous|back/.test(id) ? "back" : "forward";
            return Object.assign({ direction: direction, vertical: /up|down|workspace|window-height|window-or/.test(id) }, preview);
        }
    }
    return null;
}
