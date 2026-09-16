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
    void ignoresInputDevices();
    void parsesOverviewAndHotkeyOverlay();
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
        }
    )"));
    QCOMPARE(config.gestures.dndEdgeViewScroll.triggerSize, 10.0);
    QCOMPARE(config.gestures.dndEdgeViewScroll.delayMs, 50.0);
    QCOMPARE(config.gestures.dndEdgeViewScroll.maxSpeed, 250.0);
    QCOMPARE(config.gestures.dndEdgeWorkspaceSwitch.triggerSize, 20.0);
    QCOMPARE(config.gestures.dndEdgeWorkspaceSwitch.maxSpeed, 900.0);
    QCOMPARE(config.gestures.titlebarDrag, TitlebarDrag::MoveWindow);
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
            focus-follows-mouse max-scroll-amount="25%"
            warp-mouse-to-focus mode="center-xy-always"
            workspace-auto-back-and-forth
            mod-key "Mod5"
            mod-key-nested "Super"
        }
    )"));
    QCOMPARE(config.input.focusFollowsMouse, true);
    QCOMPARE(config.input.focusFollowsMouseMaxScrollPercent, std::optional {0.25});
    QCOMPARE(config.input.warpMouseToFocus, true);
    QCOMPARE(config.input.warpMouseMode, WarpMouseMode::CenterXYAlways);
    QCOMPARE(config.input.workspaceAutoBackAndForth, true);
    QCOMPARE(config.input.modKey, QStringLiteral("ISO_Level3_Shift"));
    QCOMPARE(config.input.modKeyNested, std::optional {QStringLiteral("Super")});

    QVERIFY(mustFail(QStringLiteral("input {\n mod-key \"Hyper\"\n}\n")).message.contains(QStringLiteral("invalid Mod key")));
    QVERIFY(mustFail(QStringLiteral("input {\n focus-follows-mouse max-scroll-amount=\"25\"\n}\n"))
            .message.contains(QStringLiteral("must end with '%'")));
}

void TestConfigInput::ignoresInputDevices()
{
    const LoadResult result = mustLoad(QStringLiteral(R"(
        input {
            keyboard {
                repeat-delay 600
            }
            touchpad { tap; }
            mouse { natural-scroll; }
            trackpoint { off; }
            tablet { map-to-output "eDP-1"; }
            touch { map-to-output "eDP-1"; }
            disable-power-key-handling
        }
    )"));
    QCOMPARE(result.warnings.size(), 7);
    QVERIFY(result.warnings.first().contains(QStringLiteral("KDE manages input devices")));
}

void TestConfigInput::parsesOverviewAndHotkeyOverlay()
{
    const Config config = parsed(QStringLiteral(R"(
        overview {
            zoom 0.25
            backdrop-color "#112233"
            workspace-shadow {
                softness 12
                spread 4
                offset x=1 y=2
                color "#445566"
            }
        }
        hotkey-overlay {
            skip-at-startup
            hide-not-bound
        }
        config-notification {
            disable-failed
        }
    )"));
    QCOMPARE(config.overview.zoom, 0.25);
    QCOMPARE(config.overview.backdropColor, QColor(0x11, 0x22, 0x33));
    QCOMPARE(config.overview.workspaceShadow.softness, 12.0);
    QCOMPARE(config.overview.workspaceShadow.spread, 4.0);
    QCOMPARE(config.overview.workspaceShadow.offset, QPointF(1, 2));
    QCOMPARE(config.overview.workspaceShadow.color, QColor(0x44, 0x55, 0x66));
    QCOMPARE(config.hotkeyOverlay.skipAtStartup, true);
    QCOMPARE(config.hotkeyOverlay.hideNotBound, true);
    QCOMPARE(config.configNotificationDisableFailed, true);
}

QTEST_MAIN(TestConfigInput)
#include "test_config_input.moc"
