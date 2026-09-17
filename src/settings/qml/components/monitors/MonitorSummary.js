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
    if (props["aspect-ratio-above"] !== undefined) {
        parts.push("wider than " + ratioLabel(props["aspect-ratio-above"]));
    }
    if (props["aspect-ratio-below"] !== undefined) {
        parts.push("narrower than " + ratioLabel(props["aspect-ratio-below"]));
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

function overrideCount(node) {
    const layout = (node.children || []).find(child => child.name === "layout");
    return layout && layout.children ? layout.children.length : 0;
}

function nameOf(entry) {
    return String((entry.node.args || [])[0] || "");
}
