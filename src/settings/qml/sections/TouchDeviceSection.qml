import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.konveyor.components
import "../components"
import org.kde.konveyor.settings

ColumnLayout {
    id: section

    property string device: "touchpad"
    property string title
    property string kdeGestures
    property string emphasis: ""
    readonly property bool touchscreen: device === "touchscreen"
    readonly property string path: "gestures/" + device
    readonly property var touch: SettingsStore.values[path] || ({})
    readonly property bool on: touch.enabled === true

    function write(name, value) {
        SettingsStore.setValue(section.path + "/" + name, [value]);
    }

    spacing: 0

    CardHeader {
        title: section.title
    }

    Card {
        SwitchRow {
            label: section.touchscreen ? "Use touchscreen gestures" : "Use touchpad gestures"
            description: "The finger counts picked here replace KDE's own gestures with the same count (" + section.kdeGestures + "). Other finger counts keep doing what KDE does."
            iconName: section.touchscreen ? "input-touchscreen" : "input-touchpad"
            resetPaths: [section.path + "/off"]
            isOn: section.on
            onSwitched: on => SettingsStore.setFlag(section.path + "/off", !on)
        }

        SettingRow {
            label: "Swipe with"
            description: "How many fingers a swipe needs before Konveyor takes it."
            resetPaths: [section.path + "/swipe-fingers"]
            enabled: section.on
            onHoveredChanged: section.emphasis = hovered ? "horizontal" : ""

            FingerCountPicker {
                device: section.device
                motion: "swipe-horizontal"
                currentValue: section.touch["swipe-fingers"]
                onChosen: value => section.write("swipe-fingers", value)
            }
        }

        SettingRow {
            label: "Swipe left or right"
            resetPaths: [section.path + "/horizontal-swipe"]
            wideControl: true
            enabled: section.on
            onHoveredChanged: section.emphasis = hovered ? "horizontal" : ""

            ChoiceCards {
                width: parent.width
                cardHeight: Kirigami.Units.gridUnit * 8
                currentValue: section.touch["horizontal-swipe"]
                options: [
                    { value: "scroll-view", title: "Scroll the row", description: "The columns slide with your fingers", preview: gesturePreview, gesture: "horizontal" },
                    { value: "off", title: "Leave it to KDE", description: "KDE handles this swipe", preview: gesturePreview, gesture: "horizontal" }
                ]
                onChosen: value => section.write("horizontal-swipe", value)
            }
        }

        SettingRow {
            label: "Swipe up or down"
            resetPaths: [section.path + "/vertical-swipe"]
            wideControl: true
            enabled: section.on
            onHoveredChanged: section.emphasis = hovered ? "vertical" : ""

            ChoiceCards {
                width: parent.width
                cardHeight: Kirigami.Units.gridUnit * 8
                currentValue: section.touch["vertical-swipe"]
                options: [
                    { value: "switch-workspace", title: "Switch workspace", description: "Go to the workspace above or below", preview: gesturePreview, gesture: "vertical" },
                    { value: "off", title: "Leave it to KDE", description: "KDE handles this swipe", preview: gesturePreview, gesture: "vertical" }
                ]
                onChosen: value => section.write("vertical-swipe", value)
            }
        }

        SwitchRow {
            label: "Natural swiping"
            description: "The row follows your fingers, like scrolling a page. Turn off to move the view the other way."
            iconName: "transform-move-horizontal"
            resetPaths: [section.path + "/natural-swipe"]
            enabled: section.on
            isOn: section.touch["natural-swipe"] === true
            onSwitched: on => section.write("natural-swipe", on)
        }

        SettingRow {
            label: "Move windows with"
            description: "How many fingers it takes to push a window around instead of the row: the focused window on a touchpad, the one under your fingers on a touchscreen."
            resetPaths: [section.path + "/window-swipe-fingers"]
            enabled: section.on
            onHoveredChanged: section.emphasis = hovered ? "window-horizontal" : ""

            FingerCountPicker {
                device: section.device
                motion: "window-swipe-horizontal"
                currentValue: section.touch["window-swipe-fingers"]
                onChosen: value => section.write("window-swipe-fingers", value)
            }
        }

        SettingRow {
            label: "Push the window left or right"
            resetPaths: [section.path + "/window-horizontal-swipe"]
            wideControl: true
            enabled: section.on
            onHoveredChanged: section.emphasis = hovered ? "window-horizontal" : ""

            ChoiceCards {
                width: parent.width
                cardHeight: Kirigami.Units.gridUnit * 8
                currentValue: section.touch["window-horizontal-swipe"]
                options: [
                    { value: "consume-or-expel", title: "Merge into the next column", description: "Stack it with its neighbour, or pop it back out", preview: gesturePreview, gesture: "window-horizontal" },
                    { value: "off", title: "Leave it to KDE", description: "KDE handles this swipe", preview: gesturePreview, gesture: "window-horizontal" }
                ]
                onChosen: value => section.write("window-horizontal-swipe", value)
            }
        }

        SettingRow {
            label: "Push the window up or down"
            resetPaths: [section.path + "/window-vertical-swipe"]
            wideControl: true
            enabled: section.on
            onHoveredChanged: section.emphasis = hovered ? "window-vertical" : ""

            ChoiceCards {
                width: parent.width
                cardHeight: Kirigami.Units.gridUnit * 8
                currentValue: section.touch["window-vertical-swipe"]
                options: [
                    { value: "move-to-workspace", title: "Carry it to another workspace", description: "The window goes to the workspace above or below", preview: gesturePreview, gesture: "window-vertical" },
                    { value: "off", title: "Leave it to KDE", description: "KDE handles this swipe", preview: gesturePreview, gesture: "window-vertical" }
                ]
                onChosen: value => section.write("window-vertical-swipe", value)
            }
        }

        SettingRow {
            label: "Pinch with"
            description: "How many fingers a pinch needs."
            resetPaths: [section.path + "/pinch-fingers"]
            enabled: section.on
            onHoveredChanged: section.emphasis = hovered ? "pinch" : ""

            FingerCountPicker {
                device: section.device
                motion: "pinch"
                currentValue: section.touch["pinch-fingers"]
                onChosen: value => section.write("pinch-fingers", value)
            }
        }

        SettingRow {
            label: "Pinch"
            resetPaths: [section.path + "/pinch"]
            wideControl: true
            enabled: section.on
            onHoveredChanged: section.emphasis = hovered ? "pinch" : ""

            ChoiceCards {
                width: parent.width
                cardHeight: Kirigami.Units.gridUnit * 8
                currentValue: section.touch.pinch
                options: [
                    { value: "toggle-overview", title: "Open the overview", description: "Pinch in to open, out to close", preview: gesturePreview, gesture: "pinch" },
                    { value: "off", title: "Leave it to KDE", description: "KDE handles this pinch", preview: gesturePreview, gesture: "pinch" }
                ]
                onChosen: value => section.write("pinch", value)
            }
        }

        SwitchRow {
            visible: section.touchscreen
            label: "Hold a title bar to move the window"
            description: "Touch a tiled window's title bar, hold still, then drag to pick it up. A quick drag scrolls the row instead."
            iconName: "transform-browse"
            resetPaths: [section.path + "/long-press-to-move"]
            enabled: section.on
            isOn: section.touch["long-press-to-move"] === true
            onSwitched: on => section.write("long-press-to-move", on)
            onHoveredChanged: section.emphasis = hovered ? "long-press" : ""
        }

        SliderRow {
            visible: section.touchscreen
            label: "Hold for"
            description: "How long to hold before the window lifts."
            resetPaths: [section.path + "/long-press-ms"]
            enabled: section.on && section.touch["long-press-to-move"] === true
            from: 100
            to: 2000
            stepSize: 50
            unit: "ms"
            lowLabel: "Quick"
            highLabel: "Long"
            value: section.touch["long-press-ms"] || 500
            onEdited: value => section.write("long-press-ms", Math.round(value))
            onHoveredChanged: section.emphasis = hovered ? "long-press" : ""
        }
    }

    component FingerCountPicker: RowLayout {
        id: picker

        property string device
        property string motion
        property var currentValue
        signal chosen(int value)

        spacing: Kirigami.Units.smallSpacing

        Repeater {
            model: [2, 3, 4, 5]

            QQC2.AbstractButton {
                id: option
                required property int modelData
                readonly property bool selected: picker.currentValue === modelData
                checkable: true
                checked: selected
                hoverEnabled: true
                padding: Kirigami.Units.smallSpacing
                onClicked: picker.chosen(modelData)
                Accessible.name: modelData + " fingers"
                QQC2.ToolTip.text: modelData + " fingers"
                QQC2.ToolTip.visible: hovered
                QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay

                background: Rectangle {
                    radius: Kirigami.Units.cornerRadius * 1.5
                    color: option.selected ? Qt.alpha(Kirigami.Theme.highlightColor, 0.18) : (option.hovered ? Qt.alpha(Kirigami.Theme.textColor, 0.06) : "transparent")
                    border.width: option.selected ? 2 : 1
                    border.color: option.selected ? Kirigami.Theme.highlightColor : Qt.alpha(Kirigami.Theme.textColor, 0.15)
                }

                contentItem: FingerCaps {
                    fingers: option.modelData
                    motion: picker.motion
                    device: picker.device
                    color: "transparent"
                    border.width: 0
                }
            }
        }
    }

    Component {
        id: gesturePreview

        GestureAnimation {
            readonly property string gestureName: parent ? parent.choice.gesture : "horizontal"
            device: section.device
            gesture: gestureName
            fingers: section.touch[gestureName === "pinch" ? "pinch-fingers" : (gestureName.startsWith("window-") ? "window-swipe-fingers" : "swipe-fingers")] || 3
            natural: section.touch["natural-swipe"] === true
            active: parent ? parent.choice.value !== "off" : true
            showCaption: false
        }
    }
}
