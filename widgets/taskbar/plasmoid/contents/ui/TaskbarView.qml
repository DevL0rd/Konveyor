import QtQuick
import QtQuick.Controls
import org.kde.kirigami as Kirigami

Item {
    id: root
    property var entries: []
    property var pinnedLaunchers: []
    property bool grouped: true
    property string groupingMode: "follow"
    property bool vertical: false
    property int iconSize: 48
    property int dropIndex: -1
    property var selectedEntry: null
    signal moveRequested(int from, int to)
    signal activateRequested(var entry)
    signal closeRequested(var entry)
    signal pinRequested(var entry)
    signal groupingRequested(string mode)
    implicitWidth: vertical ? iconSize : iconSize * Math.max(1, entries.length)
    implicitHeight: vertical ? iconSize * Math.max(1, entries.length) : iconSize

    Text {
        anchors.centerIn: parent
        visible: root.entries.length === 0
        text: qsTr("Open an app to pin it here")
        color: Kirigami.Theme.textColor
        font.pixelSize: 12
    }

    Repeater {
        model: root.entries
        delegate: ToolButton {
            id: button
            required property var modelData
            required property int index
            readonly property var entry: modelData.entry || modelData
            objectName: "task-" + entry.appId
            x: root.vertical ? 0 : index * root.iconSize
            y: root.vertical ? index * root.iconSize : 0
            width: root.iconSize
            height: root.iconSize
            Accessible.name: entry.title
            ToolTip.visible: mouse.containsMouse && !mouse.pressed && !menu.visible
            ToolTip.text: entry.title
            ToolTip.delay: 750
            onClicked: root.activateRequested(entry)
            background: Rectangle {
                radius: 6
                color: button.entry.active || mouse.containsMouse ? Qt.alpha(Kirigami.Theme.highlightColor, 0.25) : "transparent"
                border.width: root.dropIndex === button.index ? 2 : 0
                border.color: Kirigami.Theme.highlightColor
            }
            contentItem: Kirigami.Icon {
                source: button.entry.icon
                implicitWidth: 32
                implicitHeight: 32
            }
            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom
                width: button.entry.active ? 24 : 8
                height: 3
                radius: 1
                visible: !button.entry.launcher
                color: Kirigami.Theme.highlightColor
            }
            Text {
                anchors.right: parent.right
                anchors.top: parent.top
                visible: button.entry.windowIds.length > 1
                text: button.entry.windowIds.length
                color: Kirigami.Theme.textColor
            }
            MouseArea {
                id: mouse
                z: 10
                objectName: "mouse-" + button.entry.appId
                anchors.fill: parent
                hoverEnabled: true
                preventStealing: true
                acceptedButtons: Qt.LeftButton | Qt.MiddleButton | Qt.RightButton
                property point startPosition
                property bool moved: false
                onPressed: event => {
                    startPosition = Qt.point(event.x, event.y)
                    moved = false
                }
                onPositionChanged: event => {
                    if (!(pressedButtons & Qt.LeftButton))
                        return
                    if (Math.abs(event.x - startPosition.x) + Math.abs(event.y - startPosition.y) > 12)
                        moved = true
                    if (moved) {
                        const point = root.mapFromItem(mouse, event.x, event.y)
                        root.dropIndex = Math.max(0, Math.min(root.entries.length - 1, Math.floor((root.vertical ? point.y : point.x) / root.iconSize)))
                    }
                }
                onReleased: event => {
                    if (moved && root.dropIndex >= 0)
                        root.moveRequested(button.entry.index, root.entries[root.dropIndex].index)
                    root.dropIndex = -1
                }
                onCanceled: root.dropIndex = -1
                onClicked: event => {
                    if (moved)
                        return
                    if (event.button === Qt.MiddleButton)
                        root.closeRequested(button.entry)
                    else if (event.button === Qt.RightButton) {
                        root.selectedEntry = button.entry
                        menu.popup()
                    } else
                        root.activateRequested(button.entry)
                }
            }
        }
    }
    Menu {
        id: menu
        MenuItem {
            text: root.selectedEntry && root.pinnedLaunchers.includes(root.selectedEntry.launcherUrl) ? qsTr("Unpin application") : qsTr("Pin application")
            enabled: !!root.selectedEntry && !!root.selectedEntry.launcherUrl
            onTriggered: root.pinRequested(root.selectedEntry)
        }
        MenuItem {
            text: root.groupingMode === "follow" ? qsTr("Grouping: Follow Konveyor")
                : root.groupingMode === "grouped" ? qsTr("Grouping: Group applications") : qsTr("Grouping: Separate windows")
            onTriggered: root.groupingRequested(root.groupingMode === "follow" ? "grouped" : root.groupingMode === "grouped" ? "separate" : "follow")
        }
    }
}
