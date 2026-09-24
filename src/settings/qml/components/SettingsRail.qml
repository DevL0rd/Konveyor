import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

ColumnLayout {
    id: rail

    required property var entries
    required property string current
    property int currentIndex: -1
    property bool sectionActive: false
    readonly property bool scrolling: false
    readonly property int columns: 1
    readonly property int shownCount: entries.length
    readonly property color ink: Kirigami.Theme.textColor
    signal chosen(string id)

    function itemAtIndex(index) {
        return rows.itemAt(index)
    }
    function reset() {
        currentIndex = Math.max(0, entries.findIndex(entry => entry.id === current))
    }
    function enterFrom(fromBelow) {
        currentIndex = fromBelow ? shownCount - 1 : 0
    }
    function move(dx, dy) {
        const next = currentIndex + dy
        if (dy === 0 || next < 0 || next >= shownCount)
            return false
        currentIndex = next
        return true
    }
    function activate() {
        if (currentIndex >= 0 && currentIndex < shownCount)
            chosen(entries[currentIndex].id)
    }
    function openMenu() {
        activate()
    }

    spacing: 2

    Repeater {
        id: rows
        model: rail.entries

        MouseArea {
            id: row
            required property var modelData
            required property int index
            readonly property bool open: rail.current === modelData.id
            readonly property bool keyboard: rail.sectionActive && rail.currentIndex === index && !open
            Layout.fillWidth: true
            implicitHeight: Kirigami.Units.gridUnit * 2.3
            hoverEnabled: true
            onClicked: {
                rail.currentIndex = index
                rail.chosen(modelData.id)
            }

            Rectangle {
                anchors.fill: parent
                radius: Kirigami.Units.cornerRadius * 1.5
                color: row.open ? Qt.alpha(rail.ink, 0.13) : row.containsMouse || row.keyboard ? Qt.alpha(rail.ink, 0.06) : "transparent"
                border.width: row.open || row.keyboard ? 1 : 0
                border.color: row.keyboard ? Qt.alpha(rail.ink, 0.35) : Qt.alpha(rail.ink, 0.09)
            }
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Kirigami.Units.largeSpacing
                anchors.rightMargin: Kirigami.Units.largeSpacing
                spacing: Kirigami.Units.largeSpacing
                Kirigami.Icon {
                    Layout.preferredWidth: Kirigami.Units.iconSizes.small
                    Layout.preferredHeight: Kirigami.Units.iconSizes.small
                    source: row.modelData.icon
                    opacity: row.open ? 1 : 0.75
                }
                QQC2.Label {
                    Layout.fillWidth: true
                    text: row.modelData.title
                    elide: Text.ElideRight
                    font.weight: row.open ? Font.DemiBold : Font.Normal
                    opacity: row.open ? 1 : 0.8
                }
            }
        }
    }
}
