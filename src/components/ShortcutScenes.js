.pragma library
.import "Scenes.js" as Scenes

const W = 0.28;
const C = Scenes.columns;
const S = Scenes.scene;
const merge = Scenes.merge;

function col(key, extra) {
    return Object.assign({ key: key, w: W }, extra || {});
}

function hot(key, extra) {
    return col(key, Object.assign({ focus: key }, extra || {}));
}

function stack(keys, focus, extra) {
    return Object.assign({ keys: keys, w: W, focus: focus }, extra || {});
}

function arrowKey(back, vertical) {
    if (vertical) {
        return back ? "up" : "down";
    }
    return back ? "left" : "right";
}

function focusColumn(back, edge) {
    if (edge) {
        const rest = [col("a"), col("b"), col("c"), col("d")];
        const start = rest.map(c => c.key === (back ? "d" : "a") ? hot(c.key) : c);
        const end = rest.map(c => c.key === (back ? "a" : "d") ? hot(c.key) : c);
        const shift = -(W + 0.035);
        return S([C(start, { offset: back ? shift : 0 }), C(end, { offset: back ? 0 : shift })], { key: back ? "home" : "end" });
    }
    return S([C([col("a"), hot("b"), col("c")]), C([back ? hot("a") : col("a"), col("b"), back ? col("c") : hot("c")])], { key: arrowKey(back) });
}

function moveColumn(back, edge) {
    if (edge) {
        const from = [col("a"), hot("b"), col("c")];
        return S([C(from), C(back ? [hot("b"), col("a"), col("c")] : [col("a"), col("c"), hot("b")])], { key: back ? "home" : "end" });
    }
    return S([C([col("a"), hot("b"), col("c")]), C(back ? [hot("b"), col("a"), col("c")] : [col("a"), col("c"), hot("b")])], { key: arrowKey(back) });
}

function consume(id, back) {
    if (/^consume-window-into-column/.test(id)) {
        return S([C([hot("a"), col("b"), col("c")]), C([stack(["a", "b"], "a"), col("c")])], { key: "letter" });
    }
    if (/^expel-window-from-column/.test(id)) {
        return S([C([col("a"), stack(["b", "x"], "x"), col("c")]), C([col("a"), col("b"), hot("x"), col("c")])], { key: "letter" });
    }
    const start = C([col("a"), hot("b"), col("c")]);
    return S([start, C(back ? [stack(["a", "b"], "b"), col("c")] : [col("a"), stack(["c", "b"], "b")]), start], { key: arrowKey(back) });
}

function widthCycle(back) {
    const presets = [0.25, 0.5, 0.75, 1];
    const order = back ? presets.slice().reverse() : presets;
    return S(order.map(p => C([hot("a", { w: Scenes.presetWidth(p), label: Math.round(p * 100) + "%" }), col("b"), col("c")])), { cycle: true, key: "letter" });
}

function heightCycle(back) {
    const presets = [[1, 3], [1, 2], [1, 1], [2, 1]];
    const order = back ? presets.slice().reverse() : presets;
    return S(order.map(h => C([col("a"), stack(["b", "x"], "b", { heights: h }), col("c")])), { cycle: true, key: "letter" });
}

function sizing(id) {
    if (/^switch-preset-column-width|^switch-preset-window-width/.test(id)) {
        return widthCycle(/-back$/.test(id));
    }
    if (/^switch-preset-window-height/.test(id)) {
        return heightCycle(/-back$/.test(id));
    }
    if (/^set-column-width|^set-window-width/.test(id)) {
        return S([C([hot("a", { w: 0.28 }), col("b"), col("c")]), C([hot("a", { w: 0.42 }), col("b"), col("c")])], { key: "letter" });
    }
    if (/^set-window-height|^reset-window-height/.test(id)) {
        const uneven = C([col("a"), stack(["b", "x"], "b", { heights: [2.2, 1] }), col("c")]);
        const even = C([col("a"), stack(["b", "x"], "b"), col("c")]);
        return S(/^reset/.test(id) ? [uneven, even] : [even, uneven], { key: "letter" });
    }
    if (/^expand-column-to-available-width/.test(id)) {
        return S([C([col("a", { w: 0.24 }), hot("b", { w: 0.3 }), col("c", { w: 0.24 })]), C([col("a", { w: 0.24 }), hot("b", { w: 1 - 0.24 - 0.035 * 3 }), col("c", { w: 0.24 })])], { key: "letter" });
    }
    return S([C([col("a"), hot("b"), col("c")]), C([col("a"), hot("b", { w: Scenes.presetWidth(1) }), col("c")], { offset: -(W + 0.035) })], { key: "letter" });
}

