#include "config/color.h"
#include "config/sections.h"

namespace Konveyor::Config
{

namespace
{

const QStringList &modKeyNames()
{
    static const QStringList names {QStringLiteral("ctrl"), QStringLiteral("control"), QStringLiteral("shift"), QStringLiteral("alt"),
        QStringLiteral("super"), QStringLiteral("win"), QStringLiteral("iso_level3_shift"), QStringLiteral("mod5"),
        QStringLiteral("iso_level5_shift"), QStringLiteral("mod3")};
    return names;
}

const QStringList &canonicalModKeys()
{
    static const QStringList names {QStringLiteral("Ctrl"), QStringLiteral("Ctrl"), QStringLiteral("Shift"), QStringLiteral("Alt"),
        QStringLiteral("Super"), QStringLiteral("Super"), QStringLiteral("ISO_Level3_Shift"), QStringLiteral("ISO_Level3_Shift"),
        QStringLiteral("ISO_Level5_Shift"), QStringLiteral("ISO_Level5_Shift")};
    return names;
}

QString decodeModKey(const Kdl::Node &node)
{
    const QString text = stringArgument(node);
    const qsizetype index = modKeyNames().indexOf(text.toLower());
    if (index < 0) {
        fail(node, QStringLiteral("invalid Mod key: ") + text);
    }
    return canonicalModKeys().at(index);
}

void decodeEdgeScroll(const Kdl::Node &node, const QString &triggerName, DndEdgeScroll &scroll)
{
    expectOnlyChildren(node);
    NodeTable table;
    table.insert(triggerName, [&scroll](const Kdl::Node &child) { scroll.triggerSize = numberArgument(child, Range {0, 65535}); });
    table.insert(QStringLiteral("delay-ms"),
        [&scroll](const Kdl::Node &child) { scroll.delayMs = static_cast<double>(integerArgument(child, Range {0, 65535})); });
    table.insert(
        QStringLiteral("max-speed"), [&scroll](const Kdl::Node &child) { scroll.maxSpeed = numberArgument(child, Range {0, 1000000}); });
    decodeChildren(node, table);
}

void addIgnoredInputHandlers(NodeTable &table, LoadContext &context)
{
    static const QStringList names {QStringLiteral("keyboard"), QStringLiteral("touchpad"), QStringLiteral("mouse"),
        QStringLiteral("trackpoint"), QStringLiteral("trackball"), QStringLiteral("tablet"), QStringLiteral("touch"),
        QStringLiteral("disable-power-key-handling")};
    for (const QString &name : names) {
        table.insert(name, [&context](const Kdl::Node &child) { ignoreNode(context, child, QStringLiteral("KDE manages input devices")); });
    }
}

void decodeFocusFollowsMouse(const Kdl::Node &node, Input &input)
{
    expectNoArguments(node);
    expectNoChildren(node);
    input.focusFollowsMouse = true;
    ValueTable table;
    table.insert(QStringLiteral("max-scroll-amount"), [&input](const Kdl::Value &value) {
        const QString text = toText(value);
        if (!text.endsWith(QLatin1Char('%'))) {
            failAt(value.location, QStringLiteral("value must end with '%'"));
        }
        bool ok = false;
        const double percent = QStringView {text}.chopped(1).toDouble(&ok);
        if (!ok) {
            failAt(value.location, QStringLiteral("error parsing value"));
        }
        input.focusFollowsMouseMaxScrollPercent = percent / 100.0;
    });
    decodeProperties(node, table);
}

void decodeWarpMouse(const Kdl::Node &node, Input &input)
{
    expectNoArguments(node);
    expectNoChildren(node);
    input.warpMouseToFocus = true;
    ValueTable table;
    table.insert(QStringLiteral("mode"), [&input](const Kdl::Value &value) {
        const int index = toKeyword(value, {QStringLiteral("center-xy"), QStringLiteral("center-xy-always")});
        input.warpMouseMode = index == 0 ? WarpMouseMode::CenterXY : WarpMouseMode::CenterXYAlways;
    });
    decodeProperties(node, table);
}

}

void decodeGestures(LoadContext &context, const Kdl::Node &node)
{
    expectOnlyChildren(node);
    Gestures &gestures = context.config.gestures;
    NodeTable table;
    table.insert(QStringLiteral("dnd-edge-view-scroll"),
        [&gestures](const Kdl::Node &child) { decodeEdgeScroll(child, QStringLiteral("trigger-width"), gestures.dndEdgeViewScroll); });
    table.insert(QStringLiteral("dnd-edge-workspace-switch"), [&gestures](const Kdl::Node &child) {
        decodeEdgeScroll(child, QStringLiteral("trigger-height"), gestures.dndEdgeWorkspaceSwitch);
    });
    table.insert(QStringLiteral("hot-corners"), [&gestures](const Kdl::Node &child) { gestures.hotCorners = decodeHotCorners(child); });
    table.insert(QStringLiteral("titlebar-drag"), [&gestures](const Kdl::Node &child) {
        const int index = keywordArgument(child, {QStringLiteral("scroll-view"), QStringLiteral("move-window")});
        gestures.titlebarDrag = index == 0 ? TitlebarDrag::ScrollView : TitlebarDrag::MoveWindow;
    });
    decodeChildren(node, table);
}

void decodeInput(LoadContext &context, const Kdl::Node &node)
{
    expectOnlyChildren(node);
    Input &input = context.config.input;
    NodeTable table;
    addIgnoredInputHandlers(table, context);
    table.insert(QStringLiteral("focus-follows-mouse"), [&input](const Kdl::Node &child) { decodeFocusFollowsMouse(child, input); });
    table.insert(QStringLiteral("warp-mouse-to-focus"), [&input](const Kdl::Node &child) { decodeWarpMouse(child, input); });
    table.insert(QStringLiteral("workspace-auto-back-and-forth"),
        [&input](const Kdl::Node &child) { input.workspaceAutoBackAndForth = flagArgument(child); });
    table.insert(QStringLiteral("mod-key"), [&input](const Kdl::Node &child) { input.modKey = decodeModKey(child); });
    table.insert(QStringLiteral("mod-key-nested"), [&input](const Kdl::Node &child) { input.modKeyNested = decodeModKey(child); });
    decodeChildren(node, table);
}

void decodeOverview(const Kdl::Node &node, Overview &overview)
{
    expectOnlyChildren(node);
    NodeTable table;
    table.insert(QStringLiteral("zoom"), [&overview](const Kdl::Node &child) { overview.zoom = numberArgument(child, Range {0, 1}); });
    table.insert(
        QStringLiteral("backdrop-color"), [&overview](const Kdl::Node &child) { overview.backdropColor = decodeColorNode(child); });
    table.insert(QStringLiteral("workspace-shadow"),
        [&overview](const Kdl::Node &child) { mergeShadow(overview.workspaceShadow, decodeShadowRule(child)); });
    decodeChildren(node, table);
}

void decodeHotkeyOverlay(const Kdl::Node &node, HotkeyOverlay &overlay)
{
    expectOnlyChildren(node);
    NodeTable table;
    table.insert(QStringLiteral("skip-at-startup"), [&overlay](const Kdl::Node &child) { overlay.skipAtStartup = flagArgument(child); });
    table.insert(QStringLiteral("hide-not-bound"), [&overlay](const Kdl::Node &child) { overlay.hideNotBound = flagArgument(child); });
    decodeChildren(node, table);
}

}
