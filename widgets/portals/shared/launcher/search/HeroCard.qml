import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PlasmaComponents
import "../lib"

Item {
    id: heroCard
    property var iconSource
    property string label
    property string subtitle
    property string kind
    property bool selected: false
    property var actions: []
    property var game: null
    signal clicked()
    signal rightClicked()
    signal hovered()

    Rectangle {
        anchors.fill: parent
        anchors.margins: 2
        radius: Kirigami.Units.cornerRadius * 3
        color: heroCard.selected ? launcher.selectedFill : heroMouse.containsMouse ? launcher.hoverFill : launcher.well
        border.width: 1
        border.color: heroCard.selected ? launcher.selectedLine : launcher.hairline
    }
    MouseArea {
        id: heroMouse
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onEntered: if (launcher.pointerMoved(mapToItem(null, mouseX, mouseY))) heroCard.hovered()
        onPressAndHold: function(event) {
            if (launcher.touchMode)
                heroCard.rightClicked()
            else
                event.accepted = false
        }
        onClicked: function(event) {
            if (event.button === Qt.RightButton)
                heroCard.rightClicked()
            else
                heroCard.clicked()
        }
    }
    RowLayout {
        anchors.fill: parent
        anchors.margins: Kirigami.Units.largeSpacing * 1.5
        spacing: Kirigami.Units.largeSpacing * 2
        GameArt {
            visible: heroCard.game !== null
            Layout.fillHeight: true
            Layout.preferredWidth: Math.round(height / 0.4667)
            game: heroCard.game || ({})
            wide: true
            radius: Kirigami.Units.cornerRadius * 2
        }
        Kirigami.Icon {
            visible: heroCard.game === null
            Layout.preferredWidth: Kirigami.Units.iconSizes.huge
            Layout.preferredHeight: Kirigami.Units.iconSizes.huge
            source: heroCard.iconSource
            fallback: "application-x-executable"
        }
        ColumnLayout {
            Layout.fillWidth: true
            spacing: Kirigami.Units.smallSpacing * 0.5
            PlasmaComponents.Label {
                text: heroCard.kind
                font.pointSize: Kirigami.Theme.smallFont.pointSize
                font.weight: Font.DemiBold
                font.capitalization: Font.AllUppercase
                font.letterSpacing: 0.6
                opacity: 0.5
            }
            PlasmaComponents.Label {
                Layout.fillWidth: true
                text: heroCard.label
                font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.6
                font.weight: Font.DemiBold
                elide: Text.ElideRight
            }
            PlasmaComponents.Label {
                Layout.fillWidth: true
                visible: text !== ""
                text: heroCard.subtitle
                opacity: 0.6
                elide: Text.ElideRight
            }
        }
        RowLayout {
            spacing: Kirigami.Units.smallSpacing
            Repeater {
                model: heroCard.actions
                PlasmaComponents.Button {
                    required property var modelData
                    required property int index
                    text: modelData.text
                    icon.name: modelData.icon
                    highlighted: index === 0
                    onClicked: modelData.run()
                }
            }
        }
    }
}
