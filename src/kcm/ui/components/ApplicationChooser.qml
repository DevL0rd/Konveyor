import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

QQC2.Popup {
    id: chooser

    property string query
    signal chosen(string exec)

    readonly property var results: kcm.live.applications.filter(app => {
        const words = query.toLowerCase().split(/\s+/).filter(Boolean);
        const haystack = (app.name + " " + app.appId + " " + app.exec).toLowerCase();
        return words.every(word => haystack.includes(word));
    })

    modal: true
    width: Kirigami.Units.gridUnit * 22
    height: Kirigami.Units.gridUnit * 20
    padding: Kirigami.Units.smallSpacing
    onOpened: search.forceActiveFocus()

    contentItem: ColumnLayout {
        spacing: Kirigami.Units.smallSpacing

        Kirigami.SearchField {
            id: search
            Layout.fillWidth: true
            placeholderText: "Search applications…"
            onTextChanged: chooser.query = text
        }

        QQC2.ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ListView {
                clip: true
                model: chooser.results

                delegate: QQC2.ItemDelegate {
                    id: app
                    required property var modelData
                    width: ListView.view.width
                    onClicked: {
                        chooser.chosen(modelData.exec);
                        chooser.close();
                    }

                    contentItem: RowLayout {
                        spacing: Kirigami.Units.largeSpacing

                        Kirigami.Icon {
                            source: app.modelData.icon
                            Layout.preferredWidth: Kirigami.Units.iconSizes.smallMedium
                            Layout.preferredHeight: Kirigami.Units.iconSizes.smallMedium
                        }

                        QQC2.Label {
                            text: app.modelData.name
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }
                }
            }
        }
    }
}
