#include "configtesthelpers.h"

#include <QTest>

using namespace Konveyor::Config;
using namespace Konveyor::Config::Testing;

class TestConfigInput : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void parsesGestures();
    void parsesHotCorners();
    void parsesMultiTouch();
    void parsesInput();
    void rejectsUnknownInputOptions();
    void parsesConfigNotification();
};

void TestConfigInput::parsesGestures()
{
    const Config config = parsed(QStringLiteral(R"(
        gestures {
            dnd-edge-view-scroll {
                trigger-width 10
                delay-ms 50
                max-speed 250
            }
            dnd-edge-workspace-switch {
                trigger-height 20
                max-speed 900
            }
            titlebar-drag "move-window"
            resize-tiled-windows
        }
    )"));
    QCOMPARE(config.gestures.dndEdgeViewScroll.triggerSize, 10.0);
    QCOMPARE(config.gestures.dndEdgeViewScroll.delayMs, 50.0);
    QCOMPARE(config.gestures.dndEdgeViewScroll.maxSpeed, 250.0);
    QCOMPARE(config.gestures.dndEdgeWorkspaceSwitch.triggerSize, 20.0);
    QCOMPARE(config.gestures.dndEdgeWorkspaceSwitch.maxSpeed, 900.0);
    QCOMPARE(config.gestures.titlebarDrag, TitlebarDrag::MoveWindow);
    QCOMPARE(config.gestures.resizeTiledWindows, true);
    QVERIFY(mustFail(QStringLiteral("gestures {\n titlebar-drag \"wobble\"\n}\n")).message.contains(QStringLiteral("expected one of")));
}

void TestConfigInput::parsesMultiTouch()
{
    const Config defaults = parsed(QStringLiteral(""));
    QCOMPARE(defaults.gestures.touchpad.enabled, true);
    QCOMPARE(defaults.gestures.touchpad.swipeFingers, 3);
    QCOMPARE(defaults.gestures.touchpad.pinchFingers, 4);
    QCOMPARE(defaults.gestures.touchscreen.longPressToMove, true);
    QCOMPARE(defaults.gestures.touchscreen.longPressMs, 500);

    const Config config = parsed(QStringLiteral(R"(
        gestures {
            touchpad {
                swipe-fingers 4
                pinch-fingers 3
                natural-swipe false
                horizontal-swipe "off"
                pinch "off"
                window-swipe-fingers 5
                window-horizontal-swipe "off"
                window-vertical-swipe "move-to-workspace"
            }
            touchscreen {
                window-vertical-swipe "off"
                off
                vertical-swipe "off"
                long-press-to-move false
                long-press-ms 800
            }
        }
    )"));
    QCOMPARE(config.gestures.touchpad.swipeFingers, 4);
    QCOMPARE(config.gestures.touchpad.pinchFingers, 3);
    QCOMPARE(config.gestures.touchpad.naturalSwipe, false);
    QCOMPARE(config.gestures.touchpad.horizontalSwipe, HorizontalSwipe::Off);
    QCOMPARE(config.gestures.touchpad.verticalSwipe, VerticalSwipe::SwitchWorkspace);
    QCOMPARE(config.gestures.touchpad.pinch, PinchAction::Off);
    QCOMPARE(defaults.gestures.touchpad.windowSwipeFingers, 4);
    QCOMPARE(config.gestures.touchpad.windowSwipeFingers, 5);
    QCOMPARE(config.gestures.touchpad.windowHorizontalSwipe, WindowHorizontalSwipe::Off);
    QCOMPARE(config.gestures.touchpad.windowVerticalSwipe, WindowVerticalSwipe::MoveWindow);
    QCOMPARE(config.gestures.touchscreen.windowVerticalSwipe, WindowVerticalSwipe::Off);
    QCOMPARE(config.gestures.touchscreen.enabled, false);
    QCOMPARE(config.gestures.touchscreen.verticalSwipe, VerticalSwipe::Off);
    QCOMPARE(config.gestures.touchscreen.longPressToMove, false);
    QCOMPARE(config.gestures.touchscreen.longPressMs, 800);
    QVERIFY(mustFail(QStringLiteral("gestures {\n touchpad {\n swipe-fingers 9\n }\n}\n")).message.length() > 0);
    QVERIFY(mustFail(QStringLiteral("gestures {\n touchpad {\n long-press-ms 800\n }\n}\n")).message.length() > 0);
}

void TestConfigInput::parsesHotCorners()
{
    const Config off = parsed(QStringLiteral("gestures {\n hot-corners {\n off\n }\n}\n"));
    QCOMPARE(off.gestures.hotCorners.enabled, false);

    const Config corners = parsed(QStringLiteral("gestures {\n hot-corners {\n top-right\n bottom-left\n }\n}\n"));
    QCOMPARE(corners.gestures.hotCorners.enabled, true);
    QCOMPARE(corners.gestures.hotCorners.topLeft, false);
    QCOMPARE(corners.gestures.hotCorners.topRight, true);
    QCOMPARE(corners.gestures.hotCorners.bottomLeft, true);
    QCOMPARE(corners.gestures.hotCorners.bottomRight, false);

    const Config implied = parsed(QStringLiteral("gestures {\n hot-corners {\n }\n}\n"));
    QCOMPARE(implied.gestures.hotCorners.topLeft, true);
}

void TestConfigInput::parsesInput()
{
    const Config config = parsed(QStringLiteral(R"(
        input {
            focus-follows-mouse
            warp-mouse-to-focus mode="center-xy-always"
            workspace-auto-back-and-forth
            mod-key "Mod5"
        }
    )"));
    QCOMPARE(config.input.focusFollowsMouse, true);
    QCOMPARE(config.input.warpMouseToFocus, true);
    QCOMPARE(config.input.warpMouseMode, WarpMouseMode::CenterXYAlways);
    QCOMPARE(config.input.workspaceAutoBackAndForth, true);
    QCOMPARE(config.input.modKey, QStringLiteral("ISO_Level3_Shift"));

    QVERIFY(mustFail(QStringLiteral("input {\n mod-key \"Hyper\"\n}\n")).message.contains(QStringLiteral("invalid Mod key")));
}

void TestConfigInput::rejectsUnknownInputOptions()
{
    QVERIFY(mustFail(QStringLiteral("input {\n focus-follows-mouse max-scroll-amount=\"25%\"\n}\n"))
            .message.contains(QStringLiteral("unexpected property")));
    QVERIFY(mustFail(QStringLiteral("input {\n mod-key-nested \"Super\"\n}\n")).message.contains(QStringLiteral("unexpected node")));
    QVERIFY(
        mustFail(QStringLiteral("input {\n keyboard {\n repeat-delay 600\n }\n}\n")).message.contains(QStringLiteral("unexpected node")));
}

void TestConfigInput::parsesConfigNotification()
{
    const Config config = parsed(QStringLiteral("config-notification {\n disable-failed\n}\n"));
    QCOMPARE(config.configNotificationDisableFailed, true);
}

QTEST_MAIN(TestConfigInput)
#include "test_config_input.moc"
