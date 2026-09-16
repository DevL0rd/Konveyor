#include "config/sections.h"

#include <QRegularExpression>

namespace Konveyor::Config
{

namespace
{

const QStringList &floatingAnchors()
{
    static const QStringList names {QStringLiteral("top-left"), QStringLiteral("top-right"), QStringLiteral("bottom-left"),
        QStringLiteral("bottom-right"), QStringLiteral("top"), QStringLiteral("bottom"), QStringLiteral("left"), QStringLiteral("right")};
    return names;
}

QRegularExpression compileRegex(const Kdl::Value &value)
{
    const QRegularExpression regex(toText(value));
    if (!regex.isValid()) {
        failAt(value.location, QStringLiteral("invalid regex: ") + regex.errorString());
    }
    return regex;
}

void addMatchBooleans(ValueTable &table, Match &match)
{
    const auto flagOf = [&table](const QString &name, std::optional<bool> &target) {
        table.insert(name, [&target](const Kdl::Value &value) { target = toBoolean(value); });
    };
    flagOf(QStringLiteral("is-active"), match.isActive);
    flagOf(QStringLiteral("is-focused"), match.isFocused);
    flagOf(QStringLiteral("is-active-in-column"), match.isActiveInColumn);
    flagOf(QStringLiteral("is-floating"), match.isFloating);
    flagOf(QStringLiteral("is-urgent"), match.isUrgent);
    flagOf(QStringLiteral("at-startup"), match.atStartup);
    flagOf(QStringLiteral("is-window-cast-target"), match.isWindowCastTarget);
}

Match decodeMatch(const Kdl::Node &node)
{
    expectNoArguments(node);
    expectNoChildren(node);
    Match match;
    ValueTable table;
    table.insert(QStringLiteral("app-id"), [&match](const Kdl::Value &value) { match.appId = compileRegex(value); });
    table.insert(QStringLiteral("title"), [&match](const Kdl::Value &value) { match.title = compileRegex(value); });
    table.insert(QStringLiteral("monitor-profile"), [&match](const Kdl::Value &value) { match.monitorProfile = compileRegex(value); });
    addMatchBooleans(table, match);
    decodeProperties(node, table);
    return match;
}

FloatingPosition decodeFloatingPosition(const Kdl::Node &node)
{
    expectNoArguments(node);
    expectNoChildren(node);
    FloatingPosition position;
    bool hasX = false;
    bool hasY = false;
    ValueTable table;
    table.insert(QStringLiteral("x"), [&](const Kdl::Value &value) {
        position.x = toNumber(value, Range {-65535, 65535});
        hasX = true;
    });
    table.insert(QStringLiteral("y"), [&](const Kdl::Value &value) {
        position.y = toNumber(value, Range {-65535, 65535});
        hasY = true;
    });
    table.insert(QStringLiteral("relative-to"), [&position](const Kdl::Value &value) {
        position.relativeTo = static_cast<FloatingRelativeTo>(toKeyword(value, floatingAnchors()));
    });
    decodeProperties(node, table);
    if (!hasX) {
        fail(node, QStringLiteral("property `x` is required"));
    }
    if (!hasY) {
        fail(node, QStringLiteral("property `y` is required"));
    }
    return position;
}

void addOpenHandlers(NodeTable &table, WindowRule &rule)
{
    const auto boolOf = [&table](const QString &name, std::optional<bool> &target) {
        table.insert(name, [&target](const Kdl::Node &node) { target = booleanArgument(node); });
    };
    const auto textOf = [&table](const QString &name, std::optional<QString> &target) {
        table.insert(name, [&target](const Kdl::Node &node) { target = stringArgument(node); });
    };
    boolOf(QStringLiteral("open-maximized"), rule.openMaximized);
    boolOf(QStringLiteral("open-maximized-to-edges"), rule.openMaximizedToEdges);
    boolOf(QStringLiteral("open-fullscreen"), rule.openFullscreen);
    boolOf(QStringLiteral("open-floating"), rule.openFloating);
    boolOf(QStringLiteral("manage"), rule.manage);
    table.insert(QStringLiteral("column-position"), [&rule](const Kdl::Node &node) {
        static const QList<std::pair<QString, ColumnPosition>> positions {
            {QStringLiteral("start"), ColumnPosition::Start},
            {QStringLiteral("end"), ColumnPosition::End},
        };
        const Kdl::Value value = requiredArgument(node, QStringLiteral("position"));
        const QString text = toText(value);
        for (const auto &[name, position] : positions) {
            if (text == name) {
                rule.columnPosition = position;
                return;
            }
        }
        failAt(value.location, QStringLiteral("column-position must be \"start\" or \"end\""));
    });
    boolOf(QStringLiteral("open-focused"), rule.openFocused);
    textOf(QStringLiteral("open-on-output"), rule.openOnOutput);
    textOf(QStringLiteral("open-on-workspace"), rule.openOnWorkspace);
    table.insert(QStringLiteral("default-column-width"),
        [&rule](const Kdl::Node &node) { rule.defaultColumnWidth = decodeDefaultPresetSize(node); });
    table.insert(QStringLiteral("default-window-height"),
        [&rule](const Kdl::Node &node) { rule.defaultWindowHeight = decodeDefaultPresetSize(node); });
    table.insert(QStringLiteral("default-column-display"), [&rule](const Kdl::Node &node) {
        rule.defaultColumnDisplay = static_cast<ColumnDisplay>(keywordArgument(node, {QStringLiteral("normal"), QStringLiteral("tabbed")}));
    });
    table.insert(QStringLiteral("default-floating-position"),
        [&rule](const Kdl::Node &node) { rule.defaultFloatingPosition = decodeFloatingPosition(node); });
    table.insert(QStringLiteral("on-xdg-activate"), [&rule](const Kdl::Node &node) {
        rule.onXdgActivate = static_cast<XdgActivate>(
            keywordArgument(node, {QStringLiteral("ignore"), QStringLiteral("set-urgent"), QStringLiteral("focus")}));
    });
}

void addSizeHandlers(NodeTable &table, WindowRule &rule)
{
    const auto sizeOf = [&table](const QString &name, std::optional<int> &target) {
        table.insert(name, [&target](const Kdl::Node &node) { target = static_cast<int>(integerArgument(node, Range {0, 65535})); });
    };
    sizeOf(QStringLiteral("min-width"), rule.minWidth);
    sizeOf(QStringLiteral("min-height"), rule.minHeight);
    sizeOf(QStringLiteral("max-width"), rule.maxWidth);
    sizeOf(QStringLiteral("max-height"), rule.maxHeight);
}

void addDynamicHandlers(NodeTable &table, WindowRule &rule)
{
    const auto boolOf = [&table](const QString &name, std::optional<bool> &target) {
        table.insert(name, [&target](const Kdl::Node &node) { target = booleanArgument(node); });
    };
    boolOf(QStringLiteral("draw-border-with-background"), rule.drawBorderWithBackground);
    boolOf(QStringLiteral("clip-to-geometry"), rule.clipToGeometry);
    boolOf(QStringLiteral("baba-is-float"), rule.babaIsFloat);
    boolOf(QStringLiteral("variable-refresh-rate"), rule.variableRefreshRate);
    boolOf(QStringLiteral("tiled-state"), rule.tiledState);
    table.insert(QStringLiteral("opacity"), [&rule](const Kdl::Node &node) { rule.opacity = numberArgument(node, AnyNumber); });
    table.insert(
        QStringLiteral("scroll-factor"), [&rule](const Kdl::Node &node) { rule.scrollFactor = numberArgument(node, Range {0, 100}); });
    table.insert(
        QStringLiteral("geometry-corner-radius"), [&rule](const Kdl::Node &node) { rule.geometryCornerRadius = decodeCornerRadius(node); });
    table.insert(QStringLiteral("block-out-from"), [&rule](const Kdl::Node &node) {
        rule.blockOutFrom
            = static_cast<BlockOutFrom>(keywordArgument(node, {QStringLiteral("screencast"), QStringLiteral("screen-capture")}));
        rule.blockOutFromScreencast = true;
    });
}

void addAppearanceHandlers(NodeTable &table, WindowRule &rule)
{
    table.insert(QStringLiteral("focus-ring"), [&rule](const Kdl::Node &node) { rule.focusRing = decodeBorderRule(node); });
    table.insert(QStringLiteral("border"), [&rule](const Kdl::Node &node) { rule.border = decodeBorderRule(node); });
    table.insert(QStringLiteral("tab-indicator"), [&rule](const Kdl::Node &node) { rule.tabIndicator = decodeTabIndicatorRule(node); });
    table.insert(QStringLiteral("shadow"), [&rule](const Kdl::Node &node) {
        rule.shadowRule = decodeShadowRule(node);
        rule.shadow = rule.shadowRule.enabled;
    });
}

void addIgnoredRuleHandlers(NodeTable &table, LoadContext &context)
{
    static const QStringList names {QStringLiteral("background-effect"), QStringLiteral("popups")};
    for (const QString &name : names) {
        table.insert(name,
            [&context](const Kdl::Node &node) { ignoreNode(context, node, QStringLiteral("this effect has no equivalent in KWin")); });
    }
}

}

WindowRule decodeWindowRule(LoadContext &context, const Kdl::Node &node)
{
    expectOnlyChildren(node);
    WindowRule rule;
    NodeTable table;
    table.insert(QStringLiteral("match"), [&rule](const Kdl::Node &child) { rule.matches.append(decodeMatch(child)); });
    table.insert(QStringLiteral("exclude"), [&rule](const Kdl::Node &child) { rule.excludes.append(decodeMatch(child)); });
    addOpenHandlers(table, rule);
    addSizeHandlers(table, rule);
    addDynamicHandlers(table, rule);
    addAppearanceHandlers(table, rule);
    addIgnoredRuleHandlers(table, context);
    decodeChildren(node, table, {QStringLiteral("match"), QStringLiteral("exclude")});
    return rule;
}

}
