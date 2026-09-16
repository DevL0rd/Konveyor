import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import "lib"

MouseArea {
    id: compact

    readonly property bool vertical: Plasmoid.formFactor === PlasmaCore.Types.Vertical
    readonly property real thickness: vertical ? width : height
    readonly property bool showCount: Plasmoid.configuration.showCountBadge
    property bool wasExpanded: false

    hoverEnabled: true
    onPressed: wasExpanded = root.expanded
    onClicked: root.expanded = !wasExpanded

    Layout.minimumWidth: vertical ? 0 : chip.implicitWidth + Kirigami.Units.smallSpacing * 2
    Layout.preferredWidth: Layout.minimumWidth
    Layout.minimumHeight: vertical ? chip.implicitHeight + Kirigami.Units.smallSpacing * 2 : 0
    Layout.preferredHeight: Layout.minimumHeight

    Rectangle {
        anchors.fill: parent
        anchors.margins: 1
        radius: Kirigami.Units.cornerRadius
        color: Qt.alpha(Kirigami.Theme.textColor, compact.containsMouse || root.expanded ? 0.08 : 0)
        Behavior on color { ColorAnimation { duration: 150 } }
    }

    PopChip {
        id: chip
        anchors.centerIn: parent
        vertical: compact.vertical
        panelThickness: compact.thickness
        chipStyle: "none"
        iconSource: root.panelIcon
        widestValue: compact.showCount ? "8".repeat(Math.max(2, String(root.friends.length).length)) : ""
        value: compact.showCount ? (root.ready && root.error === "" ? root.onlineCount + "" : "–") : ""
        valueColor: root.error !== "" ? Kirigami.Theme.negativeTextColor : root.inGameCount > 0 ? root.cInGame : Kirigami.Theme.textColor
    }
}
