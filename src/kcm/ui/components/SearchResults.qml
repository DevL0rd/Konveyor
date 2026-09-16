import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import "../catalog/Pages.js" as Pages

ColumnLayout {
    id: root

    property var results: []
    signal opened(string pageId)

    spacing: Kirigami.Units.smallSpacing

    Kirigami.PlaceholderMessage {
        visible: root.results.length === 0
        Layout.fillWidth: true
        icon.name: "search"
        text: "No settings match"
    }

    Repeater {
        model: root.results

        QQC2.ItemDelegate {
            id: result
            required property var modelData
            readonly property var page: Pages.byId(modelData.page)

            Layout.fillWidth: true
            icon.name: page.icon
            onClicked: root.opened(modelData.page)

            contentItem: RowLayout {
                spacing: Kirigami.Units.largeSpacing

                Kirigami.Icon {
                    source: result.page.icon
                    Layout.preferredWidth: Kirigami.Units.iconSizes.smallMedium
                    Layout.preferredHeight: Kirigami.Units.iconSizes.smallMedium
                }

                ColumnLayout {
                    spacing: 0
                    Layout.fillWidth: true

                    QQC2.Label {
                        text: result.modelData.label
                        Layout.fillWidth: true
                    }

                    QQC2.Label {
                        text: result.page.title + (result.modelData.section ? " › " + result.modelData.section : "")
                        opacity: 0.7
                        font: Kirigami.Theme.smallFont
                        Layout.fillWidth: true
                    }
                }
            }
        }
    }
}
