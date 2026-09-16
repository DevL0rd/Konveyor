import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

GridLayout {
    id: root

    property var options: []
    property var currentValue
    property real cardHeight: Kirigami.Units.gridUnit * 7
    signal chosen(var value)

    columns: Math.max(1, Math.min(options.length, Math.floor(width / (Kirigami.Units.gridUnit * 7))))
    columnSpacing: Kirigami.Units.largeSpacing
    rowSpacing: Kirigami.Units.largeSpacing

    Repeater {
        model: root.options

        QQC2.AbstractButton {
            id: card

            required property var modelData
            readonly property bool selected: root.currentValue === modelData.value

            Layout.fillWidth: true
            Layout.preferredWidth: 1
            Layout.preferredHeight: root.cardHeight
            hoverEnabled: true
            onClicked: root.chosen(modelData.value)

            Accessible.role: Accessible.RadioButton
            Accessible.name: modelData.title
            Accessible.checked: selected

            background: Rectangle {
                radius: Kirigami.Units.cornerRadius * 2
                color: card.selected ? Qt.alpha(Kirigami.Theme.highlightColor, 0.18)
                                     : (card.hovered ? Qt.alpha(Kirigami.Theme.textColor, 0.06) : Qt.alpha(Kirigami.Theme.textColor, 0.03))
                border.width: card.selected ? 2 : 1
                border.color: card.selected ? Kirigami.Theme.highlightColor : Qt.alpha(Kirigami.Theme.textColor, 0.15)

                Behavior on color {
                    ColorAnimation {
                        duration: Kirigami.Units.shortDuration
                    }
                }
            }

            contentItem: ColumnLayout {
                spacing: Kirigami.Units.smallSpacing

                Loader {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.margins: Kirigami.Units.smallSpacing
                    sourceComponent: card.modelData.preview || null
                    property bool selected: card.selected
                    property var choice: card.modelData
                }

                QQC2.Label {
                    text: card.modelData.title
                    font.bold: card.selected
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                QQC2.Label {
                    text: card.modelData.description || ""
                    visible: text.length > 0
                    wrapMode: Text.Wrap
                    horizontalAlignment: Text.AlignHCenter
                    opacity: 0.75
                    font: Kirigami.Theme.smallFont
                    maximumLineCount: 2
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                    Layout.bottomMargin: Kirigami.Units.smallSpacing
                }
            }
        }
    }
}
