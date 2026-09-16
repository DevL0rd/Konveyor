import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.plasma.plasmoid
import org.kde.plasma.components as PlasmaComponents
import ".."

ColumnLayout {
    id: page

    readonly property var sections: [grid]
    readonly property int tileSize: Plasmoid.configuration.tileSize
    property int categoryRow: 0
    readonly property var categoryModel: launcherData.rootModel.count > categoryRow ? launcherData.rootModel.modelForRow(categoryRow) : null

    spacing: Kirigami.Units.largeSpacing

    function cycle(forward) {
        const total = launcherData.rootModel.count
        let row = categoryRow
        for (let step = 0; step < total; ++step) {
            row = (row + (forward ? 1 : -1) + total) % total
            if (launcherData.rootModel.labelForRow(row) !== "") {
                categoryRow = row
                break
            }
        }
        grid.positionViewAtBeginning()
        Qt.callLater(launcher.resetSelection)
    }

    Flickable {
        Layout.fillWidth: true
        Layout.preferredHeight: chips.implicitHeight
        contentWidth: chips.implicitWidth
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        QQC2.ScrollBar.horizontal: QQC2.ScrollBar { policy: QQC2.ScrollBar.AsNeeded }

        RowLayout {
            id: chips
            spacing: Kirigami.Units.smallSpacing
            Repeater {
                model: launcherData.rootModel
                delegate: MouseArea {
                    id: chip
                    required property int index
                    required property var model
                    readonly property bool current: page.categoryRow === index
                    visible: (model.display || "") !== ""
                    implicitWidth: chipLabel.implicitWidth + Kirigami.Units.largeSpacing * 2
                    implicitHeight: chipLabel.implicitHeight + Kirigami.Units.smallSpacing * 2.5
                    hoverEnabled: true
                    onClicked: {
                        page.categoryRow = index
                        grid.positionViewAtBeginning()
                        Qt.callLater(launcher.resetSelection)
                    }
                    Rectangle {
                        anchors.fill: parent
                        radius: height / 2
                        color: chip.current ? Qt.alpha(Kirigami.Theme.highlightColor, 0.25)
                             : chip.containsMouse ? Qt.alpha(Kirigami.Theme.textColor, 0.1) : Qt.alpha(Kirigami.Theme.textColor, 0.05)
                        border.width: chip.current ? 1 : 0
                        border.color: Qt.alpha(Kirigami.Theme.highlightColor, 0.6)
                        Behavior on color { ColorAnimation { duration: 120 } }
                    }
                    PlasmaComponents.Label {
                        id: chipLabel
                        anchors.centerIn: parent
                        text: chip.model.display || ""
                    }
                }
            }
        }
    }

    RowLayout {
        Layout.fillWidth: true
        PlasmaComponents.Label {
            text: page.categoryModel ? i18np("%1 app", "%1 apps", page.categoryModel.count) : ""
            opacity: 0.6
        }
        Item { Layout.fillWidth: true }
        PlasmaComponents.Label {
            text: i18n("Tab switches category")
            font: Kirigami.Theme.smallFont
            opacity: 0.45
        }
    }

    TileGrid {
        id: grid
        Layout.fillWidth: true
        Layout.fillHeight: true
        scrolling: true
        cellWidth: Math.floor(width / Math.max(1, Math.floor(width / (page.tileSize + Kirigami.Units.gridUnit * 3.6))))
        cellHeight: Math.round(page.tileSize + Kirigami.Units.gridUnit * 3.2)
        iconSize: page.tileSize
        model: page.categoryModel
        delegate: KickerTile {}
        QQC2.ScrollBar.vertical: PlasmaComponents.ScrollBar {}
    }
}
