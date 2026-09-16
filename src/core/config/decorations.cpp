#include "config/color.h"
#include "config/loader.h"
#include "config/sections.h"

namespace Konveyor::Config
{

namespace
{

const QStringList &tabPositions()
{
    static const QStringList positions {QStringLiteral("left"), QStringLiteral("right"), QStringLiteral("top"), QStringLiteral("bottom")};
    return positions;
}

void addToggleHandlers(NodeTable &table, bool &on, bool &off)
{
    table.insert(QStringLiteral("on"), [&on](const Kdl::Node &node) { on = flagArgument(node); });
    table.insert(QStringLiteral("off"), [&off](const Kdl::Node &node) { off = flagArgument(node); });
}

void addPaintHandlers(NodeTable &table, const QString &colorName, const QString &gradientName, std::optional<Paint> &paint)
{
    table.insert(colorName, [&paint](const Kdl::Node &node) { applyPaintColor(paint, decodePaintNode(node)); });
    table.insert(gradientName, [&paint](const Kdl::Node &node) { applyPaintGradient(paint, decodeGradientNode(node)); });
}

void addStateHandlers(NodeTable &table, const QString &state, std::optional<Paint> &paint)
{
    addPaintHandlers(table, state + QStringLiteral("-color"), state + QStringLiteral("-gradient"), paint);
}

void addTripletHandlers(NodeTable &table, std::optional<Paint> &active, std::optional<Paint> &inactive, std::optional<Paint> &urgent)
{
    addStateHandlers(table, QStringLiteral("active"), active);
    addStateHandlers(table, QStringLiteral("inactive"), inactive);
    addStateHandlers(table, QStringLiteral("urgent"), urgent);
}

}

BorderRule decodeBorderRule(const Kdl::Node &node)
{
    expectOnlyChildren(node);
    BorderRule rule;
    bool on = false;
    bool off = false;
    NodeTable table;
    addToggleHandlers(table, on, off);
    addTripletHandlers(table, rule.active, rule.inactive, rule.urgent);
    table.insert(QStringLiteral("width"), [&rule](const Kdl::Node &child) { rule.width = numberArgument(child, Range {0, 65535}); });
    decodeChildren(node, table);
    if (on) {
        rule.enabled = true;
    } else if (off) {
        rule.enabled = false;
    }
    return rule;
}

ShadowRule decodeShadowRule(const Kdl::Node &node)
{
    expectOnlyChildren(node);
    ShadowRule rule;
    bool on = false;
    bool off = false;
    NodeTable table;
    addToggleHandlers(table, on, off);
    table.insert(QStringLiteral("offset"), [&rule](const Kdl::Node &child) {
        QPointF offset;
        ValueTable properties;
        properties.insert(QStringLiteral("x"), [&offset](const Kdl::Value &value) { offset.setX(toNumber(value, Range {-65535, 65535})); });
        properties.insert(QStringLiteral("y"), [&offset](const Kdl::Value &value) { offset.setY(toNumber(value, Range {-65535, 65535})); });
        expectNoArguments(child);
        expectNoChildren(child);
        decodeProperties(child, properties);
        rule.offset = offset;
    });
    table.insert(QStringLiteral("softness"), [&rule](const Kdl::Node &child) { rule.softness = numberArgument(child, Range {0, 1024}); });
    table.insert(QStringLiteral("spread"), [&rule](const Kdl::Node &child) { rule.spread = numberArgument(child, Range {-1024, 1024}); });
    table.insert(QStringLiteral("draw-behind-window"), [&rule](const Kdl::Node &child) { rule.drawBehindWindow = booleanArgument(child); });
    table.insert(QStringLiteral("color"), [&rule](const Kdl::Node &child) { rule.color = decodeColorNode(child); });
    table.insert(QStringLiteral("inactive-color"), [&rule](const Kdl::Node &child) { rule.inactiveColor = decodeColorNode(child); });
    decodeChildren(node, table);
    if (off) {
        rule.enabled = false;
    } else if (on) {
        rule.enabled = true;
    }
    return rule;
}

TabIndicatorRule decodeTabIndicatorRule(const Kdl::Node &node)
{
    expectOnlyChildren(node);
    TabIndicatorRule rule;
    NodeTable table;
    addTripletHandlers(table, rule.active, rule.inactive, rule.urgent);
    decodeChildren(node, table);
    return rule;
}

TabIndicatorPart decodeTabIndicatorPart(const Kdl::Node &node)
{
    expectOnlyChildren(node);
    TabIndicatorPart part;
    bool on = false;
    bool off = false;
    NodeTable table;
    addToggleHandlers(table, on, off);
    addTripletHandlers(table, part.colors.active, part.colors.inactive, part.colors.urgent);
    table.insert(QStringLiteral("hide-when-single-tab"), [&part](const Kdl::Node &child) { part.hideWhenSingleTab = flagArgument(child); });
    table.insert(QStringLiteral("place-within-column"), [&part](const Kdl::Node &child) { part.placeWithinColumn = flagArgument(child); });
    table.insert(QStringLiteral("gap"), [&part](const Kdl::Node &child) { part.gap = numberArgument(child, Range {-65535, 65535}); });
    table.insert(QStringLiteral("width"), [&part](const Kdl::Node &child) { part.width = numberArgument(child, Range {0, 65535}); });
    table.insert(QStringLiteral("gaps-between-tabs"),
        [&part](const Kdl::Node &child) { part.gapsBetweenTabs = numberArgument(child, Range {0, 65535}); });
    table.insert(
        QStringLiteral("corner-radius"), [&part](const Kdl::Node &child) { part.cornerRadius = numberArgument(child, Range {0, 65535}); });
    table.insert(QStringLiteral("position"),
        [&part](const Kdl::Node &child) { part.position = static_cast<TabIndicatorPosition>(keywordArgument(child, tabPositions())); });
    table.insert(QStringLiteral("length"), [&part](const Kdl::Node &child) {
        expectNoArguments(child);
        expectNoChildren(child);
        ValueTable properties;
        properties.insert(QStringLiteral("total-proportion"),
            [&part](const Kdl::Value &value) { part.lengthTotalProportion = toNumber(value, AnyNumber); });
        decodeProperties(child, properties);
    });
    decodeChildren(node, table);
    if (on) {
        part.enabled = true;
    } else if (off) {
        part.enabled = false;
    }
    return part;
}

InsertHintPart decodeInsertHintPart(const Kdl::Node &node)
{
    expectOnlyChildren(node);
    InsertHintPart part;
    bool on = false;
    bool off = false;
    NodeTable table;
    addToggleHandlers(table, on, off);
    addPaintHandlers(table, QStringLiteral("color"), QStringLiteral("gradient"), part.paint);
    decodeChildren(node, table);
    if (on) {
        part.enabled = true;
    } else if (off) {
        part.enabled = false;
    }
    return part;
}

}