function windowState(id) {
    const base = C([col("a"), hot("b"), col("c")]);
    if (/^fullscreen-window|^toggle-windowed-fullscreen/.test(id)) {
        return S([base, merge(base, { b: merge(base.b, { x: 0, y: 0, w: 1, h: 1, z: 2 }) })], { key: "letter" });
    }
    if (/^maximize-window-to-edges/.test(id)) {
        return S([base, merge(base, { b: merge(base.b, { x: 0.02, y: 0.04, w: 0.96, h: 0.92, z: 2 }) })], { key: "letter" });
    }
    const floating = { x: 0.26, y: 0.2, w: 0.48, h: 0.62, z: 2, f: 1 };
    if (/^switch-focus-between-floating|^focus-floating|^focus-tiling/.test(id)) {
        const tiled = C([hot("a"), col("c")]);
        const lifted = merge(tiled, { b: merge(tiled.a, floating, { f: 0 }) });
        return S([lifted, merge(tiled, { a: merge(tiled.a, { f: 0 }), b: merge(tiled.a, floating) })], { key: "letter" });
    }
    return S([base, merge(C([col("a"), col("c")]), { b: merge(base.b, floating) })], { key: "letter" });
}

function center() {
    const list = [col("a"), col("b"), hot("c", { w: 0.36 }), col("d")];
    const before = 2 * (W + 0.035) + 0.035;
    return S([C(list, { offset: 1 - 0.035 - 0.36 - before }), C(list, { offset: 0.5 - 0.18 - before })], { key: "letter" });
}

function lifecycle(id) {
    if (/^close-window/.test(id)) {
        return S([C([col("a"), hot("b"), col("c")]), C([col("a"), hot("c")])], { key: "letter" });
    }
    if (/^spawn|^system-run/.test(id)) {
        return S([C([col("a"), hot("b")]), C([col("a"), col("b"), hot("n")])], { key: "enter" });
    }
    return S([C([col("a"), hot("b"), col("c")]), C([col("a"), hot("b"), col("c")])], { key: "letter", panel: [0, 1] });
}

function workspaces(id, back) {
    const top = [col("a"), hot("b"), col("c")];
    const bottom = [col("d"), col("e")];
    const view = (shift, current, next) => merge(C(current, { workspace: shift }), C(next, { workspace: shift + 1 }));
    const nextFocus = [hot("d"), col("e")];
    if (/^move-workspace/.test(id)) {
        const frames = [view(0, top, bottom), view(0, bottom, top), view(-1, bottom, top)];
        return S(back ? frames.reverse() : frames, { key: arrowKey(back, true) });
    }
    if (/^move-(column|window)-to-workspace/.test(id)) {
        const start = back ? view(-1, bottom, top) : view(0, top, bottom);
        const rest = [col("a"), col("c")];
        const carried = [hot("b"), col("d"), col("e")];
        const end = back ? merge(C(carried, { workspace: 0 }), C(rest, { workspace: 1 })) : merge(C(rest, { workspace: -1 }), C(carried, { workspace: 0 }));
        return S([start, end], { key: arrowKey(back, true) });
    }
    if (/^move-window-(up|down)-or-to-workspace/.test(id)) {
        const inColumn = keys => [col("a"), stack(keys, "b"), col("c")];
        const first = back ? ["x", "b"] : ["b", "x"];
        const second = back ? ["b", "x"] : ["x", "b"];
        const landed = back ? merge(C([col("d"), hot("b")], { workspace: 0 }), C([col("a"), col("x"), col("c")], { workspace: 1 })) : merge(C([col("a"), col("x"), col("c")], { workspace: -1 }), C([hot("b"), col("d")], { workspace: 0 }));
        const other = [col("d")];
        const place = (keys) => back ? merge(C(other, { workspace: -1 }), C(inColumn(keys), { workspace: 0 })) : merge(C(inColumn(keys), { workspace: 0 }), C(other, { workspace: 1 }));
        return S([place(first), place(second), landed], { key: arrowKey(back, true) });
    }
    if (/^focus-window-or-workspace/.test(id)) {
        const inColumn = f => [col("a"), stack(back ? ["x", "b"] : ["b", "x"], f), col("c")];
        const place = (f, shift) => back ? merge(C(nextFocus.map(c => col(c.key)), { workspace: shift - 1 }), C(inColumn(f), { workspace: shift })) : merge(C(inColumn(f), { workspace: shift }), C(bottom, { workspace: shift + 1 }));
        const last = back ? merge(C(nextFocus, { workspace: 0 }), C(inColumn(""), { workspace: 1 })) : merge(C(inColumn(""), { workspace: -1 }), C(nextFocus, { workspace: 0 }));
        return S([place("b", 0), place("x", 0), last], { key: arrowKey(back, true) });
    }
    return S(back ? [view(-1, nextFocus.map(c => col(c.key)), top), view(0, nextFocus, top.map(c => col(c.key)))] : [view(0, top, bottom), view(-1, top.map(c => col(c.key)), nextFocus)], { key: /workspace$/.test(id) ? "number" : arrowKey(back, true) });
}

