import QtQuick
import org.kde.kirigami as Kirigami
import "TaskOrder.js" as TaskOrder

Item {
    id: strip

    property var workspaces: []
    property real size: 32
    property bool vertical: false
    property var badges: ({})
    property bool showBadges: false
    property int contentMode: 0
    property bool animate: true
    property bool wheelSwitches: true

    signal picked(var workspace)
    signal stepped(int step)

    readonly property real padding: Math.round(size * 0.1)
    property var byId: ({})

    onWorkspacesChanged: {
        byId = TaskOrder.keyed(workspaces, workspace => String(workspace.id))
        TaskOrder.syncKeys(ids, workspaces.map(workspace => String(workspace.id)))
    }

    ListModel {
        id: ids
    }

    implicitWidth: vertical ? size : pills.implicitWidth + padding * 2
    implicitHeight: vertical ? pills.implicitHeight + padding * 2 : size

    Rectangle {
        anchors.centerIn: parent
        width: strip.vertical ? Math.round(strip.size * 0.72) : pills.implicitWidth + strip.padding * 2
        height: strip.vertical ? pills.implicitHeight + strip.padding * 2 : Math.round(strip.size * 0.72)
        radius: Math.min(width, height) / 2
        color: Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, 0.07)
        border.width: 1
        border.color: Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, 0.1)
    }

    Grid {
        id: pills
        anchors.centerIn: parent
        rows: strip.vertical ? Math.max(1, strip.workspaces.length) : 1
        columns: strip.vertical ? 1 : Math.max(1, strip.workspaces.length)
        spacing: Math.round(strip.size * 0.08)
        horizontalItemAlignment: Grid.AlignHCenter
        verticalItemAlignment: Grid.AlignVCenter

        move: Transition {
            NumberAnimation { properties: "x,y"; duration: strip.animate ? Kirigami.Units.longDuration : 0; easing.type: Easing.OutCubic }
        }

        Repeater {
            model: ids

            WorkspacePill {
                required property string key
                workspace: strip.byId[key] || ({})
                size: strip.size
                vertical: strip.vertical
                badge: strip.badges[workspace.idx] || ""
                showBadge: strip.showBadges
                contentMode: strip.contentMode
                animate: strip.animate
                onPicked: strip.picked(workspace)
            }
        }
    }

    WheelHandler {
        enabled: strip.wheelSwitches
        property real pending: 0
        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
        onWheel: event => {
            pending += event.angleDelta.y !== 0 ? event.angleDelta.y : event.angleDelta.x
            if (Math.abs(pending) >= 120) {
                strip.stepped(pending > 0 ? -1 : 1)
                pending = 0
            }
        }
    }
}
