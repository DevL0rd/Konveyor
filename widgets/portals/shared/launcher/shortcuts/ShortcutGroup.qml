import QtQuick
import QtQuick.Layouts
import ".."

ColumnLayout {
    id: group

    required property var section
    property string query
    property int currentIndex: -1
    property bool sectionActive: false
    readonly property bool scrolling: false
    readonly property int columns: 1
    readonly property int shownCount: rows.count
    function entryAt(index) {
        return index >= 0 && index < section.entries.length ? section.entries[index] : null
    }
    signal picked(var entry, string sectionName)

    function itemAtIndex(index) {
        return rows.itemAt(index)
    }
    function reset() {
        currentIndex = shownCount > 0 ? 0 : -1
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
        if (entryAt(currentIndex))
            picked(entryAt(currentIndex), section.name)
    }
    function openMenu() {
        activate()
    }
    onCurrentIndexChanged: if (entryAt(currentIndex)) picked(entryAt(currentIndex), section.name)

    spacing: 2

    SectionHeader {
        Layout.fillWidth: true
        title: group.section.name
        trailing: group.section.entries.length + ""
    }

    Repeater {
        id: rows
        model: group.section.entries

        ShortcutRow {
            required property var modelData
            required property int index
            Layout.fillWidth: true
            entry: modelData
            query: group.query
            selected: group.sectionActive && group.currentIndex === index
            onHovered: launcher.select(group, index)
            onClicked: launcher.select(group, index)
        }
    }
}
