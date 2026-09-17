.pragma library
.import "../Actions.js" as Actions

const entries = [
    { page: "shortcuts", section: "Mod key", label: "Key that “Mod” stands for", keywords: "mod key super meta alt ctrl modifier" },
    { page: "shortcuts", section: "Shortcuts", label: "Add a shortcut", keywords: "new bind keybinding hotkey keyboard mouse wheel touchpad" },
    { page: "shortcuts", section: "Shortcuts", label: "Restore default shortcuts", keywords: "reset defaults binds" },
    { page: "shortcuts", section: "Shortcuts", label: "Shortcut cheatsheet titles", keywords: "hotkey overlay cheatsheet hide title" },
    { page: "shortcuts", section: "Shortcuts", label: "Repeat and cooldown", keywords: "repeat held cooldown delay" }
].concat(Actions.actions.map(action => ({
    page: "shortcuts",
    section: Actions.category(action.category).label,
    label: action.label,
    keywords: "shortcut keybinding " + action.id.replace(/-/g, " ")
})));
