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
