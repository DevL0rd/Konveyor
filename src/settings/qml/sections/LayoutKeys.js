.pragma library
.import "../catalog/Kdl.js" as Kdl

function paintNodes(prefix, paint) {
    if (!paint) {
        return [];
    }
    const colorName = prefix ? prefix + "-color" : "color";
    const gradientName = prefix ? prefix + "-gradient" : "gradient";
    if (paint.gradient) {
        return [Kdl.leaf(gradientName, [], Kdl.gradientProps(paint.gradient))];
    }
    return [Kdl.leaf(colorName, [paint.source === "color" ? Kdl.cssColor(Qt.color(paint.color)) : paint.source])];
}

function ringNode(name, ring) {
    return Kdl.block(name, [Kdl.leaf(ring.enabled ? "on" : "off"), Kdl.leaf("width", [ring.width])]
        .concat(paintNodes("active", ring.active), paintNodes("inactive", ring.inactive), paintNodes("urgent", ring.urgent)));
}

function tabIndicatorNode(tab) {
    return Kdl.block("tab-indicator", [
        Kdl.leaf(tab.enabled ? "on" : "off"),
        Kdl.leaf("hide-when-single-tab", [tab["hide-when-single-tab"]]),
        Kdl.leaf("place-within-column", [tab["place-within-column"]]),
        Kdl.leaf("position", [tab.position]),
        Kdl.leaf("width", [tab.width]),
        Kdl.leaf("gap", [tab.gap]),
        Kdl.leaf("length", [], { "total-proportion": tab.length }),
        Kdl.leaf("gaps-between-tabs", [tab["gaps-between-tabs"]]),
        Kdl.leaf("corner-radius", [tab["corner-radius"]])
    ].concat(paintNodes("active", tab.active), paintNodes("inactive", tab.inactive), paintNodes("urgent", tab.urgent)));
}

function strutsNode(struts) {
    return Kdl.block("struts", ["left", "right", "top", "bottom"].map(side => Kdl.leaf(side, [Math.round(struts[side])])));
}

function nodeFor(key, value) {
    switch (key) {
    case "preset-column-widths":
    case "preset-window-heights":
        return Kdl.sizesBlock(key, value);
    case "default-column-width":
        return Kdl.sizeBlock(key, value);
    case "struts":
        return strutsNode(value);
    case "focus-ring":
    case "border":
        return ringNode(key, value);
    case "tab-indicator":
        return tabIndicatorNode(value);
    case "insert-hint":
        return Kdl.block(key, [Kdl.leaf(value.enabled ? "on" : "off")].concat(paintNodes("", value.paint)));
    case "background-color":
        return Kdl.leaf(key, [Kdl.cssColor(Qt.color(value))]);
    default:
        return Kdl.leaf(key, [value]);
    }
}

function write(store, scopePath, key, value) {
    return store.setNode(scopePath + "/" + key, nodeFor(key, value));
}

function flagValue(node, fallback) {
    if (!node || node.name === undefined) {
        return fallback;
    }
    return node.args.length === 0 || node.args[0] === true;
}

function writeFlag(store, scopePath, overrideMode, key, on, defaultOn) {
    const path = scopePath + "/" + key;
    if (!overrideMode && on === flagValue(store.defaultNode(path), defaultOn)) {
        return store.resetToDefault(path);
    }
    return store.setValue(path, on ? [] : [false]);
}
