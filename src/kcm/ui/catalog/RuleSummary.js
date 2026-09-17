.pragma library
.import "Kdl.js" as Kdl

function childrenNamed(node, name) {
    return (node.children || []).filter(child => child.name === name);
}

function child(node, name) {
    return childrenNamed(node, name)[0];
}

function firstArg(node) {
    return node && node.args && node.args.length ? node.args[0] : undefined;
}

function appIdsOf(match) {
    const regex = match.props["app-id"];
    if (regex === undefined) {
        return [];
    }
    const parsed = Kdl.parseTextMatch(regex);
    if (parsed.mode === "is") {
        return [parsed.text];
    }
    const group = /^\^\(([^()]+)\)\$$/.exec(regex);
    if (!group) {
        return [];
    }
    const ids = group[1].split("|").map(Kdl.unescapeRegex);
    return ids.every(id => id !== null && id.length) ? ids : [];
}

function joinNames(names) {
    return names.length <= 2 ? names.join(" and ") : names.slice(0, -1).join(", ") + " and " + names[names.length - 1];
}

function describeText(label, regex) {
    const parsed = Kdl.parseTextMatch(regex);
    switch (parsed.mode) {
    case "is":
        return label + " “" + parsed.text + "”";
    case "starts":
        return label + " starting with “" + parsed.text + "”";
    case "ends":
        return label + " ending with “" + parsed.text + "”";
    case "contains":
        return label + " containing “" + parsed.text + "”";
    default:
        return label + " matching a pattern";
    }
}

const stateWords = {
    "is-active": ["active", "inactive"],
    "is-focused": ["focused", "unfocused"],
    "is-active-in-column": ["active in its column", "not active in its column"],
    "is-floating": ["floating", "tiled"],
    "is-urgent": ["urgent", "not urgent"],
    "at-startup": ["at startup", "after startup"]
};

function describeMatch(match, nameFor) {
    const parts = [];
    const ids = appIdsOf(match);
    if (ids.length) {
        parts.push(joinNames(ids.map(nameFor)));
    } else if (match.props["app-id"] !== undefined) {
        parts.push(describeText("apps", match.props["app-id"]));
    }
    if (match.props.title !== undefined) {
        parts.push(describeText(parts.length ? "titled" : "windows titled", match.props.title));
    }
    if (match.props["monitor-profile"] !== undefined) {
        parts.push("on " + describeText("profile", match.props["monitor-profile"]));
    }
    for (const key in stateWords) {
        if (match.props[key] !== undefined) {
            parts.push(stateWords[key][match.props[key] ? 0 : 1]);
        }
    }
    return parts.length ? parts.join(" ") : "All windows";
}

function target(rule, nameFor) {
    const matches = childrenNamed(rule, "match");
    let text = matches.length ? matches.map(match => describeMatch(match, nameFor)).join(" or ") : "All windows";
    const excludes = childrenNamed(rule, "exclude");
    if (excludes.length) {
        text += ", except " + excludes.map(match => describeMatch(match, nameFor)).join(" or ");
    }
    return /^(apps|windows|all) /.test(text) ? Kdl.titleCase(text) : text;
}

function boolEffect(rule, name, yes, no) {
    const value = firstArg(child(rule, name));
    return value === undefined ? [] : [value ? yes : no];
}

function sizeEffect(rule, name, noun) {
    const node = child(rule, name);
    if (!node) {
        return [];
    }
    const inner = (node.children || [])[0];
    if (!inner) {
        return ["app picks its " + (noun === "wide" ? "width" : "height")];
    }
    return [Kdl.sizeLabel({ kind: inner.name, value: inner.args[0] }) + " " + noun];
}

function effects(rule) {
    const list = [];
    list.push(...boolEffect(rule, "manage", "tiled", "left to KDE"));
    const position = firstArg(child(rule, "column-position"));
    if (position !== undefined) {
        list.push("pinned to the " + position + " of the row");
    }
    const group = firstArg(child(rule, "group-app-windows"));
    if (group !== undefined) {
        list.push(({ off: "doesn't group with its app", beside: "opens beside its app", stack: "stacks with its app" })[group]);
    }
    const maxRows = firstArg(child(rule, "max-rows-per-column"));
    if (maxRows !== undefined) {
        list.push("up to " + maxRows + " stacked per column");
    }
    list.push(...boolEffect(rule, "open-floating", "opens floating", "opens tiled"));
    list.push(...boolEffect(rule, "float-child-windows", "floats its extra windows", "tiles its extra windows"));
    list.push(...boolEffect(rule, "open-maximized", "opens maximized", "doesn't open maximized"));
    list.push(...boolEffect(rule, "open-maximized-to-edges", "opens maximized to the edges", "doesn't open maximized to the edges"));
    list.push(...boolEffect(rule, "open-fullscreen", "opens fullscreen", "doesn't open fullscreen"));
    list.push(...boolEffect(rule, "open-focused", "takes focus when it opens", "opens without focus"));
    const workspace = firstArg(child(rule, "open-on-workspace"));
    if (workspace !== undefined) {
        list.push("opens on workspace “" + workspace + "”");
    }
    const output = firstArg(child(rule, "open-on-output"));
    if (output !== undefined) {
        list.push("opens on " + output);
    }
    list.push(...sizeEffect(rule, "default-column-width", "wide"));
    list.push(...sizeEffect(rule, "default-window-height", "tall"));
    const display = firstArg(child(rule, "default-column-display"));
    if (display !== undefined) {
        list.push(display === "tabbed" ? "opens as tabs" : "opens as a normal column");
    }
    if (child(rule, "default-floating-position")) {
        list.push("custom floating position");
    }
    if (["min-width", "max-width", "min-height", "max-height"].some(name => child(rule, name))) {
        list.push("size limits");
    }
    const opacity = firstArg(child(rule, "opacity"));
    if (opacity !== undefined) {
        list.push(Math.round(opacity * 100) + "% opaque");
    }
    const radius = child(rule, "geometry-corner-radius");
    if (radius) {
        const values = radius.args;
        const same = values.every(value => value === values[0]);
        list.push((same ? Math.round(values[0]) + " px" : "custom") + " rounded corners");
    }
    list.push(...boolEffect(rule, "clip-to-geometry", "clipped", "not clipped"));
    if (child(rule, "focus-ring")) {
        list.push("custom focus ring");
    }
    if (child(rule, "border")) {
        list.push("custom border");
    }
    return list;
}

function summary(rule) {
    const list = effects(rule);
    return list.length ? Kdl.titleCase(list.join(", ")) : "Doesn't change anything yet";
}

function appIds(rule) {
    return [].concat(...childrenNamed(rule, "match").map(appIdsOf));
}
