import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

QQC2.ItemDelegate {
    id: card

    property string iconName
    property string heading
    property string subtitle
    property string detail
    property bool active
    property bool showOrder: true
    property bool canMoveUp
    property bool canMoveDown
    signal opened
    signal movedUp
    signal movedDown
    signal deleted

    padding: Kirigami.Units.largeSpacing
    onClicked: opened()

    contentItem: RowLayout {
        spacing: Kirigami.Units.largeSpacing

        Item {
            Layout.preferredWidth: Kirigami.Units.iconSizes.large
            Layout.preferredHeight: Kirigami.Units.iconSizes.large

            Kirigami.Icon {
                anchors.fill: parent
                source: card.iconName
            }

            Rectangle {
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                width: Kirigami.Units.gridUnit * 0.7
                height: width
                radius: width / 2
                visible: card.active
                color: Kirigami.Theme.positiveTextColor
                border.color: Kirigami.Theme.backgroundColor
                border.width: 2
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: Kirigami.Units.smallSpacing / 2

            QQC2.Label {
                text: card.heading
                font.bold: true
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            QQC2.Label {
                text: card.subtitle
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }

            QQC2.Label {
                text: card.detail
                visible: text.length > 0
                opacity: 0.7
                font: Kirigami.Theme.smallFont
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
        }

        QQC2.ToolButton {
            visible: card.showOrder
            icon.name: "go-up"
            text: "Move up"
            display: QQC2.AbstractButton.IconOnly
            enabled: card.canMoveUp
            onClicked: card.movedUp()
            QQC2.ToolTip.text: text
            QQC2.ToolTip.visible: hovered
        }

        QQC2.ToolButton {
            visible: card.showOrder
            icon.name: "go-down"
            text: "Move down"
            display: QQC2.AbstractButton.IconOnly
            enabled: card.canMoveDown
            onClicked: card.movedDown()
            QQC2.ToolTip.text: text
            QQC2.ToolTip.visible: hovered
        }

        QQC2.ToolButton {
            icon.name: "edit-delete"
            text: "Delete"
            display: QQC2.AbstractButton.IconOnly
            onClicked: card.deleted()
            QQC2.ToolTip.text: text
            QQC2.ToolTip.visible: hovered
        }

        Kirigami.Icon {
            source: "go-next"
            Layout.preferredWidth: Kirigami.Units.iconSizes.small
            Layout.preferredHeight: Kirigami.Units.iconSizes.small
        }
    }
}
