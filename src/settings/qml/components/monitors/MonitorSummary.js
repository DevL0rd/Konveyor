.pragma library
.import "../../catalog/Kdl.js" as Kdl

const ratios = [
    { value: 4 / 3, label: "4:3" },
    { value: 16 / 10, label: "16:10" },
    { value: 16 / 9, label: "16:9" },
    { value: 21 / 9, label: "21:9" },
    { value: 32 / 9, label: "32:9" }
];

function ratioLabel(value) {
    const known = ratios.find(ratio => Math.abs(ratio.value - value) < 0.04);
    return known ? known.label : Number(Number(value).toFixed(2)) + ":1";
}

function describeMatch(match) {
    const props = match.props || {};
    const parts = [];
    if (props.name !== undefined) {
        const parsed = Kdl.parseTextMatch(props.name);
        parts.push(parsed.mode === "is" ? "named " + parsed.text : "with a name matching a pattern");
    }
    if (props["aspect-ratio-below"] !== undefined && Number(props["aspect-ratio-below"]) <= 1 && props["aspect-ratio-above"] === undefined) {
        parts.push("portrait (taller than wide)");
    } else if (props["aspect-ratio-below"] !== undefined) {
        parts.push("narrower than " + ratioLabel(props["aspect-ratio-below"]));
    }
    if (props["aspect-ratio-above"] !== undefined) {
        parts.push("wider than " + ratioLabel(props["aspect-ratio-above"]));
    }
    if (props["width-above"] !== undefined) {
        parts.push("more than " + Math.round(props["width-above"]) + " px wide");
    }
    if (props["width-below"] !== undefined) {
        parts.push("less than " + Math.round(props["width-below"]) + " px wide");
    }
    if (props["height-above"] !== undefined) {
        parts.push("more than " + Math.round(props["height-above"]) + " px tall");
    }
    if (props["height-below"] !== undefined) {
        parts.push("less than " + Math.round(props["height-below"]) + " px tall");
    }
    return parts.length ? parts.join(", ") : "any monitor";
}

function describeProfile(profile) {
    const matches = (profile.children || []).filter(child => child.name === "match");
    if (!matches.length) {
        return "Every monitor that no earlier profile claims";
    }
    return Kdl.titleCase(matches.map(describeMatch).join(" or "));
}

function layoutChild(node, name) {
    const layout = (node.children || []).find(child => child.name === "layout");
    return layout ? (layout.children || []).find(child => child.name === name) : undefined;
}

function isPortrait(node) {
    return (node.children || []).some(child => child.name === "match" && child.props
        && child.props["aspect-ratio-below"] !== undefined && Number(child.props["aspect-ratio-below"]) <= 1);
}

function columnWidth(node) {
    const width = layoutChild(node, "default-column-width");
    const proportion = width ? (width.children || []).find(child => child.name === "proportion") : undefined;
    return proportion ? Number(proportion.args[0]) : undefined;
}

function describeLayout(node) {
    const parts = [];
    const width = columnWidth(node);
    if (width !== undefined) {
        parts.push(width >= 1 ? "full-width columns" : Math.round(width * 100) + "% columns");
    }
    const placement = layoutChild(node, "new-window-placement");
    const rows = layoutChild(node, "max-rows-per-column");
    if (placement && placement.args[0] === "stack") {
        parts.push(rows ? "stacks " + rows.args[0] + " per column" : "stacks new windows");
    } else if (placement) {
        parts.push("a column per window");
    }
    return parts.join(", ");
}

function overrideCount(node) {
    const layout = (node.children || []).find(child => child.name === "layout");
    return layout && layout.children ? layout.children.length : 0;
}

function nameOf(entry) {
    return String((entry.node.args || [])[0] || "");
}
