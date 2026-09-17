import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import "../catalog/Actions.js" as Actions

ColumnLayout {
    id: picker

    property string current
    property bool expanded: current.length === 0
    property string query
    property string category: "all"
    signal picked(string actionId)

    readonly property var selected: Actions.byId(current)
    readonly property var results: Actions.search(query).filter(action => category === "all" || action.category === category)
        .sort((left, right) => Actions.categories.findIndex(entry => entry.id === left.category) - Actions.categories.findIndex(entry => entry.id === right.category))

    spacing: Kirigami.Units.smallSpacing

    Rectangle {
        visible: !picker.expanded && picker.selected !== null
        Layout.fillWidth: true
        implicitHeight: chosenRow.implicitHeight + Kirigami.Units.largeSpacing * 2
        radius: Kirigami.Units.cornerRadius * 2
        color: Qt.alpha(Kirigami.Theme.highlightColor, 0.12)
        border.color: Kirigami.Theme.highlightColor

        RowLayout {
            id: chosenRow
            anchors.fill: parent
            anchors.margins: Kirigami.Units.largeSpacing
            spacing: Kirigami.Units.largeSpacing

            Kirigami.Icon {
                source: picker.selected ? picker.selected.icon : ""
                Layout.preferredWidth: Kirigami.Units.iconSizes.medium
                Layout.preferredHeight: Kirigami.Units.iconSizes.medium
            }

            ColumnLayout {
                spacing: 0
                Layout.fillWidth: true

                QQC2.Label {
                    text: picker.selected ? picker.selected.label : ""
                    font.bold: true
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                }

                QQC2.Label {
                    text: picker.selected ? Actions.category(picker.selected.category).label : ""
                    opacity: 0.7
                    font: Kirigami.Theme.smallFont
                }
            }

            QQC2.Button {
                text: "Change…"
                icon.name: "document-edit"
                onClicked: picker.expanded = true
            }
        }
    }

    ColumnLayout {
        visible: picker.expanded
        Layout.fillWidth: true
        spacing: Kirigami.Units.smallSpacing

        Kirigami.SearchField {
            Layout.fillWidth: true
            placeholderText: "Search actions…"
            onTextChanged: picker.query = text
        }

        Flow {
            Layout.fillWidth: true
            spacing: Kirigami.Units.smallSpacing / 2

            Repeater {
                model: [{ id: "all", label: "All", icon: "view-list-icons" }].concat(Actions.categories)

                QQC2.ToolButton {
                    required property var modelData
                    text: modelData.label
                    icon.name: modelData.icon
                    checkable: true
                    checked: picker.category === modelData.id
                    onClicked: picker.category = modelData.id
                }
            }
        }

        QQC2.ScrollView {
            Layout.fillWidth: true
            Layout.preferredHeight: Kirigami.Units.gridUnit * 13

            ListView {
                id: list
                clip: true
                model: picker.results

                delegate: Column {
                    id: entry
                    required property var modelData
                    required property int index
                    readonly property bool firstInCategory: index === 0 || picker.results[index - 1].category !== modelData.category
                    width: ListView.view.width

                    Kirigami.ListSectionHeader {
                        visible: entry.firstInCategory
                        width: parent.width
                        text: Actions.category(entry.modelData.category).label
                    }

                    QQC2.ItemDelegate {
                        width: parent.width
                        highlighted: picker.current === entry.modelData.id
                        onClicked: {
                            picker.picked(entry.modelData.id);
                            picker.expanded = false;
                        }

                        contentItem: RowLayout {
                            spacing: Kirigami.Units.largeSpacing

                            Kirigami.Icon {
                                source: entry.modelData.icon
                                Layout.preferredWidth: Kirigami.Units.iconSizes.smallMedium
                                Layout.preferredHeight: Kirigami.Units.iconSizes.smallMedium
                            }

                            QQC2.Label {
                                text: entry.modelData.label
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }

                            QQC2.Label {
                                visible: entry.modelData.argument !== "none"
                                text: "needs a value"
                                opacity: 0.6
                                font: Kirigami.Theme.smallFont
                            }
                        }
                    }
                }

                Kirigami.PlaceholderMessage {
                    anchors.centerIn: parent
                    width: parent.width - Kirigami.Units.gridUnit * 2
                    visible: list.count === 0
                    text: "No actions match"
                }
            }
        }
    }
}
