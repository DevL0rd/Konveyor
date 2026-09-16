#include "config/color.h"
#include "config/loader.h"
#include "config/sections.h"

namespace Konveyor::Config
{

namespace
{

const QStringList &centerModes()
{
    static const QStringList modes {QStringLiteral("never"), QStringLiteral("always"), QStringLiteral("on-overflow")};
    return modes;
}

const QStringList &newColumnPositions()
{
    static const QStringList positions {QStringLiteral("left"), QStringLiteral("right")};
    return positions;
}

const QStringList &columnDisplays()
{
    static const QStringList displays {QStringLiteral("normal"), QStringLiteral("tabbed")};
    return displays;
}

PresetSize decodePresetSize(const Kdl::Node &node)
{
    if (node.name == QLatin1String("proportion")) {
        return Proportion {numberArgument(node, AnyNumber)};
    }
    if (node.name == QLatin1String("fixed")) {
        return Fixed {static_cast<double>(integerArgument(node, AnyNumber))};
    }
    fail(node, QStringLiteral("unexpected node ") + quoteName(node.name));
}

void addLayoutSizingHandlers(NodeTable &table, LayoutPart &part)
{
    table.insert(QStringLiteral("gaps"), [&part](const Kdl::Node &node) { part.gaps = numberArgument(node, Range {0, 65535}); });
    table.insert(QStringLiteral("struts"), [&part](const Kdl::Node &node) { part.struts = decodeStruts(node); });
    table.insert(
        QStringLiteral("preset-column-widths"), [&part](const Kdl::Node &node) { part.presetColumnWidths = decodePresetSizes(node); });
    table.insert(
        QStringLiteral("preset-window-heights"), [&part](const Kdl::Node &node) { part.presetWindowHeights = decodePresetSizes(node); });
    table.insert(QStringLiteral("default-column-width"),
        [&part](const Kdl::Node &node) { part.defaultColumnWidth = decodeDefaultPresetSize(node); });
    table.insert(QStringLiteral("center-focused-column"), [&part](const Kdl::Node &node) {
        part.centerFocusedColumn = static_cast<CenterFocusedColumn>(keywordArgument(node, centerModes()));
    });
    table.insert(QStringLiteral("new-column-position"), [&part](const Kdl::Node &node) {
        part.newColumnPosition = static_cast<NewColumnPosition>(keywordArgument(node, newColumnPositions()));
    });
    table.insert(QStringLiteral("always-center-single-column"),
        [&part](const Kdl::Node &node) { part.alwaysCenterSingleColumn = flagArgument(node); });
    table.insert(
        QStringLiteral("remember-window-sizes"), [&part](const Kdl::Node &node) { part.rememberWindowSizes = flagArgument(node); });
    table.insert(
        QStringLiteral("remember-window-positions"), [&part](const Kdl::Node &node) { part.rememberWindowPositions = flagArgument(node); });
    table.insert(QStringLiteral("empty-workspace-above-first"),
        [&part](const Kdl::Node &node) { part.emptyWorkspaceAboveFirst = flagArgument(node); });
    table.insert(QStringLiteral("default-column-display"), [&part](const Kdl::Node &node) {
        part.defaultColumnDisplay = static_cast<ColumnDisplay>(keywordArgument(node, columnDisplays()));
    });
}

void addLayoutAppearanceHandlers(NodeTable &table, LayoutPart &part)
{
    table.insert(QStringLiteral("focus-ring"), [&part](const Kdl::Node &node) { part.focusRing = decodeBorderRule(node); });
    table.insert(QStringLiteral("border"), [&part](const Kdl::Node &node) { part.border = decodeBorderRule(node); });
    table.insert(QStringLiteral("tab-indicator"), [&part](const Kdl::Node &node) { part.tabIndicator = decodeTabIndicatorPart(node); });
    table.insert(QStringLiteral("insert-hint"), [&part](const Kdl::Node &node) { part.insertHint = decodeInsertHintPart(node); });
    table.insert(QStringLiteral("background-color"), [&part](const Kdl::Node &node) { part.backgroundColor = decodeColorNode(node); });
}

}

LayoutPart decodeLayoutPart(const Kdl::Node &node, bool enableEmptyBorder)
{
    expectOnlyChildren(node);
    LayoutPart part;
    NodeTable table;
    addLayoutSizingHandlers(table, part);
    addLayoutAppearanceHandlers(table, part);
    decodeChildren(node, table);
    if (enableEmptyBorder && part.border && !part.border->enabled) {
        part.border->enabled = true;
    }
    return part;
}

Struts decodeStruts(const Kdl::Node &node)
{
    expectOnlyChildren(node);
    Struts struts;
    const Range range {-65535, 65535};
    NodeTable table;
    table.insert(QStringLiteral("left"), [&struts, range](const Kdl::Node &child) { struts.left = numberArgument(child, range); });
    table.insert(QStringLiteral("right"), [&struts, range](const Kdl::Node &child) { struts.right = numberArgument(child, range); });
    table.insert(QStringLiteral("top"), [&struts, range](const Kdl::Node &child) { struts.top = numberArgument(child, range); });
    table.insert(QStringLiteral("bottom"), [&struts, range](const Kdl::Node &child) { struts.bottom = numberArgument(child, range); });
    decodeChildren(node, table);
    return struts;
}

QList<PresetSize> decodePresetSizes(const Kdl::Node &node)
{
    expectOnlyChildren(node);
    QList<PresetSize> sizes;
    for (const Kdl::Node &child : node.children) {
        sizes.append(decodePresetSize(child));
    }
    return sizes;
}

std::optional<PresetSize> decodeDefaultPresetSize(const Kdl::Node &node)
{
    expectOnlyChildren(node);
    if (node.children.isEmpty()) {
        return std::nullopt;
    }
    if (node.children.size() > 1) {
        failAt(node.children.at(1).location, QStringLiteral("expected no more than one child"));
    }
    return decodePresetSize(node.children.first());
}

CornerRadius decodeCornerRadius(const Kdl::Node &node)
{
    expectLeafNode(node);
    const Range range {0, 65535};
    const double first = toNumber(requiredArgument(node, node.name), range);
    if (node.arguments.size() == 1) {
        return CornerRadius {first, first, first, first};
    }
    if (node.arguments.size() != 4) {
        fail(node, QStringLiteral("either 1 or 4 arguments are required"));
    }
    return CornerRadius {
        first, toNumber(node.arguments.at(1), range), toNumber(node.arguments.at(2), range), toNumber(node.arguments.at(3), range)};
}

HotCorners decodeHotCorners(const Kdl::Node &node)
{
    expectOnlyChildren(node);
    bool off = false;
    HotCorners corners;
    corners.topLeft = false;
    NodeTable table;
    table.insert(QStringLiteral("off"), [&off](const Kdl::Node &child) { off = flagArgument(child); });
    table.insert(QStringLiteral("top-left"), [&corners](const Kdl::Node &child) { corners.topLeft = flagArgument(child); });
    table.insert(QStringLiteral("top-right"), [&corners](const Kdl::Node &child) { corners.topRight = flagArgument(child); });
    table.insert(QStringLiteral("bottom-left"), [&corners](const Kdl::Node &child) { corners.bottomLeft = flagArgument(child); });
    table.insert(QStringLiteral("bottom-right"), [&corners](const Kdl::Node &child) { corners.bottomRight = flagArgument(child); });
    decodeChildren(node, table);
    corners.enabled = !off;
    corners.topLeft = corners.topLeft || !(corners.topRight || corners.bottomLeft || corners.bottomRight);
    return corners;
}

}
