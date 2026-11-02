.pragma library
.import "Kdl.js" as Kdl
.import "BorderPresets.js" as BorderPresets

const maxColors = 8;

const blends = [
    { value: "normal", label: "Normal", caption: "Paints over what's under it" },
    { value: "add", label: "Add", caption: "Adds light, keeps its color" },
    { value: "screen", label: "Screen", caption: "Brightens, can pale when stacked" },
    { value: "multiply", label: "Multiply", caption: "Darkens, fades on dark wallpaper" }
];

const directions = [
    { value: "out", label: "Out" },
    { value: "up", label: "Up" },
    { value: "down", label: "Down" },
    { value: "along", label: "Along" },
    { value: "random", label: "Random" }
];

const commonFields = [
    { key: "speed", label: "Speed", from: -10, to: 10, step: 0.1, decimals: 1 },
    { key: "width", label: "Width", description: "0 uses the thickness of the focus ring or border", from: 0, to: 64, step: 1, decimals: 0, unit: "px" },
    { key: "opacity", label: "Opacity", from: 0, to: 1, step: 0.05, decimals: 2 }
];

const baselineField = { key: "baseline", label: "At rest", description: "How much shows with no signal at all", from: 0, to: 1, step: 0.01, decimals: 2 };

const thicknessField = { key: "thickness", label: "Line thickness", from: 0.5, to: 12, step: 0.5, decimals: 1, unit: "px" };

const blockFields = [
    { key: "sensitivity", label: "Sensitivity", from: 0, to: 8, step: 0.1, decimals: 1, undriven: true },
    { key: "smoothing", label: "Smoothing", from: 0, to: 2, step: 0.05, decimals: 2, unit: "s", undriven: true },
    baselineField
];

