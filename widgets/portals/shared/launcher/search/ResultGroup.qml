import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import ".."

ColumnLayout {
    id: group
    property string title
    property int rows: resultGrid.count
    property string trailing: rows + ""
    property alias grid: resultGrid
    property alias model: resultGrid.model
    property alias delegate: resultGrid.delegate
    property alias cellHeight: resultGrid.cellHeight
    property alias cellWidth: resultGrid.cellWidth
    readonly property bool hasContent: rows > 0
    visible: hasContent
    Layout.fillWidth: true
    spacing: Kirigami.Units.smallSpacing
    SectionHeader {
        title: group.title
        trailing: group.trailing
    }
    TileGrid {
        id: resultGrid
        visible: count > 0
        limit: page.mode === "all" ? 6 : -1
        Layout.fillWidth: true
        Layout.preferredHeight: implicitHeight
        cellWidth: Math.floor(width / Math.max(1, Math.floor(width / page.rowWidth)))
        cellHeight: Kirigami.Units.gridUnit * 3.2
        onCountChanged: Qt.callLater(page.rebuildSections)
    }
}
