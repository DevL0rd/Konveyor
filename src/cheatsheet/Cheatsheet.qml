pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import org.kde.kirigami as Kirigami
import org.kde.layershell as LayerShell

Window {
    id: root

    readonly property var sections: JSON.parse(Qt.application.arguments[Qt.application.arguments.length - 1])
    readonly property string query: searchField.text.trim().toLowerCase()

    width: Math.min(Screen.width * 0.85, 1800)
    height: Math.min(Screen.height * 0.85, 1100)
    visible: true
    color: "transparent"
    title: "Keyboard Shortcuts"

    LayerShell.Window.layer: LayerShell.Window.LayerOverlay
    LayerShell.Window.keyboardInteractivity: LayerShell.Window.KeyboardInteractivityExclusive
    LayerShell.Window.anchors: LayerShell.Window.AnchorNone
    LayerShell.Window.scope: "plasma-shortcut-cheatsheet"

    function matches(entry, sectionName) {
        if (query === "") {
            return true;
        }
        return entry.action.toLowerCase().includes(query)
            || entry.keys.join(" ").toLowerCase().includes(query)
            || sectionName.toLowerCase().includes(query);
    }

    Rectangle {
        anchors.fill: parent
        radius: Kirigami.Units.cornerRadius * 3
        color: Qt.rgba(Kirigami.Theme.backgroundColor.r, Kirigami.Theme.backgroundColor.g, Kirigami.Theme.backgroundColor.b, 0.96)
        border.color: Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, 0.12)
        border.width: 1

        Kirigami.Theme.colorSet: Kirigami.Theme.View
        Kirigami.Theme.inherit: false

        Keys.onEscapePressed: Qt.quit()

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: Kirigami.Units.gridUnit * 1.5
            spacing: Kirigami.Units.gridUnit

            RowLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.gridUnit

                Kirigami.Heading {
                    text: "Keyboard Shortcuts"
                    level: 1
                }

                Item { Layout.fillWidth: true }

                Kirigami.SearchField {
                    id: searchField
                    Layout.preferredWidth: Kirigami.Units.gridUnit * 18
                    focus: true
                    Keys.onEscapePressed: Qt.quit()
                }
            }

            ScrollView {
                id: scrollView
                Layout.fillWidth: true
                Layout.fillHeight: true
                contentWidth: availableWidth
                clip: true

                Flow {
                    width: scrollView.availableWidth
                    spacing: Kirigami.Units.gridUnit

                    Repeater {
                        model: root.sections

                        delegate: Rectangle {
                            id: card

                            required property var modelData
                            readonly property var visibleEntries: modelData.entries.filter(entry => root.matches(entry, modelData.name))
                            readonly property int columns: Math.max(1, Math.floor((scrollView.availableWidth + Kirigami.Units.gridUnit) / (Kirigami.Units.gridUnit * 26)))

                            visible: visibleEntries.length > 0
                            width: (scrollView.availableWidth - (columns - 1) * Kirigami.Units.gridUnit) / columns
                            height: cardLayout.implicitHeight + Kirigami.Units.gridUnit * 1.5
                            radius: Kirigami.Units.cornerRadius * 2
                            color: Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, 0.05)

                            ColumnLayout {
                                id: cardLayout
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.top: parent.top
                                anchors.margins: Kirigami.Units.gridUnit * 0.75
                                spacing: Kirigami.Units.smallSpacing * 2

                                Kirigami.Heading {
                                    text: card.modelData.name
                                    level: 3
                                    color: Kirigami.Theme.highlightColor
                                    Layout.bottomMargin: Kirigami.Units.smallSpacing
                                }

                                Repeater {
                                    model: card.visibleEntries

                                    delegate: RowLayout {
                                        required property var modelData
                                        Layout.fillWidth: true
                                        spacing: Kirigami.Units.largeSpacing

                                        Label {
                                            text: modelData.action
                                            Layout.fillWidth: true
                                            elide: Text.ElideRight
                                        }

                                        ColumnLayout {
                                            spacing: Kirigami.Units.smallSpacing
                                            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter

                                            Repeater {
                                                model: modelData.keys

                                                delegate: RowLayout {
                                                    required property string modelData
                                                    spacing: Kirigami.Units.smallSpacing
                                                    Layout.alignment: Qt.AlignRight

                                                    Repeater {
                                                        model: modelData.split("+").filter(part => part !== "")

                                                        delegate: Rectangle {
                                                            required property string modelData
                                                            implicitWidth: keyLabel.implicitWidth + Kirigami.Units.smallSpacing * 4
                                                            implicitHeight: keyLabel.implicitHeight + Kirigami.Units.smallSpacing * 2
                                                            radius: Kirigami.Units.cornerRadius
                                                            color: Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, 0.1)
                                                            border.color: Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, 0.2)
                                                            border.width: 1

                                                            Label {
                                                                id: keyLabel
                                                                anchors.centerIn: parent
                                                                text: modelData === "Meta" ? "Super" : modelData
                                                                font.family: "monospace"
                                                                font.pointSize: Kirigami.Theme.smallFont.pointSize
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Label {
                text: "Esc or Super+K to close"
                opacity: 0.6
                Layout.alignment: Qt.AlignHCenter
            }
        }
    }
}
