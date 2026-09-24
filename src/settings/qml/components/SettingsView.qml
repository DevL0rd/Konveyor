import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.konveyor.settings

Item {
    id: view

    property string pageId: Pages.pages[0].id
    property string reveal: ""
    property string section: ""
    property bool active: true
    readonly property var current: Pages.byId(pageId) || Pages.pages[0]
    readonly property alias rail: rail
    readonly property color ink: Kirigami.Theme.textColor
    property bool justSaved: false
    signal pageChosen(string id)
    signal revealed()
    signal configFileOpened()

    Component.onCompleted: SettingsStore.autoSave = true
    Connections {
        target: SettingsStore
        function onSaved() {
            view.justSaved = true
            savedTimer.restart()
        }
    }
    Timer {
        id: savedTimer
        interval: 1800
        onTriggered: view.justSaved = false
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
                    text: "Settings"
                }
                QQC2.Label {
                    text: "Konveyor"
                    opacity: 0.55
                }
            }
            SettingsRail {
                id: rail
                Layout.fillWidth: true
                entries: Pages.pages
                current: view.current.id
                onChosen: id => view.pageChosen(id)
            }
            Item {
                Layout.fillHeight: true
            }
            QQC2.ToolButton {
                Layout.fillWidth: true
                icon.name: "document-edit-symbolic"
                text: "Open config.kdl"
                onClicked: {
                    SettingsStore.openConfigFile()
                    view.configFileOpened()
                }
            }
        }

        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: 1
            color: Qt.alpha(view.ink, 0.09)
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Kirigami.Units.largeSpacing

            RowLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.largeSpacing
                QQC2.ToolButton {
                    visible: SettingsNavigation.depth > 1
                    icon.name: "go-previous-symbolic"
                    display: QQC2.AbstractButton.IconOnly
                    text: "Back"
                    onClicked: SettingsNavigation.pop()
                }
                Kirigami.Icon {
                    visible: SettingsNavigation.depth <= 1
                    Layout.preferredWidth: Kirigami.Units.iconSizes.smallMedium
                    Layout.preferredHeight: Kirigami.Units.iconSizes.smallMedium
                    source: view.current.icon
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 0
                    Kirigami.Heading {
                        Layout.fillWidth: true
                        level: 3
                        text: stackLoader.item && stackLoader.item.currentItem && stackLoader.item.currentItem.title ? stackLoader.item.currentItem.title : view.current.title
                        elide: Text.ElideRight
                    }
                    QQC2.Label {
                        Layout.fillWidth: true
                        text: view.current.description
                        opacity: 0.55
                        elide: Text.ElideRight
                    }
                }
                QQC2.Label {
                    text: SettingsStore.configError !== "" ? "Not applied: config has an error" : view.justSaved ? "Applied" : "Changes apply instantly"
                    color: SettingsStore.configError !== "" ? Kirigami.Theme.negativeTextColor : Kirigami.Theme.textColor
                    opacity: SettingsStore.configError !== "" ? 1 : view.justSaved ? 0.85 : 0.45
                    font.pointSize: Kirigami.Theme.smallFont.pointSize
                }
                QQC2.ToolButton {
                    icon.name: "edit-undo-symbolic"
                    text: "Undo"
                    enabled: SettingsStore.canUndo
                    onClicked: SettingsStore.undo()
                    QQC2.ToolTip.visible: hovered
                    QQC2.ToolTip.text: "Undo the last change (Ctrl+Z)"
                }
                QQC2.ToolButton {
                    icon.name: "document-revert-symbolic"
                    display: QQC2.AbstractButton.IconOnly
                    text: "Restore defaults"
                    enabled: !SettingsStore.representsDefaults
                    onClicked: SettingsStore.defaults()
                    QQC2.ToolTip.visible: hovered
                    QQC2.ToolTip.text: "Restore every Konveyor setting to its default (Undo brings them back)"
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: Kirigami.Units.cornerRadius * 2
                color: Qt.alpha(view.ink, 0.05)
                border.width: 1
                border.color: Qt.alpha(view.ink, 0.09)
                clip: true

                Loader {
                    id: stackLoader
                    anchors.fill: parent
                    anchors.margins: 1
                    active: view.active
                    sourceComponent: SettingsStack {
                        page: view.current
                        reveal: view.reveal
                        section: view.section
                        onRevealed: view.revealed()
                    }
                }
            }
        }
    }
}