const kinds = [
    { value: "gradient", label: "Gradient", tooltip: "A gradient across the frame, turning slowly. One color and speed 0 is a solid border.",
      speedHint: "At 1 it turns once in 8 seconds.",
      colorsHint: "Colors across the frame",
      template: [["colors", "accent"], ["speed", 0]],
      fields: [{ key: "angle", label: "Angle", description: "Which way the colors run across the frame", from: -360, to: 360, step: 1, decimals: 0, unit: "°" }] },
    { value: "sweep", label: "Sweep", tooltip: "A rotating ring of colors",
      speedHint: "At 1 it goes around once in 4 seconds.",
      colorsHint: "Colors around the ring",
      template: [["colors", "accent", "#ffffff"], ["blend", "add"]],
      fields: [] },
    { value: "comet", label: "Comet", tooltip: "Bright heads with fading tails running around the frame",
      speedHint: "At 1 a comet goes around once in 4 seconds.",
      colorsHint: "The first color is the head, the rest fade along the tail",
      template: [["colors", "#ffffff", "accent"], ["tail", 0.3], ["blend", "add"]],
      fields: [
          { key: "count", label: "Comets", from: 1, to: 64, step: 1, decimals: 0 },
          { key: "tail", label: "Tail length", description: "As a share of the whole frame", from: 0, to: 1, step: 0.01, decimals: 2 }
      ] },
    { value: "glow", label: "Glow", tooltip: "A breathing glow outside the frame",
      speedHint: "At 1 one breath takes 3 seconds.",
      colorsHint: "Colors it breathes between",
      template: [["colors", "accent"], ["width", 18], ["opacity", 0.7], ["blend", "add"]],
      fields: [] },
    { value: "ripple", label: "Ripple", tooltip: "A wave around the frame. Plays once when an event effect or a state change starts it.",
      speedHint: "At 1 the wave takes 0.8 seconds.",
      colorsHint: "Colors from the top of the frame to the bottom",
      template: [["colors", "#ffffff"], ["opacity", 0.8], ["blend", "add"]],
      fields: [] },
    { value: "pulse-front", label: "Pulse front", group: "Events", oneShot: true, tooltip: "A light front spreading out from a point, across every window it reaches",
      speedHint: "At 1 the front travels about 2200 px per second.",
      colorsHint: "Colors from the front edge back along its trail",
      template: [["colors", "#ffffff", "accent"], ["blend", "add"]],
      fields: [] },
    { value: "wave", label: "Wave", group: "Events", oneShot: true, tooltip: "A band of light travelling from the old window to the new one",
      speedHint: "At 1 the band travels about 2400 px per second.",
      colorsHint: "Colors from the front edge back along its trail",
      template: [["colors", "#ffffff", "accent"], ["blend", "add"]],
      fields: [] },
    { value: "draw-in", label: "Draw in", group: "Events", oneShot: true, tooltip: "The frame traces itself in, then fades",
      speedHint: "At 1 it takes 0.75 seconds.",
      colorsHint: "Colors around the frame",
      template: [["colors", "#ffffff", "accent"], ["blend", "add"]],
      fields: [] },
    { value: "fade-out", label: "Fade out", group: "Events", oneShot: true, tooltip: "The frame lights up and fades away",
      speedHint: "At 1 it takes 0.45 seconds.",
      colorsHint: "Colors across the frame",
      template: [["colors", "accent"], ["blend", "add"]],
      fields: [] },
    { value: "dashes", label: "Dashes", tooltip: "Marching ants",
      speedHint: "At 1 the dashes move one dash and gap every half second.",
      colorsHint: "Colors around the frame",
      template: [["colors", "#ffffff"]],
      fields: [
          { key: "dash", label: "Dash length", from: 1, to: 200, step: 1, decimals: 0, unit: "px" },
          { key: "gap", label: "Gap length", from: 0, to: 200, step: 1, decimals: 0, unit: "px" }
      ] },
    { value: "pulse", label: "Pulse", tooltip: "A pulsing band with a soft halo, good for windows that need attention",
      speedHint: "At 1 one pulse takes 1.5 seconds.",
      colorsHint: "Colors it pulses between",
      template: [["colors", "#ff1f1f", "#ff8a1f"], ["speed", 1.5]],
      fields: [] },
    { value: "particles", label: "Particles", tooltip: "Sparks flying from the frame",
      speedHint: "At 1 sparks fly about 40 px per second.",
      colorsHint: "Color ramp, from birth to death",
      template: [["colors", "#ffd27a", "#ff5a1f"], ["blend", "add"]],
      fields: [
          { key: "count", label: "How many", description: "The most that can be alive at once", from: 1, to: 256, step: 1, decimals: 0 },
          { key: "rate", label: "Rate", description: "New sparks per second", from: 1, to: 500, step: 1, decimals: 0, unit: "/s" },
          { key: "size", label: "Size", description: "How big each spark is", from: 0.5, to: 32, step: 0.5, decimals: 1, unit: "px" },
          { key: "lifetime", label: "Lifetime", description: "How long each spark lives", from: 0.1, to: 20, step: 0.1, decimals: 1, unit: "s" },
          { key: "burst", label: "Burst size", description: "Sparks per firing, when something fires it", from: 0, to: 256, step: 1, decimals: 0 }
      ] },
    { value: "equalizer", label: "Equalizer", group: "Audio", noSpeed: true, tooltip: "Bars rising outward from the frame, by band",
      colorsHint: "From the foot of a bar to its top",
      template: [["colors", "#2fd6ff", "#ff2fd0"], ["width", 16], ["blend", "add"]],
      fields: [
          { key: "count", label: "Bars", from: 4, to: 64, step: 1, decimals: 0 },
          { key: "gap", label: "Gap", description: "Space between bars", from: 0, to: 40, step: 1, decimals: 0, unit: "px" }
      ].concat(blockFields) },
    { value: "waveform", label: "Waveform", group: "Audio", noSpeed: true, tooltip: "The sound wave as a line around the frame",
      colorsHint: "Colors around the frame",
      template: [["colors", "#2fffb0", "#2f7bff"], ["width", 14], ["blend", "add"]],
      fields: [{ key: "thickness", label: "Line thickness", from: 0.5, to: 12, step: 0.5, decimals: 1, unit: "px" }].concat(blockFields) },
    { value: "spectrum", label: "Spectrum", group: "Audio", noSpeed: true, tooltip: "The frame lit by band, low at the bottom round to high at the top",
      colorsHint: "From lows at the bottom to highs at the top",
      template: [["colors", "#ff3b3b", "#ffb03b", "#3bff6a", "#3bc8ff"], ["width", 5]],
      fields: blockFields },
    { value: "shimmer", label: "Shimmer", group: "Audio", tooltip: "Fine sparkle along the edge, by default with the treble",
      speedHint: "At 1 the twinkle repeats every 8 seconds.",
      colorsHint: "Colors the sparkles pick from",
      template: [["colors", "#ffffff", "#9fd8ff"], ["width", 6], ["blend", "add"]],
      fields: [baselineField] },
    { value: "bass-thump", label: "Bass thump", group: "Audio", noSpeed: true, tooltip: "Width and brightness jump with the bass",
      colorsHint: "From quiet to loudest",
      template: [["colors", "accent", "#ffffff"], ["width", 6], ["blend", "add"]],
      fields: [baselineField] },
    { value: "speed-lines", label: "Speed lines", group: "Scroll", directed: true, tooltip: "Streaks along the edges in the scroll direction",
      speedHint: "At 1 the streaks go around once in 4 seconds.",
      colorsHint: "From the head of a streak to its end",
      template: [["colors", "#ffffff", "accent"], ["width", 8], ["blend", "add"]],
      fields: [
          { key: "count", label: "Lines", from: 1, to: 64, step: 1, decimals: 0 },
          { key: "tail", label: "Length", description: "Longest streak, as a share of the whole frame", from: 0, to: 1, step: 0.01, decimals: 2 },
          baselineField
      ] },
    { value: "scroll-trail", label: "Scroll trail", group: "Scroll", directed: true, noSpeed: true, tooltip: "A light on the frame that runs with how far you scroll",
      colorsHint: "From the head of the light to its end",
      template: [["colors", "#ffffff", "accent"], ["width", 5], ["blend", "add"]],
      fields: [
          { key: "tail", label: "Trail length", description: "As a share of the whole frame", from: 0, to: 1, step: 0.01, decimals: 2 },
          baselineField
      ] },
    { value: "bulge", label: "Bulge", group: "Scroll", directed: true, tooltip: "The border swells out in round bumps that travel up and down the edges",
      speedHint: "At 1 a new bump passes every second.",
      colorsHint: "From the thin border between bumps to the top of a bump",
      template: [["colors", "accent", "#ffffff"], ["width", 22]],
      fields: [
          { key: "count", label: "Bumps", description: "How many on each side", from: 1, to: 16, step: 1, decimals: 0 },
          { key: "tail", label: "Bump length", description: "As a share of the room each bump has", from: 0.05, to: 1, step: 0.01, decimals: 2 },
          thicknessField,
          baselineField
      ] },
    { value: "wavy", label: "Wavy line", group: "Scroll", directed: true, tooltip: "The border line itself undulates like a sine wave along the edges",
      speedHint: "At 1 a new crest passes every second.",
      colorsHint: "From the top of the frame to the bottom",
      template: [["colors", "#ffffff", "accent"], ["width", 12], ["blend", "add"]],
      fields: [
          { key: "count", label: "Waves", description: "How many crests on each side", from: 1, to: 64, step: 1, decimals: 0 },
          thicknessField,
          baselineField
      ] },
    { value: "elastic", label: "Elastic", group: "Scroll", directed: true, noSpeed: true, tooltip: "The edges stretch outward the way you scroll, then spring back past their rest",
      colorsHint: "From the frame out to the stretched line",
      template: [["colors", "accent", "#ffffff"], ["width", 28], ["blend", "add"]],
      fields: [thicknessField, baselineField] },
    { value: "chevrons", label: "Chevrons", group: "Scroll", directed: true, tooltip: "Arrows run along the left and right edges the way you scroll",
      speedHint: "At 1 a new arrow passes every second.",
      colorsHint: "From the tip of an arrow to its arms",
      template: [["colors", "#ffffff", "accent"], ["width", 20], ["blend", "add"]],
      fields: [
          { key: "count", label: "Arrows", description: "How many on each side", from: 1, to: 64, step: 1, decimals: 0 },
          thicknessField,
          baselineField
      ] },
    { value: "edge-flash", label: "Edge flash", group: "Scroll", directed: true, noSpeed: true, tooltip: "The top or bottom edge lights up, by the scroll direction",
      colorsHint: "From the frame outward",
      template: [["colors", "#ffffff", "accent"], ["width", 14], ["blend", "add"]],
      fields: [
          { key: "tail", label: "Down the sides", description: "How far the light runs, as a share of the height", from: 0, to: 1, step: 0.01, decimals: 2 },
          baselineField
      ] },
    { value: "gauge", label: "Gauge", group: "Scroll", noSpeed: true, tooltip: "A marker slides along the right edge with how far you scroll",
      colorsHint: "The marker's color from the top of its travel to the bottom",
      template: [["colors", "#ffffff", "accent"], ["width", 8]],
      fields: [
          { key: "tail", label: "Marker length", description: "As a share of the height", from: 0.02, to: 1, step: 0.01, decimals: 2 },
          { key: "position", label: "Position", description: "Where it sits with no signal", from: 0, to: 1, step: 0.01, decimals: 2 }
      ] }
];

