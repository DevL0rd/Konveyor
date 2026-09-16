import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import ".."
import "../../previews"

ColumnLayout {
    id: editor

    property string blockPath
    readonly property var node: kcm.revision >= 0 ? kcm.node(blockPath) : ({})
    readonly property bool overridden: node.name !== undefined
    readonly property var flags: (node.children || []).map(child => child.name)
    readonly property bool enabledHere: !flags.includes("off")
    readonly property var corners: ["top-left", "top-right", "bottom-left", "bottom-right"]
    readonly property var activeCorners: {
        const set = corners.filter(corner => flags.includes(corner));
        return set.length ? set : ["top-left"];
    }
    readonly property var globalCorners: kcm.values["gestures/hot-corners"] || ({})

    function toggle(corner) {
        const next = activeCorners.includes(corner) ? activeCorners.filter(entry => entry !== corner) : activeCorners.concat([corner]);
        const children = (enabledHere ? [] : [{ name: "off", args: [], props: {} }]).concat(next.map(entry => ({ name: entry, args: [], props: {} })));
        kcm.setNode(blockPath, { name: "hot-corners", args: [], props: {}, children: children });
    }

    spacing: 0

    SwitchRow {
        label: "Own hot corners"
        description: editor.overridden ? "This monitor ignores the hot corners from Mouse & Gestures" : "Uses the hot corners from Mouse & Gestures"
        iconName: "transform-move"
        isOn: editor.overridden
        onSwitched: on => on ? kcm.setNode(editor.blockPath, { name: "hot-corners", args: [], props: {}, children: [{ name: "top-left", args: [], props: {} }] }) : kcm.remove(editor.blockPath)
    }

    SwitchRow {
        visible: editor.overridden
        label: "Hot corners on this monitor"
        isOn: editor.enabledHere
        onSwitched: on => on ? kcm.remove(editor.blockPath + "/off") : kcm.append(editor.blockPath, { name: "off", args: [], props: {} })
    }

    SettingRow {
        visible: editor.overridden && editor.enabledHere
        label: "Corners"
        description: "Click a corner to turn it on or off. Pushing the pointer into an active corner opens the overview."

        MonitorFrame {
            height: Kirigami.Units.gridUnit * 6
            aspect: 16 / 9

            Repeater {
                model: editor.corners

                Rectangle {
                    id: corner
                    required property string modelData
                    readonly property bool on: editor.activeCorners.includes(modelData)
                    width: Kirigami.Units.gridUnit * 1.6
                    height: width
                    radius: width / 2
                    x: modelData.endsWith("left") ? -width / 3 : parent.width - width * 2 / 3
                    y: modelData.startsWith("top") ? -height / 3 : parent.height - height * 2 / 3
                    color: on ? Kirigami.Theme.highlightColor : Qt.alpha(Kirigami.Theme.textColor, cornerArea.containsMouse ? 0.4 : 0.2)

                    Accessible.role: Accessible.CheckBox
                    Accessible.name: modelData.replace("-", " ") + " corner"
                    Accessible.checked: on

                    MouseArea {
                        id: cornerArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: editor.toggle(corner.modelData)
                    }
                }
            }
        }
    }
}
