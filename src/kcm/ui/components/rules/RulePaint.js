.pragma library

function argbFromCss(text) {
    if (typeof text !== "string" || !text.startsWith("#")) {
        return text;
    }
    let hex = text.slice(1);
    if (hex.length === 3 || hex.length === 4) {
        hex = hex.split("").map(c => c + c).join("");
    }
    if (hex.length === 8) {
        return "#" + hex.slice(6, 8) + hex.slice(0, 6);
    }
    return "#" + hex;
}

const themeKeywords = ["accent", "focus", "hover", "window", "window-text", "inactive-text"];

function paintOf(block, state) {
    if (!block) {
        return null;
    }
    const children = block.children || [];
    const gradient = children.find(child => child.name === state + "-gradient");
    const color = children.find(child => child.name === state + "-color");
    if (gradient) {
        const props = gradient.props;
        return {
            source: "color",
            color: "",
            gradient: {
                from: argbFromCss(props.from),
                to: argbFromCss(props.to),
                angle: props.angle !== undefined ? props.angle : 180,
                "relative-to": props["relative-to"] || "window",
                "in": props["in"] || "srgb"
            }
        };
    }
    if (color) {
        const value = String(color.args[0]);
        const keyword = value.toLowerCase();
        if (themeKeywords.includes(keyword)) {
            return { source: keyword, color: "" };
        }
        return { source: "color", color: argbFromCss(value) };
    }
    return null;
}
