import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.components as PlasmaComponents

MouseArea {
    id: compact

    readonly property bool vertical: Plasmoid.formFactor === PlasmaCore.Types.Vertical
    readonly property real thickness: vertical ? width : height
    readonly property bool showBadge: Plasmoid.configuration.showFriendsBadge && root.friendsPlaying > 0
    property bool wasExpanded: false

    hoverEnabled: true
    onPressed: wasExpanded = root.expanded
    onClicked: root.expanded = !wasExpanded

    Layout.minimumWidth: vertical ? 0 : thickness
    Layout.preferredWidth: Layout.minimumWidth
    Layout.minimumHeight: vertical ? thickness : 0
    Layout.preferredHeight: Layout.minimumHeight

    Rectangle {
        anchors.fill: parent
        anchors.margins: 1
        radius: Kirigami.Units.cornerRadius
        color: Qt.alpha(Kirigami.Theme.textColor, compact.containsMouse || root.expanded ? 0.08 : 0)
        Behavior on color { ColorAnimation { duration: 150 } }
    }

    Kirigami.Icon {
        id: icon
        anchors.centerIn: parent
        width: Math.round(compact.thickness * 0.66)
        height: width
        source: root.panelIcon
        active: compact.containsMouse
        scale: compact.pressed ? 0.92 : 1
        Behavior on scale { NumberAnimation { duration: 90 } }
    }

    Rectangle {
        visible: compact.showBadge
        anchors.right: icon.right
        anchors.bottom: icon.bottom
        anchors.rightMargin: -Math.round(width * 0.25)
        anchors.bottomMargin: -Math.round(height * 0.2)
        height: Math.round(compact.thickness * 0.34)
        width: height
        radius: height / 2
        color: Kirigami.Theme.textColor
        border.width: 1.5
        border.color: Kirigami.Theme.backgroundColor
        PlasmaComponents.Label {
            anchors.centerIn: parent
            text: root.friendsPlaying > 9 ? "9+" : root.friendsPlaying
            color: Kirigami.Theme.backgroundColor
            font.weight: Font.Bold
            font.pixelSize: Math.round(parent.height * (root.friendsPlaying > 9 ? 0.5 : 0.66))
        }
    }
}
