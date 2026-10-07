import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.plasma.private.kicker as Kicker

ColumnLayout {
    id: search

    signal picked(string url)

    spacing: Kirigami.Units.smallSpacing

    Kirigami.SearchField {
        id: field
        Layout.fillWidth: true
        placeholderText: i18n("Search for an app to pin…")
    }

    Kicker.RunnerModel {
        id: runner
        runners: ["krunner_services"]
        mergeResults: true
        query: field.text
    }

    ListView {
        id: results
        Layout.fillWidth: true
        Layout.preferredHeight: Math.min(Math.max(contentHeight, count * Kirigami.Units.gridUnit * 2), Kirigami.Units.gridUnit * 10)
        visible: count > 0
        clip: true
        model: runner.count > 0 ? runner.modelForRow(0) : null

        delegate: QQC2.ItemDelegate {
            id: result
            required property var model
            width: ListView.view.width
            text: model.display
            contentItem: RowLayout {
                spacing: Kirigami.Units.smallSpacing

                Kirigami.Icon {
                    source: result.model.decoration
                    Layout.preferredWidth: Kirigami.Units.iconSizes.smallMedium
                    Layout.preferredHeight: Kirigami.Units.iconSizes.smallMedium
                }

                QQC2.Label {
                    text: result.text
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }
            onClicked: {
                search.picked(model.favoriteId || model.url || "")
                field.text = ""
            }
        }
    }
}
