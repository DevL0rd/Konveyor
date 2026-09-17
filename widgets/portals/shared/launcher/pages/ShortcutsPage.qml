import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PlasmaComponents
import "../lib"
import "../shortcuts"
import ".."

Item {
    id: page

    property string category: launcherData.shortcutCategory
    property var picked: null
    property string pickedSection: ""
    readonly property alias column: list.column
    readonly property Item contentItem: list.contentItem
    readonly property bool wide: width > Kirigami.Units.gridUnit * 40
    readonly property var chips: {
        const names = [i18n("All")]
        for (const section of launcherData.shortcuts) {
            const name = section.name.startsWith("KDE ") ? i18n("KDE") : section.name
            if (names.indexOf(name) < 0)
                names.push(name)
        }
        return names
    }
    readonly property var shownSections: launcherData.shortcuts.filter(section => category === i18n("All") || section.name === category || (category === i18n("KDE") && section.name.startsWith("KDE ")))
    property var sections: []

    function collectSections() {
        const found = []
        for (let i = 0; i < groups.count; ++i) {
            const group = groups.itemAt(i)
            if (group)
                found.push(group)
        }
        sections = found
        launcher.ensureSelection()
        Qt.callLater(focusPending)
    }
    function focusPending() {
        const target = launcherData.shortcutFocus
        if (target === "")
            return
        for (let i = 0; i < groups.count; ++i) {
            const group = groups.itemAt(i)
            const at = group ? group.section.entries.findIndex(entry => entry.action === target) : -1
            if (at >= 0) {
                launcherData.shortcutFocus = ""
                launcher.select(group, at)
                launcher.ensureVisible(page, group)
                return
            }
        }
    }

    Component.onCompleted: launcherData.refreshShortcuts()
    Connections {
        target: launcher
        function onShownChanged() {
            if (launcher.shown && page.visible)
                launcherData.refreshShortcuts()
        }
    }
    Connections {
        target: launcherData
        function onShortcutsChanged() { Qt.callLater(page.collectSections) }
        function onShortcutFocusChanged() { Qt.callLater(page.focusPending) }
    }
    onShownSectionsChanged: Qt.callLater(collectSections)

    RowLayout {
        anchors.fill: parent
        spacing: Kirigami.Units.largeSpacing * 1.5

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Kirigami.Units.largeSpacing

            RowLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.largeSpacing
                Kirigami.Heading {
                    level: 2
                    text: i18n("Shortcuts")
                }
                PlasmaComponents.Label {
                    Layout.fillWidth: true
                    text: i18np("%1 shortcut", "%1 shortcuts", launcherData.shortcuts.reduce((sum, section) => sum + section.entries.length, 0))
                    opacity: 0.55
                    elide: Text.ElideRight
                }
            }

            SegmentGroup {
                Layout.maximumWidth: parent.width
                Repeater {
                    model: page.chips
                    delegate: Segment {
                        required property string modelData
                        text: modelData
                        current: page.category === modelData
                        onClicked: {
                            launcherData.shortcutCategory = modelData
                            Qt.callLater(launcher.resetSelection)
                        }
                    }
                }
            }

            PopScroll {
                id: list
                Layout.fillWidth: true
                Layout.fillHeight: true

                Repeater {
                    id: groups
                    model: page.shownSections
                    delegate: ShortcutGroup {
                        required property var modelData
                        Layout.fillWidth: true
                        section: modelData
                        onPicked: (entry, sectionName) => {
                            page.picked = entry
                            page.pickedSection = sectionName
                        }
                    }
                    onItemAdded: Qt.callLater(page.collectSections)
                    onItemRemoved: Qt.callLater(page.collectSections)
                }

                PlasmaExtrasPlaceholder {
                    visible: launcherData.shortcuts.length === 0 || launcherData.shortcutsError !== ""
                    text: launcherData.shortcutsError === "" ? i18n("Reading your shortcuts…") : i18n("Couldn't read your shortcuts: %1", launcherData.shortcutsError)
                    opacity: launcherData.shortcutsError === "" ? 0.55 : 1
                    color: launcherData.shortcutsError === "" ? Kirigami.Theme.textColor : Kirigami.Theme.negativeTextColor
                    wrapMode: Text.Wrap
                }
            }
        }

        Rectangle {
            visible: page.wide
            Layout.fillHeight: true
            Layout.preferredWidth: 1
            color: launcher.hairline
        }

        ShortcutDetail {
            visible: page.wide
            Layout.preferredWidth: Kirigami.Units.gridUnit * 15
            Layout.fillHeight: true
            entry: page.picked
            section: page.pickedSection
            animated: launcher.shown && page.visible && !launcher.searching
        }
    }

    component PlasmaExtrasPlaceholder: PlasmaComponents.Label {
        Layout.fillWidth: true
        Layout.topMargin: Kirigami.Units.gridUnit * 4
        horizontalAlignment: Text.AlignHCenter
    }
}
