#include "config/loader.h"
#include "config/sections.h"
#include "config/sizechange.h"

#include <xkbcommon/xkbcommon.h>

namespace Konveyor::Config
{

namespace
{

constexpr const char *kSimpleActions[] = {"close-window", "fullscreen-window", "toggle-windowed-fullscreen", "focus-window-previous",
    "focus-column-left", "focus-column-right", "focus-column-first", "focus-column-last", "focus-column-right-or-first",
    "focus-column-left-or-last", "focus-window-or-monitor-up", "focus-window-or-monitor-down", "focus-column-or-monitor-left",
    "focus-column-or-monitor-right", "focus-window-down", "focus-window-up", "focus-window-down-or-column-left",
    "focus-window-down-or-column-right", "focus-window-up-or-column-left", "focus-window-up-or-column-right",
    "focus-window-or-workspace-down", "focus-window-or-workspace-up", "focus-window-top", "focus-window-bottom", "focus-window-down-or-top",
    "focus-window-up-or-bottom", "move-column-left", "move-column-right", "move-column-to-first", "move-column-to-last",
    "move-column-left-or-to-monitor-left", "move-column-right-or-to-monitor-right", "move-window-down", "move-window-up",
    "move-window-down-or-to-workspace-down", "move-window-up-or-to-workspace-up", "consume-or-expel-window-left",
    "consume-or-expel-window-right", "consume-window-into-column", "expel-window-from-column", "swap-window-left", "swap-window-right",
    "toggle-column-tabbed-display", "center-column", "center-window", "center-visible-columns", "focus-workspace-down",
    "focus-workspace-up", "focus-workspace-previous", "move-workspace-down", "move-workspace-up", "unset-workspace-name",
    "focus-monitor-left", "focus-monitor-right", "focus-monitor-down", "focus-monitor-up", "focus-monitor-previous", "focus-monitor-next",
    "move-window-to-monitor-left", "move-window-to-monitor-right", "move-window-to-monitor-down", "move-window-to-monitor-up",
    "move-window-to-monitor-previous", "move-window-to-monitor-next", "move-column-to-monitor-left", "move-column-to-monitor-right",
    "move-column-to-monitor-down", "move-column-to-monitor-up", "move-column-to-monitor-previous", "move-column-to-monitor-next",
    "reset-window-height", "switch-preset-column-width", "switch-preset-column-width-back", "switch-preset-window-width",
    "switch-preset-window-width-back", "switch-preset-window-height", "switch-preset-window-height-back", "maximize-column",
    "maximize-window-to-edges", "cycle-window-expansion", "expand-column-to-available-width", "show-hotkey-overlay",
    "move-workspace-to-monitor-left", "move-workspace-to-monitor-right", "move-workspace-to-monitor-down", "move-workspace-to-monitor-up",
    "move-workspace-to-monitor-previous", "move-workspace-to-monitor-next", "toggle-window-floating", "move-window-to-floating",
    "move-window-to-tiling", "focus-floating", "focus-tiling", "switch-focus-between-floating-and-tiling", "toggle-window-rule-opacity",
    "toggle-overview", "open-overview", "close-overview"};

enum class ArgumentKind
{
    None,
    Text,
    Index,
    Workspace,
    Size,
    Display
};

struct ActionSpec
{
    const char *name;
    int minArguments;
    int maxArguments;
    ArgumentKind argument;
    const char *properties;
};

constexpr ActionSpec kSpecialActions[] = {
    {"spawn", 1, -1, ArgumentKind::Text, ""},
    {"spawn-sh", 1, 1, ArgumentKind::Text, ""},
    {"focus-window-in-column", 1, 1, ArgumentKind::Index, ""},
    {"focus-column", 1, 1, ArgumentKind::Index, ""},
    {"focus-taskbar-item", 1, 1, ArgumentKind::Index, ""},
    {"move-column-to-index", 1, 1, ArgumentKind::Index, ""},
    {"set-column-display", 1, 1, ArgumentKind::Display, ""},
    {"focus-workspace", 1, 1, ArgumentKind::Workspace, ""},
    {"move-window-to-workspace-down", 0, 0, ArgumentKind::None, "focus"},
    {"move-window-to-workspace-up", 0, 0, ArgumentKind::None, "focus"},
    {"move-window-to-workspace", 1, 1, ArgumentKind::Workspace, "focus"},
    {"move-column-to-workspace-down", 0, 0, ArgumentKind::None, "focus"},
    {"move-column-to-workspace-up", 0, 0, ArgumentKind::None, "focus"},
    {"move-column-to-workspace", 1, 1, ArgumentKind::Workspace, "focus"},
    {"move-workspace-to-index", 1, 1, ArgumentKind::Index, ""},
    {"move-workspace-to-monitor", 1, 1, ArgumentKind::Text, ""},
    {"set-workspace-name", 1, 1, ArgumentKind::Text, ""},
    {"focus-monitor", 1, 1, ArgumentKind::Text, ""},
    {"move-window-to-monitor", 1, 1, ArgumentKind::Text, ""},
    {"move-column-to-monitor", 1, 1, ArgumentKind::Text, ""},
    {"set-window-width", 1, 1, ArgumentKind::Size, ""},
    {"set-window-height", 1, 1, ArgumentKind::Size, ""},
    {"set-column-width", 1, 1, ArgumentKind::Size, ""},
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
            result.insert(QString::fromLatin1(name), ActionSpec {name, 0, 0, ArgumentKind::None, ""});
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

QString bindSignature(const Bind &bind, BindModifiers modifiers)
{
    const int detail = bind.trigger == BindTrigger::Key
        ? static_cast<int>(bind.keysym)
        : (bind.trigger == BindTrigger::MouseButton ? static_cast<int>(bind.mouseButton) : static_cast<int>(bind.scrollDirection));
    return QStringLiteral("%1/%2/%3").arg(static_cast<int>(bind.trigger)).arg(detail).arg(static_cast<int>(modifiers.toInt()));
}

void checkWorkspaceArgument(const Kdl::Value &value)
{
    if (value.isInteger()) {
        toInteger(value, Range {0, 255});
        return;
    }
    if (!value.isString() || value.toString().isEmpty()) {
        failAt(value.location, QStringLiteral("expected a workspace index or name"));
    }
}

void checkSizeArgument(const Kdl::Value &value)
{
    if (value.isInteger()) {
        return;
    }
    if (!value.isString()) {
        failAt(value.location, QStringLiteral("expected a size like \"+10%\", \"50%\" or \"800\""));
    }
    const auto change = parseSizeChange(value.toString(), true);
    if (!change) {
        failAt(value.location, change.error());
    }
}

void checkArgument(ArgumentKind kind, const Kdl::Value &value)
{
    switch (kind) {
    case ArgumentKind::None:
        return;
    case ArgumentKind::Text:
        toText(value);
        return;
    case ArgumentKind::Index:
        toInteger(value, Range {1, 2147483647});
        return;
    case ArgumentKind::Workspace:
        checkWorkspaceArgument(value);
        return;
    case ArgumentKind::Size:
        checkSizeArgument(value);
        return;
    case ArgumentKind::Display:
        toKeyword(value, {QStringLiteral("normal"), QStringLiteral("tabbed")});
        return;
    }
}

void checkArguments(const Kdl::Node &node, const ActionSpec &spec)
{
    for (const Kdl::Value &value : node.arguments) {
        checkArgument(spec.argument, value);
    }
    if (spec.argument == ArgumentKind::Text && !node.arguments.isEmpty() && node.arguments.first().toString().isEmpty()) {
        failAt(node.arguments.first().location, QStringLiteral("expected a non-empty string"));
    }
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
    checkArguments(node, *spec);
    const QStringList allowed = QString::fromLatin1(spec->properties).split(QLatin1Char(' '), Qt::SkipEmptyParts);
    for (const Kdl::Property &property : node.properties) {
        if (!allowed.contains(property.name)) {
            failAt(property.location, QStringLiteral("unexpected property ") + quoteName(property.name));
        }
        toBoolean(property.value);
        action.properties.append({property.name, toWritten(property.value)});
    }
    return action;
}

void decodeBindProperties(Bind &bind, const Kdl::Node &node)
{
    ValueTable table;
    table.insert(QStringLiteral("repeat"), [&bind](const Kdl::Value &value) { bind.repeat = toBoolean(value); });
    table.insert(QStringLiteral("cooldown-ms"),
        [&bind](const Kdl::Value &value) { bind.cooldownMs = static_cast<int>(toInteger(value, Range {0, 2147483647})); });
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
    decodeBindProperties(bind, node);
    if (node.children.isEmpty()) {
        fail(node, QStringLiteral("expected an action for this keybind"));
    }
    if (node.children.size() > 1) {
        failAt(node.children.at(1).location, QStringLiteral("only one action is allowed per keybind"));
    }
    bind.action = decodeAction(node.children.first());
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
        const QString signature = bindSignature(bind, bind.keyModifiers);
        if (signatures.contains(signature)) {
            failAt(child.location, QStringLiteral("duplicate keybind ") + quoteName(child.name));
        }
        signatures.append(signature);
        context.bindNodes.insert(signature, {child.name, child.location});
        binds.append(bind);
    }
    QList<Bind> &target = context.config.binds;
    target.removeIf([&signatures](const Bind &bind) { return signatures.contains(bindSignature(bind, bind.keyModifiers)); });
    target.append(binds);
}

void resolveBinds(LoadContext &context)
{
    Config &config = context.config;
    const QString modKey = config.input.modKey;
    BindModifier resolved = BindModifier::Super;
    for (const ModifierSpec &spec : kModifiers) {
        if (modKey.compare(QLatin1String(spec.name), Qt::CaseInsensitive) == 0) {
            resolved = spec.flag;
        }
    }
    QHash<QString, QString> seen;
    for (Bind &bind : config.binds) {
        bind.resolvedModifiers = bind.keyModifiers;
        if (bind.resolvedModifiers.testFlag(BindModifier::Mod)) {
            bind.resolvedModifiers &= ~BindModifiers(BindModifier::Mod);
            bind.resolvedModifiers |= resolved;
        }
        bind.modifiers = qtModifiers(bind.resolvedModifiers);
        const auto [name, location] = context.bindNodes.value(bindSignature(bind, bind.keyModifiers));
        const QString signature = bindSignature(bind, bind.resolvedModifiers);
        if (seen.contains(signature)) {
            failAt(location,
                QStringLiteral("keybind ") + quoteName(name) + QStringLiteral(" is the same as ") + quoteName(seen.value(signature))
                    + QStringLiteral(" while the Mod key is ") + modKey);
        }
        seen.insert(signature, name);
    }
}

}
