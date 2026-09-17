import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.konveyor.components
import "../components"
import "../sections"
import org.kde.konveyor.settings

SettingsPage {
    id: page

    readonly property var touchpad: SettingsStore.values["gestures/touchpad"] || ({})
    readonly property var touchscreen: SettingsStore.values["gestures/touchscreen"] || ({})
    readonly property string hoveredDevice: touchscreenSection.emphasis !== "" ? "touchscreen" : (touchpadSection.emphasis !== "" ? "touchpad" : "")
    readonly property string hoveredGesture: touchscreenSection.emphasis || touchpadSection.emphasis
    readonly property var tour: [
        { device: "touchpad", gesture: "horizontal" },
        { device: "touchpad", gesture: "vertical" },
        { device: "touchpad", gesture: "window-horizontal" },
        { device: "touchpad", gesture: "window-vertical" },
        { device: "touchpad", gesture: "pinch" },
        { device: "touchpad", gesture: "tap-3" },
        { device: "touchpad", gesture: "tap-4" },
        { device: "touchscreen", gesture: "horizontal" },
        { device: "touchscreen", gesture: "long-press" }
    ]
    property int tourStep: 0
    readonly property var shown: hoveredGesture !== "" ? { device: hoveredDevice, gesture: hoveredGesture } : tour[tourStep]
    readonly property var shownSettings: shown.device === "touchscreen" ? touchscreen : touchpad
    readonly property int shownTapFingers: shown.gesture.startsWith("tap-") ? Number(shown.gesture.slice(4)) : 0
    readonly property string shownTapAction: shownTapFingers > 0 ? (shownSettings[tapKeys[shownTapFingers]] || "off") : ""
    readonly property var tapKeys: ({ 3: "three-finger-tap", 4: "four-finger-tap", 5: "five-finger-tap" })

    function shownFingers() {
        if (shownTapFingers > 0) {
            return shownTapFingers;
        }
        return shownSettings[shown.gesture === "pinch" ? "pinch-fingers" : (shown.gesture.startsWith("window-") ? "window-swipe-fingers" : "swipe-fingers")] || 3;
    }

    function isActive(settings, gesture) {
        if (settings.enabled !== true) {
            return false;
        }
        switch (gesture) {
        case "horizontal":
            return settings["horizontal-swipe"] !== "off";
        case "vertical":
            return settings["vertical-swipe"] !== "off";
        case "window-horizontal":
            return settings["window-horizontal-swipe"] !== "off";
        case "window-vertical":
            return settings["window-vertical-swipe"] !== "off";
        case "pinch":
            return settings.pinch !== "off";
        case "long-press":
            return settings["long-press-to-move"] === true;
        case "tap-3":
        case "tap-4":
        case "tap-5":
            return (settings[tapKeys[Number(gesture.slice(4))]] || "off") !== "off";
        }
        return false;
    }

    title: "Touch & Gestures"
    preview: GestureAnimation {
        device: page.shown.device
        gesture: page.shownTapFingers > 0 ? "tap-" + (page.shownTapAction === "off" ? "cycle-width" : page.shownTapAction) : page.shown.gesture
        fingers: page.shownFingers()
        natural: page.shownSettings["natural-swipe"] === true
        active: page.isActive(page.shownSettings, page.shown.gesture)
        onLooped: {
            if (page.hoveredGesture === "") {
                page.tourStep = (page.tourStep + 1) % page.tour.length;
            }
        }
    }

    TouchDeviceSection {
        id: touchpadSection
        Layout.fillWidth: true
        device: "touchpad"
        title: "Touchpad"
        kdeGestures: "3 and 4 finger swipes switch desktops, 4 fingers up opens Overview"
    }

    TouchDeviceSection {
        id: touchscreenSection
        Layout.fillWidth: true
        Layout.topMargin: Kirigami.Units.largeSpacing * 2
        device: "touchscreen"
        title: "Touchscreen"
        kdeGestures: "3 finger swipes switch desktops and open Overview"
    }
}