function kindOf(value) {
    return kinds.find(kind => kind.value === value) || kinds[0];
}

function maxCount(kind) {
    return (kindOf(kind).fields.find(field => field.key === "count") || { to: 64 }).to;
}

function fieldsOf(kind) {
    return kindOf(kind).fields;
}

function commonFieldsOf(kind) {
    return kindOf(kind).noSpeed ? commonFields.filter(field => field.key !== "speed") : commonFields;
}

function canFire(kind) {
    return kind === "particles" || kind === "ripple" || kindOf(kind).oneShot === true;
}

function groupedKinds() {
    const labels = ["General", "Events", "Audio", "Scroll"];
    return labels.map(label => ({ label: label, kinds: kinds.filter(kind => (kind.group || "General") === label) }));
}

function groupedPresets() {
    const labels = ["Ambient", "Audio", "Input", "Scroll", "Focus"];
    return labels.map(label => ({ label: label, presets: BorderPresets.presets.filter(preset => (preset.group || "Ambient") === label) }))
        .filter(group => group.presets.length > 0);
}

function driveNode(drive) {
    const props = { amount: drive.amount };
    if (drive.smooth) {
        props.smooth = drive.smooth;
    }
    if (drive.curve && drive.curve !== "linear") {
        props.curve = drive.curve;
    }
    return Kdl.leaf("drive", [drive.setting, drive.signal], props);
}

