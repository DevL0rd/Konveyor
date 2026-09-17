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

TapAction decodeTapAction(const Kdl::Node &node)
{
    static const QList<TapAction> actions {TapAction::CycleWidth, TapAction::KontrolPanel, TapAction::ToggleOverview, TapAction::Off};
    return actions.at(keywordArgument(
        node, {QStringLiteral("cycle-width"), QStringLiteral("kontrol-panel"), QStringLiteral("toggle-overview"), QStringLiteral("off")}));
}

void decodeMultiTouch(const Kdl::Node &node, MultiTouch &touch, bool isTouchscreen)
{
    expectOnlyChildren(node);
    NodeTable table;
    table.insert(QStringLiteral("off"), [&touch](const Kdl::Node &child) { touch.enabled = !flagArgument(child); });
    table.insert(QStringLiteral("swipe-fingers"),
        [&touch](const Kdl::Node &child) { touch.swipeFingers = static_cast<int>(integerArgument(child, Range {2, 5})); });
    table.insert(QStringLiteral("pinch-fingers"),
        [&touch](const Kdl::Node &child) { touch.pinchFingers = static_cast<int>(integerArgument(child, Range {2, 5})); });
    table.insert(QStringLiteral("natural-swipe"), [&touch](const Kdl::Node &child) { touch.naturalSwipe = flagArgument(child); });
    table.insert(QStringLiteral("horizontal-swipe"), [&touch](const Kdl::Node &child) {
        touch.horizontalSwipe = keywordArgument(child, {QStringLiteral("scroll-view"), QStringLiteral("off")}) == 0
            ? HorizontalSwipe::ScrollView
            : HorizontalSwipe::Off;
    });
    table.insert(QStringLiteral("vertical-swipe"), [&touch](const Kdl::Node &child) {
        touch.verticalSwipe = keywordArgument(child, {QStringLiteral("switch-workspace"), QStringLiteral("off")}) == 0
            ? VerticalSwipe::SwitchWorkspace
            : VerticalSwipe::Off;
    });
    table.insert(QStringLiteral("pinch"), [&touch](const Kdl::Node &child) {
        touch.pinch = keywordArgument(child, {QStringLiteral("toggle-overview"), QStringLiteral("off")}) == 0 ? PinchAction::ToggleOverview
                                                                                                              : PinchAction::Off;
    });
    table.insert(QStringLiteral("window-swipe-fingers"),
        [&touch](const Kdl::Node &child) { touch.windowSwipeFingers = static_cast<int>(integerArgument(child, Range {2, 5})); });
    table.insert(QStringLiteral("window-horizontal-swipe"), [&touch](const Kdl::Node &child) {
        touch.windowHorizontalSwipe = keywordArgument(child, {QStringLiteral("consume-or-expel"), QStringLiteral("off")}) == 0
            ? WindowHorizontalSwipe::ConsumeOrExpel
            : WindowHorizontalSwipe::Off;
    });
    table.insert(QStringLiteral("window-vertical-swipe"), [&touch](const Kdl::Node &child) {
        touch.windowVerticalSwipe
            = keywordArgument(child, {QStringLiteral("move-window"), QStringLiteral("move-to-workspace"), QStringLiteral("off")}) < 2
            ? WindowVerticalSwipe::MoveWindow
            : WindowVerticalSwipe::Off;
    });
    table.insert(QStringLiteral("three-finger-tap"), [&touch](const Kdl::Node &child) { touch.threeFingerTap = decodeTapAction(child); });
    table.insert(QStringLiteral("four-finger-tap"), [&touch](const Kdl::Node &child) { touch.fourFingerTap = decodeTapAction(child); });
    table.insert(QStringLiteral("five-finger-tap"), [&touch](const Kdl::Node &child) { touch.fiveFingerTap = decodeTapAction(child); });
    if (isTouchscreen) {
        table.insert(
            QStringLiteral("long-press-to-move"), [&touch](const Kdl::Node &child) { touch.longPressToMove = flagArgument(child); });
        table.insert(QStringLiteral("long-press-ms"),
            [&touch](const Kdl::Node &child) { touch.longPressMs = static_cast<int>(integerArgument(child, Range {100, 5000})); });
    }
    decodeChildren(node, table);
}

}

void decodeGestures(const Kdl::Node &node, Gestures &gestures)
{
    expectOnlyChildren(node);
    NodeTable table;
    table.insert(QStringLiteral("dnd-edge-view-scroll"),
        [&gestures](const Kdl::Node &child) { decodeEdgeScroll(child, QStringLiteral("trigger-width"), gestures.dndEdgeViewScroll); });
    table.insert(QStringLiteral("dnd-edge-workspace-switch"), [&gestures](const Kdl::Node &child) {
        decodeEdgeScroll(child, QStringLiteral("trigger-height"), gestures.dndEdgeWorkspaceSwitch);
    });
    table.insert(QStringLiteral("touchpad"), [&gestures](const Kdl::Node &child) { decodeMultiTouch(child, gestures.touchpad, false); });
    table.insert(
        QStringLiteral("touchscreen"), [&gestures](const Kdl::Node &child) { decodeMultiTouch(child, gestures.touchscreen, true); });
    table.insert(QStringLiteral("hot-corners"), [&gestures](const Kdl::Node &child) { gestures.hotCorners = decodeHotCorners(child); });
    table.insert(QStringLiteral("titlebar-drag"), [&gestures](const Kdl::Node &child) {
        const int index = keywordArgument(child, {QStringLiteral("scroll-view"), QStringLiteral("move-window")});
        gestures.titlebarDrag = index == 0 ? TitlebarDrag::ScrollView : TitlebarDrag::MoveWindow;
    });
    table.insert(
        QStringLiteral("resize-tiled-windows"), [&gestures](const Kdl::Node &child) { gestures.resizeTiledWindows = flagArgument(child); });
    decodeChildren(node, table);
}

void decodeInput(const Kdl::Node &node, Input &input)
{
    expectOnlyChildren(node);
    NodeTable table;
    table.insert(
        QStringLiteral("focus-follows-mouse"), [&input](const Kdl::Node &child) { input.focusFollowsMouse = flagArgument(child); });
    table.insert(QStringLiteral("warp-mouse-to-focus"), [&input](const Kdl::Node &child) { decodeWarpMouse(child, input); });
    table.insert(QStringLiteral("workspace-auto-back-and-forth"),
        [&input](const Kdl::Node &child) { input.workspaceAutoBackAndForth = flagArgument(child); });
    table.insert(QStringLiteral("mod-key"), [&input](const Kdl::Node &child) { input.modKey = decodeModKey(child); });
    decodeChildren(node, table);
}

}
