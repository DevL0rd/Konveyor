import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PlasmaComponents

Rectangle {
    id: pinHint
    readonly property Item target: launcher.hoveredPin || (launcher.railIndex >= 0 ? pinsView.itemAtIndex(launcher.railIndex) : null)
    property var pin: null
    onTargetChanged: if (target) pin = target.modelData
    readonly property bool wanted: target !== null && !pinHintDelay.running && launcher.sidebarDrag === null && !menu.visible
    visible: opacity > 0
    opacity: wanted ? 1 : 0
    Behavior on opacity { NumberAnimation { duration: 120 } }
    x: {
        target
        rail.width
        return Math.round(rail.mapToItem(content, rail.width, 0).x + Kirigami.Units.largeSpacing)
    }
    y: {
        pinsView.contentY
        launcher.contentProgress
        return target ? Math.round(target.mapToItem(content, 0, target.height / 2).y - height / 2) : y
    }
    width: pinHintRow.implicitWidth + Kirigami.Units.largeSpacing * 1.5
    height: pinHintRow.implicitHeight + Kirigami.Units.smallSpacing * 2
    radius: height / 2
    color: Qt.rgba(0.08, 0.08, 0.09, 0.96)
    border.width: 1
    border.color: launcher.hairline
    Timer {
        id: pinHintDelay
        interval: 450
        running: pinHint.target !== null
    }
    RowLayout {
        id: pinHintRow
        anchors.centerIn: parent
        spacing: Kirigami.Units.smallSpacing * 1.5
        PlasmaComponents.Label {
            text: pinHint.pin ? pinHint.pin.name : ""
            color: "white"
        }
        PlasmaComponents.Label {
            readonly property var game: pinHint.pin ? launcherData.sidebarGame(pinHint.pin) : null
            text: !pinHint.pin ? "" : pinHint.pin.missing ? i18n("Missing") : game ? i18n("Game") : pinHint.pin.kind === "app" ? i18n("App") : pinHint.pin.folder ? i18n("Folder") : i18n("File")
            color: pinHint.pin && pinHint.pin.missing ? Kirigami.Theme.negativeTextColor : "white"
            font.pointSize: Kirigami.Theme.smallFont.pointSize
            opacity: pinHint.pin && pinHint.pin.missing ? 1 : 0.55
        }
    }
}
