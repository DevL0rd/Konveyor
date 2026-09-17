.pragma library

function find(store) {
    const rule = store.children("", "window-rule").find(entry => !(entry.node.children || []).some(child => child.name === "match" || child.name === "exclude")) || null;
    const nodes = rule ? (rule.node.children || []) : [];
    const radiusNode = nodes.find(child => child.name === "geometry-corner-radius");
    const args = radiusNode ? radiusNode.args : [0];
    const clipNode = nodes.find(child => child.name === "clip-to-geometry");
    return {
        path: rule ? rule.path : "",
        radii: args.length === 4 ? args.map(Number) : [0, 1, 2, 3].map(() => Number(args[0])),
        clip: clipNode ? clipNode.args[0] !== false : false
    };
}
