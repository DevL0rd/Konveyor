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
    readonly property var letters: {
        const model = categoryModel
        if (!model)
            return []
        model.count
        const found = []
        for (let i = 0; i < model.count; ++i) {
            const label = String(model.labelForRow(i) || "")
            const first = label.charAt(0).toUpperCase()
            const key = /[A-Z]/.test(first) ? first : "#"
            if (found.length === 0 || found[found.length - 1].key !== key)
                found.push({ key: key, row: i })
        }
        return found
    }

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
    function jump(row) {
        grid.positionViewAtIndex(row, GridView.Beginning)
        launcher.select(grid, row)
    }

    Flickable {
        Layout.fillWidth: true
        Layout.preferredHeight: chips.implicitHeight
        contentWidth: chips.implicitWidth
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        Rectangle {
            width: chips.implicitWidth + Kirigami.Units.smallSpacing
            height: chips.implicitHeight
            radius: height / 2
            color: launcher.well
            border.width: 1
            border.color: launcher.hairline
        }

        RowLayout {
            id: chips
            spacing: 0
            x: Kirigami.Units.smallSpacing / 2
            Repeater {
                model: launcherData.rootModel
                delegate: MouseArea {
                    id: chip
                    required property int index
                    required property var model
                    readonly property bool current: page.categoryRow === index
                    visible: (model.display || "") !== ""
                    implicitWidth: visible ? chipLabel.implicitWidth + Kirigami.Units.largeSpacing * 2 : 0
                    implicitHeight: Kirigami.Units.gridUnit * 1.9
                    hoverEnabled: true
                    onClicked: {
                        page.categoryRow = index
                        grid.positionViewAtBeginning()
                        Qt.callLater(launcher.resetSelection)
                    }
                    Rectangle {
                        anchors.fill: parent
                        anchors.topMargin: Kirigami.Units.smallSpacing / 2
                        anchors.bottomMargin: Kirigami.Units.smallSpacing / 2
                        radius: height / 2
                        color: chip.current ? launcher.selectedFill : chip.containsMouse ? launcher.hoverFill : "transparent"
                        border.width: chip.current ? 1 : 0
                        border.color: launcher.hairline
                    }
                    PlasmaComponents.Label {
                        id: chipLabel
                        anchors.centerIn: parent
                        text: chip.model.display || ""
                        font.weight: chip.current ? Font.DemiBold : Font.Normal
                        opacity: chip.current ? 1 : 0.7
                    }
                }
            }
        }
    }

    RowLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        spacing: Kirigami.Units.largeSpacing

        TileGrid {
            id: grid
            Layout.fillWidth: true
            Layout.fillHeight: true
            scrolling: true
            cellWidth: Math.floor(width / Math.max(1, Math.floor(width / (page.tileSize + Kirigami.Units.gridUnit * 4))))
            cellHeight: Math.round(page.tileSize + Kirigami.Units.gridUnit * 3.4)
            iconSize: page.tileSize
            model: page.categoryModel
            delegate: KickerTile {}
            QQC2.ScrollBar.vertical: PlasmaComponents.ScrollBar {}
        }

        ColumnLayout {
            visible: page.letters.length > 4
            Layout.fillHeight: true
            Layout.fillWidth: false
            Layout.preferredWidth: Kirigami.Units.gridUnit * 1.4
            spacing: 0
            Repeater {
                model: page.letters
                MouseArea {
                    id: letter
                    required property var modelData
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.maximumHeight: Kirigami.Units.gridUnit * 1.3
                    hoverEnabled: true
                    onClicked: page.jump(modelData.row)
                    Rectangle {
                        anchors.centerIn: parent
                        width: Math.min(parent.width, parent.height)
                        height: width
                        radius: width / 2
                        color: letter.containsMouse ? launcher.selectedFill : "transparent"
                    }
                    PlasmaComponents.Label {
                        anchors.centerIn: parent
                        text: letter.modelData.key
                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                        font.weight: Font.DemiBold
                        opacity: letter.containsMouse ? 1 : 0.5
                    }
                }
            }
            Item { Layout.fillHeight: true }
        }
    }
}
