import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PlasmaComponents

Item {
    id: list

    property string title
    property var windows: []

    signal picked(var window)

    implicitWidth: Kirigami.Units.gridUnit * 14
    implicitHeight: rows.implicitHeight

    ColumnLayout {
        id: rows
        width: parent.width
        spacing: 0

        PlasmaComponents.Label {
            text: list.title
            font.weight: Font.DemiBold
            elide: Text.ElideRight
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.smallSpacing
        }

        Repeater {
            model: list.windows

            PlasmaComponents.ItemDelegate {
                id: row
                required property var modelData
                Layout.fillWidth: true
                text: modelData.title
                highlighted: modelData.active
                onClicked: list.picked(modelData)

                contentItem: RowLayout {
                    spacing: Kirigami.Units.smallSpacing

                    Kirigami.Icon {
                        source: row.modelData.icon
                        opacity: row.modelData.minimized ? 0.6 : 1
                        Layout.preferredWidth: Kirigami.Units.iconSizes.small
                        Layout.preferredHeight: Kirigami.Units.iconSizes.small
                    }

                    PlasmaComponents.Label {
                        text: row.text
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }
            }
        }
    }
}