function focusWindow(id, back) {
    const place = f => C([col("a"), stack(["b", "x"], f), col("c")]);
    return S(back ? [place("x"), place("b")] : [place("b"), place("x")], { key: arrowKey(back, true) });
}

function moveWindow(id, back) {
    const place = keys => C([col("a"), stack(keys, "b"), col("c")]);
    return S(back ? [place(["x", "b"]), place(["b", "x"])] : [place(["b", "x"]), place(["x", "b"])], { key: arrowKey(back, true) });
}

function monitors(id, back) {
    const vertical = /up|down/.test(id);
    const near = back ? 1 : 0;
    const far = back ? 0 : 1;
    const moves = /^move-/.test(id);
    const start = merge(C([col("a", { w: 0.44 }), hot("b", { w: 0.44 })], { monitor: near }), C([col("c", { w: 0.44 })], { monitor: far }));
    const end = moves ? merge(C([col("a", { w: 0.44 })], { monitor: near }), C([col("c", { w: 0.44 }), hot("b", { w: 0.44 })], { monitor: far })) : merge(C([col("a", { w: 0.44 }), col("b", { w: 0.44 })], { monitor: near }), C([hot("c", { w: 0.44 })], { monitor: far }));
    return S([start, end], { monitors: 2, stackedMonitors: vertical, key: arrowKey(back, vertical) });
}

const table = [
    [/^focus-column-(left|right)$|^focus-column-(left|right)-or|^focus-column-or-monitor/, id => focusColumn(/left/.test(id), false)],
    [/^focus-column-(first|last)$/, id => focusColumn(/first/.test(id), true)],
    [/^focus-column$/, () => focusColumn(false, true)],
    [/^move-column-(left|right)$|^move-column-(left|right)-or|^swap-window/, id => moveColumn(/left/.test(id), false)],
    [/^move-column-to-(first|last)$|^move-column-to-index/, id => moveColumn(/first/.test(id), true)],
    [/^consume|^expel/, id => consume(id, /left/.test(id))],
    [/^switch-preset|^set-(column|window)-(width|height)|^reset-window-height|^expand-column|^maximize-column|^cycle-window-expansion/, sizing],
    [/^toggle-column-tabbed-display|^set-column-display/, () => S([C([col("a"), stack(["b", "x", "y"], "b"), col("c")]), C([col("a"), stack(["b", "x", "y"], "b", { tabbed: true }), col("c")])], { key: "letter" })],
    [/^fullscreen-window|^toggle-windowed-fullscreen|^maximize-window-to-edges|^toggle-window-floating|^move-window-to-(floating|tiling)|^switch-focus-between-floating|^focus-floating|^focus-tiling/, windowState],
    [/^center-/, center],
    [/^close-window|^spawn|^system-run|^show-hotkey-overlay|overview/, lifecycle],
    [/workspace/, id => workspaces(id, /up|previous/.test(id))],
    [/^focus-window-(up|down|top|bottom)|^focus-window-in-column/, id => focusWindow(id, /up|top/.test(id))],
    [/^move-window-(up|down)$/, id => moveWindow(id, /up/.test(id))],
    [/monitor/, id => monitors(id, /left|up/.test(id))]
];

const cache = {};

function sceneFor(id) {
    if (!id) {
        return null;
    }
    if (cache[id] !== undefined) {
        return cache[id];
    }
    const match = table.find(entry => entry[0].test(id));
    cache[id] = match ? match[1](id) : null;
    return cache[id];
}
