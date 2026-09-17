import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PlasmaComponents
import org.kde.konveyor.settings
import "../settings"
import ".."

Item {
    id: settingsPage

    readonly property var current: Pages.byId(launcherData.settingsTarget.page) || Pages.pages[0]
    readonly property var sections: [rail]
    readonly property bool live: launcher.shown && visible
    property bool justSaved: false

    function undo() {
        SettingsStore.undo()
    }
    function openPage(id) {
        launcherData.settingsTarget = { page: id, section: "", label: "" }
    }

    Component.onCompleted: SettingsStore.autoSave = true
    Connections {
        target: SettingsStore
        function onSaved() {
            settingsPage.justSaved = true
            savedTimer.restart()
        }
    }
    Timer {
        id: savedTimer
        interval: 1800
        onTriggered: settingsPage.justSaved = false
    }

    RowLayout {
        anchors.fill: parent
        spacing: Kirigami.Units.largeSpacing * 1.5

        ColumnLayout {
            Layout.preferredWidth: Kirigami.Units.gridUnit * 12
            Layout.maximumWidth: Layout.preferredWidth
            Layout.fillHeight: true
            spacing: Kirigami.Units.largeSpacing

            ColumnLayout {
                spacing: 0
                Kirigami.Heading {
                    level: 2
                    text: i18n("Settings")
                }
                PlasmaComponents.Label {
                    text: i18n("Konveyor")
                    opacity: 0.55
                }
            }
            SettingsRail {
                id: rail
                Layout.fillWidth: true
                entries: Pages.pages
                current: settingsPage.current.id
                onChosen: id => settingsPage.openPage(id)
            }
            Item {
                Layout.fillHeight: true
            }
            PlasmaComponents.ToolButton {
                Layout.fillWidth: true
                icon.name: "document-edit-symbolic"
                text: i18n("Open config.kdl")
                onClicked: {
                    SettingsStore.openConfigFile()
                    root.hide()
                }
            }
        }

        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: 1
            color: launcher.hairline
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Kirigami.Units.largeSpacing

            RowLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.largeSpacing
                PlasmaComponents.ToolButton {
                    visible: SettingsNavigation.depth > 1
                    icon.name: "go-previous-symbolic"
                    display: PlasmaComponents.AbstractButton.IconOnly
                    text: i18n("Back")
                    onClicked: SettingsNavigation.pop()
                }
                Kirigami.Icon {
                    visible: SettingsNavigation.depth <= 1
                    Layout.preferredWidth: Kirigami.Units.iconSizes.smallMedium
                    Layout.preferredHeight: Kirigami.Units.iconSizes.smallMedium
                    source: settingsPage.current.icon
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 0
                    Kirigami.Heading {
                        Layout.fillWidth: true
                        level: 3
                        text: stackLoader.item && stackLoader.item.currentItem && stackLoader.item.currentItem.title ? stackLoader.item.currentItem.title : settingsPage.current.title
                        elide: Text.ElideRight
                    }
                    PlasmaComponents.Label {
                        Layout.fillWidth: true
                        text: settingsPage.current.description
                        opacity: 0.55
                        elide: Text.ElideRight
                    }
                }
                PlasmaComponents.Label {
                    text: SettingsStore.configError !== "" ? i18n("Not applied: config has an error") : settingsPage.justSaved ? i18n("Applied") : i18n("Changes apply instantly")
                    color: SettingsStore.configError !== "" ? Kirigami.Theme.negativeTextColor : Kirigami.Theme.textColor
                    opacity: SettingsStore.configError !== "" ? 1 : settingsPage.justSaved ? 0.85 : 0.45
                    font.pointSize: Kirigami.Theme.smallFont.pointSize
                }
                PlasmaComponents.ToolButton {
                    icon.name: "edit-undo-symbolic"
                    text: i18n("Undo")
                    enabled: SettingsStore.canUndo
                    onClicked: SettingsStore.undo()
                    QQC2.ToolTip.visible: hovered
                    QQC2.ToolTip.text: i18n("Undo the last change (Ctrl+Z)")
                }
                PlasmaComponents.ToolButton {
                    icon.name: "document-revert-symbolic"
                    display: PlasmaComponents.AbstractButton.IconOnly
                    text: i18n("Restore defaults")
                    enabled: !SettingsStore.representsDefaults
                    onClicked: SettingsStore.defaults()
                    QQC2.ToolTip.visible: hovered
                    QQC2.ToolTip.text: i18n("Restore every Konveyor setting to its default (Undo brings them back)")
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: Kirigami.Units.cornerRadius * 2
                color: launcher.well
                border.width: 1
                border.color: launcher.hairline
                clip: true

                Loader {
                    id: stackLoader
                    anchors.fill: parent
                    anchors.margins: 1
                    active: settingsPage.live
                    sourceComponent: SettingsStack {
                        page: settingsPage.current
                        reveal: launcherData.settingsTarget.label || ""
                        section: launcherData.settingsTarget.section
                        onRevealed: launcherData.settingsTarget = { page: settingsPage.current.id, section: "", label: "" }
                    }
                }
            }
        }
    }
}
