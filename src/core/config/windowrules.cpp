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
    table.insert(QStringLiteral("group-app-windows"), [&rule](const Kdl::Node &node) {
        rule.groupAppWindows = static_cast<GroupAppWindows>(
            keywordArgument(node, {QStringLiteral("off"), QStringLiteral("beside"), QStringLiteral("stack")}));
    });
    table.insert(QStringLiteral("max-rows-per-column"),
        [&rule](const Kdl::Node &node) { rule.maxRowsPerColumn = static_cast<int>(integerArgument(node, Range {1, 64})); });
    table.insert(QStringLiteral("new-window-placement"), [&rule](const Kdl::Node &node) {
        rule.newWindowPlacement
            = static_cast<NewWindowPlacement>(keywordArgument(node, {QStringLiteral("column"), QStringLiteral("stack")}));
    });
    boolOf(QStringLiteral("float-child-windows"), rule.floatChildWindows);
    boolOf(QStringLiteral("force-resizable"), rule.forceResizable);
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
    boolOf(QStringLiteral("clip-to-geometry"), rule.clipToGeometry);
    table.insert(QStringLiteral("opacity"), [&rule](const Kdl::Node &node) { rule.opacity = numberArgument(node, AnyNumber); });
    table.insert(
        QStringLiteral("geometry-corner-radius"), [&rule](const Kdl::Node &node) { rule.geometryCornerRadius = decodeCornerRadius(node); });
}

void addAppearanceHandlers(NodeTable &table, WindowRule &rule)
{
    table.insert(QStringLiteral("focus-ring"), [&rule](const Kdl::Node &node) { rule.focusRing = decodeBorderRule(node); });
    table.insert(QStringLiteral("border"), [&rule](const Kdl::Node &node) { rule.border = decodeBorderRule(node); });
}

}

WindowRule decodeWindowRule(const Kdl::Node &node)
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
    decodeChildren(node, table, {QStringLiteral("match"), QStringLiteral("exclude")});
    return rule;
}

}
