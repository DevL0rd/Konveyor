.pragma library
.import "Kdl.js" as Kdl

const events = [
    { value: "focus", label: "Focus", phrase: "on focus", description: "A window gains focus", icon: "window-new", mode: false, defaultMode: "once" },
    { value: "unfocus", label: "Unfocus", phrase: "on unfocus", description: "A window loses focus", icon: "window", mode: false, defaultMode: "once" },
    { value: "click", label: "Click", phrase: "on click", description: "You click in a window", icon: "input-mouse", mode: false, defaultMode: "once" },
    { value: "scroll", label: "Scroll", phrase: "on scroll", description: "You scroll in a window", icon: "input-mouse", mode: true, defaultMode: "while" },
    { value: "view", label: "View scroll", phrase: "on view scroll", description: "Konveyor's own view scrolls", icon: "view-fullscreen", mode: true, defaultMode: "while" },
    { value: "keypress", label: "Keypress", phrase: "on keypress", description: "You type in a window. Which key is never recorded.", icon: "input-keyboard", mode: true, defaultMode: "once" },
    { value: "urgent", label: "Needs attention", phrase: "on needs attention", description: "A window asks for your attention", icon: "emblem-important", mode: true, defaultMode: "once" },
    { value: "beat", label: "Beat", phrase: "on beat", description: "The window's sound hits a beat", icon: "audio-volume-high", mode: false, defaultMode: "once" }
];

const reaches = [
    { value: "self", label: "This window", caption: "Only the window itself" },
    { value: "neighbors", label: "Neighbors", caption: "The windows right beside it" },
    { value: "all", label: "All windows", caption: "Every window on the workspace" }
];

const modes = [
    { value: "once", label: "Once" },
    { value: "while", label: "While it lasts" }
];

function eventInfo(value) {
    return events.find(entry => entry.value === value);
}

function blockPath(scopePath) {
    return scopePath + "/event-effects";
}

function nodeEvent(node) {
    return String((node.args || [])[0] || "");
}

function ownEntry(store, scopePath, event) {
    return store.children(blockPath(scopePath), "on").find(entry => nodeEvent(entry.node) === event) || null;
}

function shippedNode(store, scopePath, event) {
    for (let index = 0; ; ++index) {
        const node = store.defaultNode(blockPath(scopePath) + "/on#" + index);
        if (node.name === undefined) {
            return null;
        }
        if (nodeEvent(node) === event) {
            return node;
        }
    }
}

function binding(event, style, props) {
    const info = eventInfo(event);
    const source = props || {};
    return {
        style: style || "none",
        reach: source.reach || "self",
        mode: source.mode || (info ? info.defaultMode : "once"),
        duration: Number(source.duration || 0)
    };
}

function resolved(store, values, scopePath, event) {
    const own = ownEntry(store, scopePath, event);
    if (own) {
        return binding(event, String((own.node.args || [])[1] || "none"), own.node.props);
    }
    return Object.assign(binding(event), (values["event-effects"] || {})[event] || {});
}

function nodeFor(event, value) {
    const info = eventInfo(event);
    const props = {};
    if (value.style !== "none" && value.reach !== "self") {
        props.reach = value.reach;
    }
    if (value.style !== "none" && info && value.mode !== info.defaultMode) {
        props.mode = value.mode;
    }
    if (value.style !== "none" && value.duration > 0) {
        props.duration = value.duration;
    }
    return Kdl.leaf("on", [event, value.style], props);
}

function write(store, scopePath, event, value) {
    const own = ownEntry(store, scopePath, event);
    const node = nodeFor(event, value);
    return own ? store.setNode(own.path, node) : store.append(blockPath(scopePath), node).length > 0;
}

function clear(store, scopePath, event, overrideMode, value) {
    const own = ownEntry(store, scopePath, event);
    if (!overrideMode && !shippedNode(store, scopePath, event)) {
        return own ? store.remove(own.path) : true;
    }
    return write(store, scopePath, event, Object.assign({}, value, { style: "none" }));
}

function comparable(node) {
    return node ? JSON.stringify([(node.args || []).map(String), node.props || {}]) : "";
}

function isModified(store, scopePath, event) {
    const own = ownEntry(store, scopePath, event);
    return comparable(own ? own.node : null) !== comparable(shippedNode(store, scopePath, event));
}

function reset(store, scopePath, event) {
    const own = ownEntry(store, scopePath, event);
    const shipped = shippedNode(store, scopePath, event);
    if (!shipped) {
        return own ? store.remove(own.path) : true;
    }
    return own ? store.setNode(own.path, shipped) : store.append(blockPath(scopePath), shipped).length > 0;
}

function crosses(layers) {
    return (layers || []).some(layer => layer.kind === "wave");
}

function reached(index, subject, reach, layers) {
    if (index === subject || reach === "all" || crosses(layers)) {
        return true;
    }
    return reach === "neighbors" && Math.abs(index - subject) === 1;
}

function farthest(rects, subject, reach) {
    const from = rects[subject];
    let most = 0;
    rects.forEach((rect, index) => {
        if (index === subject || reach === "all" || (reach === "neighbors" && Math.abs(index - subject) === 1)) {
            const dx = Math.max(from.x - rect.x, rect.x + rect.width - from.x - from.width, 0);
            const dy = Math.max(from.y - rect.y, rect.y + rect.height - from.y - from.height, 0);
            most = Math.max(most, Math.hypot(dx, dy));
        }
    });
    return most;
}

function play(rects, subject, previous, reach, time, length) {
    const empty = Qt.rect(0, 0, 0, 0);
    const distance = farthest(rects, subject, reach);
    return rects.map(rect => ({
        time: time,
        frame: rect,
        subject: rects[subject],
        from: previous >= 0 ? rects[previous] : empty,
        to: previous >= 0 ? rects[subject] : empty,
        farthest: distance,
        length: length
    }));
}

function blockNode(effects) {
    const source = effects || {};
    return Kdl.block("event-effects", events.filter(info => source[info.value])
        .map(info => nodeFor(info.value, Object.assign(binding(info.value), source[info.value]))));
}

function summary(effects) {
    const source = effects || {};
    const used = events.filter(info => source[info.value] && source[info.value].style !== "none")
        .map(info => info.label + ": " + source[info.value].style);
    return used.length ? used.join(", ") : "None";
}
