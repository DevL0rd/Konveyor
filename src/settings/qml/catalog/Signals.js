.pragma library

const groups = [
    { label: "Audio", signals: [
        { value: "audio.level", label: "Volume" },
        { value: "audio.bass", label: "Bass" },
        { value: "audio.mid", label: "Mids" },
        { value: "audio.treble", label: "Treble" },
        { value: "audio.beat", label: "Beat", impulse: true }] },
    { label: "Input", signals: [
        { value: "scroll.speed", label: "Scroll speed" },
        { value: "scroll.distance", label: "Scroll distance" },
        { value: "scroll.direction", label: "Scroll direction" },
        { value: "pointer.speed", label: "Pointer speed" },
        { value: "typing.rate", label: "Typing speed" },
        { value: "click", label: "Click", impulse: true },
        { value: "scroll.step", label: "Scroll step", impulse: true },
        { value: "keypress", label: "Keypress", impulse: true }] },
    { label: "Konveyor", signals: [
        { value: "view.speed", label: "View scroll speed" },
        { value: "view.distance", label: "View scroll distance" },
        { value: "view.direction", label: "View scroll direction" },
        { value: "view.step", label: "Column change", impulse: true }] },
    { label: "Window", signals: [
        { value: "time", label: "Time" },
        { value: "focus", label: "Focused", impulse: true },
        { value: "unfocus", label: "Lost focus", impulse: true },
        { value: "urgent", label: "Needs attention", impulse: true }] }
];

const curves = [
    { value: "linear", label: "Linear" },
    { value: "square", label: "Punchy" },
    { value: "sqrt", label: "Gentle" },
    { value: "invert", label: "Inverted" }
];

const badges = {
    audio: { tooltip: "Reacts to this window's sound" },
    input: { icon: "input-mouse", tooltip: "Reacts to the pointer and keyboard" },
    view: { icon: "view-split-left-right", tooltip: "Reacts to Konveyor's own scrolling" }
};

const blockKinds = ["equalizer", "waveform", "spectrum"];

function pickerEntries() {
    const entries = [{ value: "", label: "Nothing" }];
    groups.forEach(group => group.signals.forEach(signal => entries.push({ value: signal.value, label: group.label + ": " + signal.label })));
    return entries;
}

function impulses() {
    const found = [];
    groups.forEach(group => group.signals.filter(signal => signal.impulse).forEach(signal => found.push({ value: signal.value, label: signal.label })));
    return found;
}

function sourceOf(signal) {
    if (signal.startsWith("audio.")) {
        return "audio";
    }
    if (signal.startsWith("view.")) {
        return "view";
    }
    return ["click", "keypress"].includes(signal) || /^(scroll|pointer|typing)\./.test(signal) ? "input" : "";
}

function layerSources(layer) {
    const found = (layer.drives || []).map(drive => drive.amount !== 0 ? sourceOf(drive.signal) : "").concat((layer.fireOn || []).map(sourceOf));
    if (blockKinds.includes(layer.kind)) {
        found.push("audio");
    }
    return found.filter(source => source.length > 0);
}

function sourcesOf(layers) {
    const found = new Set();
    (layers || []).forEach(layer => layerSources(layer).forEach(source => found.add(source)));
    return Array.from(found);
}

function tooltipFor(layers) {
    const sources = sourcesOf(layers);
    if (!sources.length) {
        return "";
    }
    return sources.map(source => badges[source].tooltip).join("\n") + "\nSimulated beat";
}
