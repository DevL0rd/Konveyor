import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import "../components"
import "../previews"
import org.kde.konveyor.settings

SettingsPage {
    id: page

    readonly property var output: SettingsStore.live.outputs.length > 0 ? SettingsStore.live.outputs[0].logical : ({ width: 1920, height: 1080 })
    readonly property var corners: SettingsStore.values["gestures/hot-corners"] || ({})
    readonly property var viewScroll: SettingsStore.values["gestures/dnd-edge-view-scroll"] || ({})
    readonly property var workspaceSwitch: SettingsStore.values["gestures/dnd-edge-workspace-switch"] || ({})
    readonly property string warpChoice: SettingsStore.values["input/warp-mouse-to-focus"] !== true ? "off" : (SettingsStore.values["input/warp-mouse-to-focus/mode"] || "nearest")
    property string edgeEmphasis: ""

    readonly property var cornerIds: ["top-left", "top-right", "bottom-left", "bottom-right"]

    function toggleCorner(corner) {
        const next = {};
        for (const id of cornerIds) {
            next[id] = id === corner ? !corners[id] : corners[id] === true;
        }
        if (!cornerIds.some(id => next[id])) {
            SettingsStore.setFlag("gestures/hot-corners/off", true);
            return;
        }
        for (const id of cornerIds) {
            SettingsStore.setFlag("gestures/hot-corners/" + id, next[id]);
        }
    }

    function writeWarp(choice) {
        if (choice === "off") {
            SettingsStore.remove("input/warp-mouse-to-focus");
        } else {
            SettingsStore.setValue("input/warp-mouse-to-focus", [], choice === "nearest" ? {} : { mode: choice });
        }
    }

    title: "Mouse & Gestures"
    preview: EdgeZonesMap {
        screenWidth: page.output.width
        screenHeight: page.output.height
        viewTrigger: page.viewScroll.trigger || 0
        workspaceTrigger: page.workspaceSwitch.trigger || 0
        emphasis: page.edgeEmphasis
    }

    CardHeader {
        title: "Focus"
    }

    Card {
        SwitchRow {
            label: "Focus follows the mouse"
            description: "Hovering a window focuses it. The view doesn't scroll just because the pointer passes over a partly visible column."
            iconName: "input-mouse"
            resetPaths: ["input/focus-follows-mouse"]
            isOn: SettingsStore.values["input/focus-follows-mouse"] === true
            onSwitched: on => SettingsStore.setFlag("input/focus-follows-mouse", on)
        }

        SettingRow {
            label: "Move the pointer with keyboard focus"
            description: "When a shortcut focuses another window, bring the pointer along so it's where you're looking."
            iconName: "transform-move"
            resetPaths: ["input/warp-mouse-to-focus"]
            wideControl: true

            ChoiceCards {
                width: parent.width
                cardHeight: Kirigami.Units.gridUnit * 7.5
                currentValue: page.warpChoice
                options: [
                    { value: "off", title: "Leave it", description: "The pointer stays where it is", preview: warpPreview },
                    { value: "nearest", title: "Nudge inside", description: "Only if it's outside, to the nearest edge", preview: warpPreview },
                    { value: "center-xy", title: "Center if outside", description: "Jumps to the middle when it's outside", preview: warpPreview },
                    { value: "center-xy-always", title: "Always center", description: "Jumps to the middle every time", preview: warpPreview }
                ]
                onChosen: value => page.writeWarp(value)
            }
        }
    }

    CardHeader {
        title: "Dragging windows"
    }

    Card {
        SettingRow {
            label: "Dragging a tiled window's title bar"
            description: "What happens when you grab the title bar of a window in the row."
            iconName: "transform-browse"
            resetPaths: ["gestures/titlebar-drag"]
            wideControl: true

            ChoiceCards {
                width: parent.width
                cardHeight: Kirigami.Units.gridUnit * 8
                currentValue: SettingsStore.values["gestures/titlebar-drag"]
                options: [
                    { value: "scroll-view", title: "Scroll the row", description: "The whole row slides with your pointer", preview: dragPreview },
                    { value: "move-window", title: "Move the window", description: "Pick the window up and drop it somewhere else", preview: dragPreview }
                ]
                onChosen: value => SettingsStore.setValue("gestures/titlebar-drag", [value])
            }
        }

        SwitchRow {
            label: "Resize tiled windows by dragging their edges"
            description: "When off, window edges in the row can't be dragged, so the layout only changes through shortcuts and presets. Floating windows can always be resized."
            iconName: "transform-scale"
            resetPaths: ["gestures/resize-tiled-windows"]
            isOn: SettingsStore.values["gestures/resize-tiled-windows"]
            onSwitched: on => SettingsStore.setFlag("gestures/resize-tiled-windows", on)
        }
    }

    CardHeader {
        title: "Hot corners"
    }

    Card {
        SwitchRow {
            label: "Open the overview from screen corners"
            description: "Plasma's own screen edge actions also work; turn this off if you set those up instead."
            iconName: "preferences-desktop-screensaver"
            resetPaths: ["gestures/hot-corners"]
            isOn: page.corners.enabled === true
            onSwitched: on => SettingsStore.setFlag("gestures/hot-corners/off", !on)
        }

        SettingRow {
            label: "Active corners"
            description: "Click a corner to turn it on or off."
            resetPaths: ["gestures/hot-corners"]
            wideControl: true
            enabled: page.corners.enabled === true

            HotCornersMap {
                width: parent.width
                height: Kirigami.Units.gridUnit * 10
                corners: page.corners
                active: page.corners.enabled === true
                aspect: page.output.width / Math.max(1, page.output.height)
                onToggled: corner => page.toggleCorner(corner)
            }
        }
    }

    CardHeader {
        title: "Scrolling while dragging"
    }

    EdgeScrollCard {
        path: "gestures/dnd-edge-view-scroll"
        triggerName: "trigger-width"
        values: page.viewScroll
        label: "Scroll the row"
        description: "Drag a window or file to the left or right edge and the row scrolls to show more columns."
        triggerLabel: "Edge width"
        iconName: "go-next-view"
        onHoveredChanged: page.edgeEmphasis = hovered ? "view" : ""
    }

    EdgeScrollCard {
        Layout.topMargin: Kirigami.Units.largeSpacing * 2
        path: "gestures/dnd-edge-workspace-switch"
        triggerName: "trigger-height"
        values: page.workspaceSwitch
        label: "Switch workspaces"
        description: "Drag to the top or bottom edge to move to the workspace above or below."
        triggerLabel: "Edge height"
        iconName: "virtual-desktops"
        onHoveredChanged: page.edgeEmphasis = hovered ? "workspace" : ""
    }

    Component {
        id: warpPreview

        WarpDemo {
            choice: parent ? parent.choice.value : "off"
        }
    }

    Component {
        id: dragPreview

        TitlebarDragDemo {
            mode: parent ? parent.choice.value : "scroll-view"
        }
    }
}
