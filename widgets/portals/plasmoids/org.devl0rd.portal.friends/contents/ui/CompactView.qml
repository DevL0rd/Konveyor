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
    readonly property bool shrink: Plasmoid.configuration.panelShrink
    readonly property string lockedStage: Plasmoid.configuration.panelDetail === "auto" ? "" : Plasmoid.configuration.panelDetail
    property bool wasExpanded: false

    hoverEnabled: true
    onPressed: wasExpanded = root.expanded
    onClicked: root.expanded = !wasExpanded

    readonly property real padding: Kirigami.Units.smallSpacing * 2
    readonly property real minimumSpan: Math.ceil(chip.minimumSize) + padding
    readonly property real preferredSpan: Math.ceil(chip.preferredSize) + padding

    Layout.minimumWidth: vertical ? 0 : Math.max(thickness, shrink ? minimumSpan : preferredSpan)
    Layout.preferredWidth: vertical ? 0 : Math.max(thickness, preferredSpan)
    Layout.minimumHeight: vertical ? Math.max(thickness, shrink ? minimumSpan : preferredSpan) : 0
    Layout.preferredHeight: vertical ? Math.max(thickness, preferredSpan) : 0

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
        lockedStage: compact.lockedStage
        fitSpace: compact.shrink ? (compact.vertical ? compact.height : compact.width) - compact.padding : -1
        iconSource: root.panelIcon
        widestValue: compact.showCount ? "8".repeat(Math.max(2, String(root.friends.length).length)) : ""
        value: compact.showCount ? (root.ready && root.error === "" ? root.onlineCount + "" : "–") : ""
        valueColor: root.error !== "" ? Kirigami.Theme.negativeTextColor : root.inGameCount > 0 ? root.cInGame : Kirigami.Theme.textColor
    }
}
