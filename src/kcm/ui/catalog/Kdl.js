.pragma library

function hex2(value) {
    return Math.round(value * 255).toString(16).padStart(2, "0");
}

function cssColor(color) {
    const base = "#" + hex2(color.r) + hex2(color.g) + hex2(color.b);
    return color.a >= 0.999 ? base : base + hex2(color.a);
}

function leaf(name, args, props) {
    return { name: name, args: args || [], props: props || {} };
}

function block(name, children, args, props) {
    return { name: name, args: args || [], props: props || {}, children: children };
}

function sizeNode(size) {
    return leaf(size.kind, [size.kind === "fixed" ? Math.round(size.value) : Number(size.value.toFixed(5))]);
}

function sizeBlock(name, size) {
    return block(name, size ? [sizeNode(size)] : []);
}

function sizesBlock(name, sizes) {
    return block(name, sizes.map(sizeNode));
}

function sizeLabel(size) {
    if (!size) {
        return "App decides";
    }
    if (size.kind === "fixed") {
        return Math.round(size.value) + " px";
    }
    const percent = size.value * 100;
    return (Math.abs(percent - Math.round(percent)) < 0.05 ? Math.round(percent) : percent.toFixed(1)) + "%";
}

const themeColors = [
    { value: "accent", label: "Accent color" },
    { value: "focus", label: "Focus color" },
    { value: "hover", label: "Hover color" },
    { value: "window", label: "Window background" },
    { value: "window-text", label: "Window text" },
    { value: "inactive-text", label: "Inactive text" }
];

function paintMode(paint) {
    if (!paint) {
        return "none";
    }
    if (paint.gradient) {
        return "gradient";
    }
    return paint.source === "color" ? "color" : "theme";
}

function escapeRegex(text) {
    return text.replace(/[.*+?^${}()|[\]\\\/]/g, "\\$&");
}

function unescapeRegex(text) {
    if (/(^|[^\\])[.*+?^${}()|[\]]/.test(text)) {
        return null;
    }
    return text.replace(/\\(.)/g, "$1");
}

function textMatch(mode, text) {
    const escaped = escapeRegex(text);
    switch (mode) {
    case "is":
        return "^" + escaped + "$";
    case "starts":
        return "^" + escaped;
    case "ends":
        return escaped + "$";
    case "contains":
        return escaped;
    default:
        return text;
    }
}

function parseTextMatch(regex) {
    if (regex === undefined || regex === null) {
        return { mode: "is", text: "" };
    }
    const starts = regex.startsWith("^");
    const ends = regex.endsWith("$") && !regex.endsWith("\\$");
    const inner = regex.slice(starts ? 1 : 0, ends ? regex.length - 1 : regex.length);
    const plain = unescapeRegex(inner);
    if (plain === null) {
        return { mode: "pattern", text: regex };
    }
    const mode = starts && ends ? "is" : starts ? "starts" : ends ? "ends" : "contains";
    return { mode: mode, text: plain };
}

function titleCase(text) {
    return text.length ? text.charAt(0).toUpperCase() + text.slice(1) : text;
}

function humanize(name) {
    return titleCase(name.replace(/-/g, " "));
}
