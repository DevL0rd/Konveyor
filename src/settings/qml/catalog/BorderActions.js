.pragma library
.import "BorderStyles.js" as BorderStyles
.import "Kdl.js" as Kdl

function parsedStyles(store) {
    return store.values["border-styles"] || [];
}

function takenNames(store) {
    return parsedStyles(store).map(style => style.name)
        .concat(store.children("", "border-style").map(BorderStyles.nameOf), ["none"]);
}

function matchingName(styles, layers) {
    const wanted = JSON.stringify(layers);
    const found = styles.find(style => JSON.stringify(style.layers) === wanted);
    return found ? found.name : "";
}

function namesByLook(styles) {
    const names = {};
    for (const style of styles) {
        const look = JSON.stringify(style.layers);
        if (!(look in names)) {
            names[look] = style.name;
        }
    }
    return names;
}

function addCopy(store, preset) {
    return store.append("", BorderStyles.presetNode(preset, BorderStyles.uniqueName(preset.name, takenNames(store))));
}

function ensurePreset(store, preset) {
    const existing = matchingName(parsedStyles(store), BorderStyles.presetValues(store, preset));
    if (existing.length > 0) {
        return existing;
    }
    const name = BorderStyles.uniqueName(preset.name, takenNames(store));
    return store.append("", BorderStyles.presetNode(preset, name)).length > 0 ? name : "";
}

function paintArgument(paint) {
    return paint.source === "color" ? Kdl.cssColor(Qt.color(paint.color)) : paint.source;
}

function layerNode(layer) {
    const keys = ["speed", "width", "opacity", "blend"].concat(BorderStyles.fieldsOf(layer.kind).map(field => field.key));
    if (layer.kind === "particles") {
        keys.push("direction");
    }
    const children = keys.map(key => Kdl.leaf(key, [layer[key]])).concat((layer.drives || []).map(BorderStyles.driveNode));
    if ((layer.fireOn || []).length > 0) {
        children.push(Kdl.leaf("fire-on", Array.from(layer.fireOn)));
    }
    children.unshift(Kdl.leaf("colors", layer.colors.map(paintArgument)));
    return Kdl.block("layer", children, [layer.kind]);
}

function duplicateNode(store, entry, name) {
    const parsed = BorderStyles.layersNamed(parsedStyles(store), name);
    const children = entry ? entry.node.children || [] : parsed.map(layerNode);
    return Kdl.block("border-style", children, [BorderStyles.uniqueName(name + "-copy", takenNames(store))]);
}

function targetOn(values) {
    return (values[BorderStyles.targetPart(values)] || {}).enabled === true;
}

function inUse(values, name) {
    const part = BorderStyles.targetPart(values);
    return name.length > 0 && targetOn(values) && (values[part] || {})["active-style"] === name;
}

function useLabel(values, name) {
    return (inUse(values, name) ? "In use on " : "Use on ") + BorderStyles.partLabel(BorderStyles.targetPart(values));
}

function useTip(values) {
    const word = BorderStyles.partLabel(BorderStyles.targetPart(values));
    return "Use on the " + word + " of the focused window." + (targetOn(values) ? "" : " This also turns the " + word + " on.");
}

function use(store, values, name) {
    if (name.length === 0) {
        return;
    }
    const path = "layout/" + BorderStyles.targetPart(values);
    if (!targetOn(values)) {
        store.setToggle(path, true);
    }
    store.setValue(path + "/active-style", [name]);
}
