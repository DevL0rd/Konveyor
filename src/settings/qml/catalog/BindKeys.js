.pragma library

const modifierOrder = ["Mod", "Super", "Ctrl", "Alt", "Shift"];

const triggerKinds = [
    { id: "keyboard", label: "Keyboard", icon: "input-keyboard" },
    { id: "mouse", label: "Mouse button", icon: "input-mouse" },
    { id: "wheel", label: "Mouse wheel", icon: "input-mouse-click-middle" },
    { id: "touchpad", label: "Touchpad", icon: "input-touchpad" }
];

const pointerTriggers = {
    mouse: [
        { key: "MouseLeft", label: "Left", icon: "input-mouse-click-left" },
        { key: "MouseMiddle", label: "Middle", icon: "input-mouse-click-middle" },
        { key: "MouseRight", label: "Right", icon: "input-mouse-click-right" },
        { key: "MouseBack", label: "Back", icon: "go-previous" },
        { key: "MouseForward", label: "Forward", icon: "go-next" }
    ],
    wheel: [
        { key: "WheelScrollUp", label: "Up", icon: "go-up" },
        { key: "WheelScrollLeft", label: "Left", icon: "go-previous" },
        { key: "WheelScrollRight", label: "Right", icon: "go-next" },
        { key: "WheelScrollDown", label: "Down", icon: "go-down" }
    ],
    touchpad: [
        { key: "TouchpadScrollUp", label: "Up", icon: "go-up" },
        { key: "TouchpadScrollLeft", label: "Left", icon: "go-previous" },
        { key: "TouchpadScrollRight", label: "Right", icon: "go-next" },
        { key: "TouchpadScrollDown", label: "Down", icon: "go-down" }
    ]
};

const modifierAliases = {
    mod: "mod", ctrl: "ctrl", control: "ctrl", shift: "shift", alt: "alt", super: "super", win: "super",
    iso_level3_shift: "iso_level3_shift", mod5: "iso_level3_shift", iso_level5_shift: "iso_level5_shift", mod3: "iso_level5_shift"
};

function split(keyName) {
    const parts = keyName.split("+");
    return { modifiers: parts.slice(0, -1), key: parts.length ? parts[parts.length - 1] : "" };
}

function join(modifiers, key) {
    const ordered = modifierOrder.filter(modifier => modifiers.includes(modifier));
    return ordered.concat(modifiers.filter(modifier => !modifierOrder.includes(modifier))).concat([key]).join("+");
}

function kindOf(keyName) {
    const key = split(keyName).key.toLowerCase();
    for (const kind of ["mouse", "wheel", "touchpad"]) {
        if (pointerTriggers[kind].some(trigger => trigger.key.toLowerCase() === key)) {
            return kind;
        }
    }
    return "keyboard";
}

function signature(keyName, modKey) {
    const parts = split(keyName);
    const mod = modifierAliases[modKey.toLowerCase()] || "super";
    const modifiers = parts.modifiers.map(part => {
        const alias = modifierAliases[part.trim().toLowerCase()] || part.trim().toLowerCase();
        return alias === "mod" ? mod : alias;
    });
    return Array.from(new Set(modifiers)).sort().join("+") + "|" + parts.key.toLowerCase();
}

function conflicts(keyNames, modKey) {
    const seen = {};
    for (const name of keyNames) {
        const sig = signature(name, modKey);
        seen[sig] = (seen[sig] || 0) + 1;
    }
    const result = {};
    for (const name of keyNames) {
        result[name] = seen[signature(name, modKey)] > 1;
    }
    return result;
}
