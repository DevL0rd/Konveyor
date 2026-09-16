#include "config/loader.h"
#include "config/sections.h"

#include <xkbcommon/xkbcommon.h>

namespace Konveyor::Config
{

namespace
{

constexpr const char *kSimpleActions[] = {"suspend", "power-off-monitors", "power-on-monitors", "toggle-debug-tint",
    "debug-toggle-opaque-regions", "debug-toggle-damage", "toggle-keyboard-shortcuts-inhibit", "close-window", "fullscreen-window",
    "toggle-windowed-fullscreen", "focus-window-previous", "focus-column-left", "focus-column-right", "focus-column-first",
    "focus-column-last", "focus-column-right-or-first", "focus-column-left-or-last", "focus-window-or-monitor-up",
    "focus-window-or-monitor-down", "focus-column-or-monitor-left", "focus-column-or-monitor-right", "focus-window-down", "focus-window-up",
    "focus-window-down-or-column-left", "focus-window-down-or-column-right", "focus-window-up-or-column-left",
    "focus-window-up-or-column-right", "focus-window-or-workspace-down", "focus-window-or-workspace-up", "focus-window-top",
    "focus-window-bottom", "focus-window-down-or-top", "focus-window-up-or-bottom", "move-column-left", "move-column-right",
    "move-column-to-first", "move-column-to-last", "move-column-left-or-to-monitor-left", "move-column-right-or-to-monitor-right",
    "move-window-down", "move-window-up", "move-window-down-or-to-workspace-down", "move-window-up-or-to-workspace-up",
    "consume-or-expel-window-left", "consume-or-expel-window-right", "consume-window-into-column", "expel-window-from-column",
    "swap-window-left", "swap-window-right", "toggle-column-tabbed-display", "center-column", "center-window", "center-visible-columns",
    "focus-workspace-down", "focus-workspace-up", "focus-workspace-previous", "move-workspace-down", "move-workspace-up",
    "unset-workspace-name", "focus-monitor-left", "focus-monitor-right", "focus-monitor-down", "focus-monitor-up", "focus-monitor-previous",
    "focus-monitor-next", "move-window-to-monitor-left", "move-window-to-monitor-right", "move-window-to-monitor-down",
    "move-window-to-monitor-up", "move-window-to-monitor-previous", "move-window-to-monitor-next", "move-column-to-monitor-left",
    "move-column-to-monitor-right", "move-column-to-monitor-down", "move-column-to-monitor-up", "move-column-to-monitor-previous",
    "move-column-to-monitor-next", "reset-window-height", "switch-preset-column-width", "switch-preset-column-width-back",
    "switch-preset-window-width", "switch-preset-window-width-back", "switch-preset-window-height", "switch-preset-window-height-back",
    "maximize-column", "maximize-window-to-edges", "cycle-window-expansion", "expand-column-to-available-width", "show-hotkey-overlay",
    "move-workspace-to-monitor-left", "move-workspace-to-monitor-right", "move-workspace-to-monitor-down", "move-workspace-to-monitor-up",
    "move-workspace-to-monitor-previous", "move-workspace-to-monitor-next", "toggle-window-floating", "move-window-to-floating",
    "move-window-to-tiling", "focus-floating", "focus-tiling", "switch-focus-between-floating-and-tiling", "toggle-window-rule-opacity",
    "set-dynamic-cast-window", "clear-dynamic-cast-target", "toggle-overview", "open-overview", "close-overview"};

struct ActionSpec
{
    const char *name;
    int minArguments;
    int maxArguments;
    const char *properties;
};

constexpr ActionSpec kSpecialActions[] = {
    {"quit", 0, 0, "skip-confirmation"},
    {"spawn", 0, -1, ""},
    {"spawn-sh", 1, 1, ""},
    {"do-screen-transition", 0, 0, "delay-ms"},
    {"screenshot", 0, 0, "show-pointer"},
    {"screenshot-screen", 0, 0, "write-to-disk show-pointer"},
    {"screenshot-window", 0, 0, "write-to-disk show-pointer"},
    {"focus-window-in-column", 1, 1, ""},
    {"focus-column", 1, 1, ""},
    {"move-column-to-index", 1, 1, ""},
    {"set-column-display", 1, 1, ""},
    {"focus-workspace", 1, 1, ""},
    {"move-window-to-workspace-down", 0, 0, "focus"},
    {"move-window-to-workspace-up", 0, 0, "focus"},
    {"move-window-to-workspace", 1, 1, "focus"},
    {"move-column-to-workspace-down", 0, 0, "focus"},
    {"move-column-to-workspace-up", 0, 0, "focus"},
    {"move-column-to-workspace", 1, 1, "focus"},
    {"move-workspace-to-index", 1, 1, ""},
    {"move-workspace-to-monitor", 1, 1, ""},
    {"set-workspace-name", 1, 1, ""},
    {"focus-monitor", 1, 1, ""},
    {"move-window-to-monitor", 1, 1, ""},
    {"move-column-to-monitor", 1, 1, ""},
    {"set-window-width", 1, 1, ""},
    {"set-window-height", 1, 1, ""},
    {"set-column-width", 1, 1, ""},
    {"switch-layout", 1, 1, ""},
    {"set-dynamic-cast-monitor", 0, 1, ""},
};

struct ModifierSpec
{
    const char *name;
    BindModifier flag;
};

constexpr ModifierSpec kModifiers[] = {
    {"mod", BindModifier::Mod},
    {"ctrl", BindModifier::Ctrl},
    {"control", BindModifier::Ctrl},
    {"shift", BindModifier::Shift},
    {"alt", BindModifier::Alt},
    {"super", BindModifier::Super},
    {"win", BindModifier::Super},
    {"iso_level3_shift", BindModifier::IsoLevel3Shift},
    {"mod5", BindModifier::IsoLevel3Shift},
    {"iso_level5_shift", BindModifier::IsoLevel5Shift},
    {"mod3", BindModifier::IsoLevel5Shift},
};

struct TriggerSpec
{
    const char *name;
    BindTrigger trigger;
    int value;
};

constexpr TriggerSpec kTriggers[] = {
    {"mouseleft", BindTrigger::MouseButton, static_cast<int>(MouseButton::Left)},
    {"mouseright", BindTrigger::MouseButton, static_cast<int>(MouseButton::Right)},
    {"mousemiddle", BindTrigger::MouseButton, static_cast<int>(MouseButton::Middle)},
    {"mouseback", BindTrigger::MouseButton, static_cast<int>(MouseButton::Back)},
    {"mouseforward", BindTrigger::MouseButton, static_cast<int>(MouseButton::Forward)},
    {"wheelscrolldown", BindTrigger::Wheel, static_cast<int>(ScrollDirection::Down)},
    {"wheelscrollup", BindTrigger::Wheel, static_cast<int>(ScrollDirection::Up)},
    {"wheelscrollleft", BindTrigger::Wheel, static_cast<int>(ScrollDirection::Left)},
    {"wheelscrollright", BindTrigger::Wheel, static_cast<int>(ScrollDirection::Right)},
    {"touchpadscrolldown", BindTrigger::TouchpadScroll, static_cast<int>(ScrollDirection::Down)},
    {"touchpadscrollup", BindTrigger::TouchpadScroll, static_cast<int>(ScrollDirection::Up)},
    {"touchpadscrollleft", BindTrigger::TouchpadScroll, static_cast<int>(ScrollDirection::Left)},
    {"touchpadscrollright", BindTrigger::TouchpadScroll, static_cast<int>(ScrollDirection::Right)},
};

const QHash<QString, ActionSpec> &actionSpecs()
{
    static const QHash<QString, ActionSpec> specs = [] {
        QHash<QString, ActionSpec> result;
        for (const char *name : kSimpleActions) {
            result.insert(QString::fromLatin1(name), ActionSpec {name, 0, 0, ""});
        }
        for (const ActionSpec &spec : kSpecialActions) {
            result.insert(QString::fromLatin1(spec.name), spec);
        }
        return result;
    }();
    return specs;
}

BindModifier modifierFromName(const QString &name, const Kdl::Location &location)
{
    const QString lowered = name.toLower();
    for (const ModifierSpec &spec : kModifiers) {
        if (lowered == QLatin1String(spec.name)) {
            return spec.flag;
        }
    }
    failAt(location, QStringLiteral("invalid modifier: ") + name);
}

void applyTrigger(Bind &bind, const QString &key, const Kdl::Location &location)
{
    const QString lowered = key.toLower();
    for (const TriggerSpec &spec : kTriggers) {
        if (lowered != QLatin1String(spec.name)) {
            continue;
        }
        bind.trigger = spec.trigger;
        if (spec.trigger == BindTrigger::MouseButton) {
            bind.mouseButton = static_cast<MouseButton>(spec.value);
        } else {
            bind.scrollDirection = static_cast<ScrollDirection>(spec.value);
        }
        return;
    }
    const quint32 keysym = keysymFromName(key);
    if (keysym == XKB_KEY_NoSymbol) {
        failAt(location, QStringLiteral("invalid key: ") + key);
    }
    bind.trigger = BindTrigger::Key;
    bind.keysym = keysym;
    bind.key = qtKeyFromKeysym(keysym);
}

void parseBindKey(Bind &bind, const Kdl::Node &node)
{
    const QStringList parts = node.name.split(QLatin1Char('+'));
    for (qsizetype index = 0; index < parts.size() - 1; ++index) {
        bind.keyModifiers |= modifierFromName(parts.at(index).trimmed(), node.location);
    }
    bind.keyText = parts.last();
    applyTrigger(bind, bind.keyText, node.location);
}

QString bindSignature(const Bind &bind)
{
    const int detail = bind.trigger == BindTrigger::Key
        ? static_cast<int>(bind.keysym)
        : (bind.trigger == BindTrigger::MouseButton ? static_cast<int>(bind.mouseButton) : static_cast<int>(bind.scrollDirection));
    return QStringLiteral("%1/%2/%3").arg(static_cast<int>(bind.trigger)).arg(detail).arg(static_cast<int>(bind.keyModifiers.toInt()));
}

Action decodeAction(const Kdl::Node &node)
{
    const auto spec = actionSpecs().constFind(node.name);
    if (spec == actionSpecs().constEnd()) {
        fail(node, QStringLiteral("unknown action ") + quoteName(node.name));
    }
    expectNoChildren(node);
    Action action;
    action.name = node.name;
    for (const Kdl::Value &value : node.arguments) {
        action.arguments.append(toWritten(value));
    }
    if (action.arguments.size() < spec->minArguments) {
        fail(node, QStringLiteral("action ") + quoteName(node.name) + QStringLiteral(" requires an argument"));
    }
    if (spec->maxArguments >= 0) {
        expectArgumentLimit(node, spec->maxArguments);
    }
    const QStringList allowed = QString::fromLatin1(spec->properties).split(QLatin1Char(' '), Qt::SkipEmptyParts);
    for (const Kdl::Property &property : node.properties) {
        if (!allowed.contains(property.name)) {
            failAt(property.location, QStringLiteral("unexpected property ") + quoteName(property.name));
        }
        action.properties.append({property.name, toWritten(property.value)});
    }
    return action;
}

void decodeBindProperties(Bind &bind, const Kdl::Node &node, bool &lockedRequested)
{
    ValueTable table;
    table.insert(QStringLiteral("repeat"), [&bind](const Kdl::Value &value) { bind.repeat = toBoolean(value); });
    table.insert(QStringLiteral("cooldown-ms"),
        [&bind](const Kdl::Value &value) { bind.cooldownMs = static_cast<int>(toInteger(value, Range {0, 2147483647})); });
    table.insert(QStringLiteral("allow-when-locked"), [&bind, &lockedRequested](const Kdl::Value &value) {
        bind.allowWhenLocked = toBoolean(value);
        lockedRequested = true;
    });
    table.insert(QStringLiteral("allow-inhibiting"), [&bind](const Kdl::Value &value) { bind.allowInhibiting = toBoolean(value); });
    table.insert(QStringLiteral("hotkey-overlay-title"), [&bind](const Kdl::Value &value) {
        if (value.isNull()) {
            bind.hideFromHotkeyOverlay = true;
            return;
        }
        bind.hotkeyOverlayTitle = toText(value);
    });
    decodeProperties(node, table);
}

Bind decodeBind(const Kdl::Node &node)
{
    expectNoArguments(node);
    Bind bind;
    parseBindKey(bind, node);
    bool lockedRequested = false;
    decodeBindProperties(bind, node, lockedRequested);
    if (node.children.isEmpty()) {
        fail(node, QStringLiteral("expected an action for this keybind"));
    }
    if (node.children.size() > 1) {
        failAt(node.children.at(1).location, QStringLiteral("only one action is allowed per keybind"));
    }
    bind.action = decodeAction(node.children.first());
    const bool spawns = bind.action.name == QLatin1String("spawn") || bind.action.name == QLatin1String("spawn-sh");
    if (lockedRequested && !spawns) {
        fail(node, QStringLiteral("allow-when-locked can only be set on spawn binds"));
    }
    if (bind.action.name == QLatin1String("toggle-keyboard-shortcuts-inhibit")) {
        bind.allowInhibiting = false;
    }
    return bind;
}

}

void decodeBinds(LoadContext &context, const Kdl::Node &node)
{
    expectOnlyChildren(node);
    QList<Bind> binds;
    QStringList signatures;
    for (const Kdl::Node &child : node.children) {
        const Bind bind = decodeBind(child);
        const QString signature = bindSignature(bind);
        if (signatures.contains(signature)) {
            failAt(child.location, QStringLiteral("duplicate keybind ") + quoteName(child.name));
        }
        signatures.append(signature);
        binds.append(bind);
    }
    QList<Bind> &target = context.config.binds;
    target.removeIf([&signatures](const Bind &bind) { return signatures.contains(bindSignature(bind)); });
    target.append(binds);
}

void resolveBinds(Config &config)
{
    const QString modKey = config.input.modKey;
    BindModifier resolved = BindModifier::Super;
    for (const ModifierSpec &spec : kModifiers) {
        if (modKey.compare(QLatin1String(spec.name), Qt::CaseInsensitive) == 0) {
            resolved = spec.flag;
        }
    }
    for (Bind &bind : config.binds) {
        bind.resolvedModifiers = bind.keyModifiers;
        if (bind.resolvedModifiers.testFlag(BindModifier::Mod)) {
            bind.resolvedModifiers &= ~BindModifiers(BindModifier::Mod);
            bind.resolvedModifiers |= resolved;
        }
        bind.modifiers = qtModifiers(bind.resolvedModifiers);
    }
}

}