function speedDescription(kind) {
    return kindOf(kind).speedHint + (kind === "ripple" ? " 0 turns it off." : " Negative runs it backwards, 0 holds it still.");
}

function newLayer(kind) {
    return Kdl.block("layer", kindOf(kind).template.map(entry => Kdl.leaf(entry[0], entry.slice(1))), [kind]);
}

function presetNode(preset, name) {
    const layers = preset.layers.map(entry => {
        const settings = Object.keys(entry[1]).filter(key => key !== "drives").map(key => {
            const value = entry[1][key];
            return Kdl.leaf(key, typeof value === "object" ? Array.from(value) : [value]);
        });
        return Kdl.block("layer", settings.concat((entry[1].drives || []).map(driveNode)), [entry[0]]);
    });
    return Kdl.block("border-style", layers, [name]);
}

function presetValues(store, preset) {
    return store.borderStyleLayers(presetNode(preset, preset.name));
}

function kindValues(store, kind) {
    return store.borderStyleLayers(Kdl.block("border-style", [newLayer(kind)], [kind]));
}

function layersNamed(styles, name) {
    return (styles.find(style => style.name === name) || { layers: [] }).layers;
}

function targetPart(values) {
    const ringOn = (values["focus-ring"] || {}).enabled === true;
    const borderOn = (values.border || {}).enabled === true;
    return !ringOn && borderOn ? "border" : "focus-ring";
}

function partLabel(part) {
    return part === "border" ? "border" : "focus ring";
}

function nameOf(entry) {
    return String(((entry.node || {}).args || [])[0] || "");
}

function uniqueName(base, taken) {
    if (!taken.includes(base)) {
        return base;
    }
    let number = 2;
    while (taken.includes(base + "-" + number)) {
        ++number;
    }
    return base + "-" + number;
}

function summary(layers) {
    return layers.length ? layers.map(layer => kindOf(layer.kind).label).join(" + ") : "No layers yet";
}

function referencePaths(store) {
    const scopes = ["layout"];
    for (const section of ["output", "monitor-profile", "workspace"]) {
        store.children("", section).forEach(entry => scopes.push(entry.path + "/layout"));
    }
    store.children("", "window-rule").forEach(entry => scopes.push(entry.path));
    const paths = [];
    for (const scope of scopes) {
        for (const part of ["focus-ring", "border"]) {
            for (const state of ["active", "inactive", "urgent"]) {
                paths.push(scope + "/" + part + "/" + state + "-style");
            }
        }
    }
    return paths;
}

function referencesTo(store, name) {
    return referencePaths(store).filter(path => (store.node(path).args || [])[0] === name);
}

function rename(store, stylePath, oldName, newName) {
    const references = referencesTo(store, oldName);
    if (store.setNode(stylePath, { name: "border-style", args: [newName], props: {} })) {
        references.forEach(path => store.setValue(path, [newName]));
    }
}

function remove(store, stylePath, name) {
    const references = referencesTo(store, name);
    if (!store.remove(stylePath)) {
        return false;
    }
    references.forEach(path => store.remove(path));
    return true;
}

function usage(ring, border, name) {
    const uses = [];
    const collect = (values, title) => {
        const states = [["active", "focused"], ["inactive", "inactive windows"], ["urgent", "needs attention"]];
        const used = states.filter(state => values && values[state[0] + "-style"] === name).map(state => state[1]);
        if (used.length) {
            uses.push(title + ": " + used.join(", "));
        }
    };
    collect(ring, "Focus ring");
    collect(border, "Border");
    return uses.join(" · ");
}
