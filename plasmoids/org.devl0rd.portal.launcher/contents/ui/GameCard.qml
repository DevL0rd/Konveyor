import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.components as Components
import org.kde.plasma.components as PlasmaComponents

Item {
    id: card

    required property int index
    required property var modelData
    readonly property var grid: GridView.view
    readonly property var game: modelData
    readonly property bool wide: grid.wideCards === true
    readonly property var playing: launcherData.friendsFor(game)
    readonly property bool selected: GridView.isCurrentItem && grid.sectionActive
    readonly property string art: wide ? (game.header || game.hero || "") : (game.portrait || "")

    width: grid.cellWidth
    height: grid.cellHeight

    function activate() {
        launcherData.launchGame(game)
        root.hide()
    }
    function openMenu() {
        launcher.openMenu(launcher.gameEntries(game), card)
    }

    Item {
        id: frame
        anchors.fill: parent
        anchors.margins: Kirigami.Units.smallSpacing * 1.5
        scale: card.selected ? 1.04 : mouse.containsMouse ? 1.02 : 1
        Behavior on scale { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }

        Kirigami.ShadowedRectangle {
            anchors.fill: parent
            radius: Kirigami.Units.cornerRadius * 2.5
            color: Qt.alpha(Kirigami.Theme.textColor, 0.06)
            shadow.size: card.selected ? Kirigami.Units.gridUnit : Kirigami.Units.smallSpacing * 2
            shadow.color: card.playing.length > 0 ? Qt.alpha(Kirigami.Theme.positiveTextColor, 0.55) : Qt.rgba(0, 0, 0, 0.35)
            border.width: card.selected ? 2 : card.playing.length > 0 ? 1.5 : 0
            border.color: card.selected ? Kirigami.Theme.highlightColor : Kirigami.Theme.positiveTextColor
        }

        Kirigami.ShadowedImage {
            anchors.fill: parent
            visible: card.art !== "" && status === Image.Ready
            radius: Kirigami.Units.cornerRadius * 2.5
            source: card.art !== "" ? "file://" + card.art : ""
            asynchronous: true
            fillMode: Image.PreserveAspectCrop
            sourceSize.width: width * 1.5
            sourceSize.height: height * 1.5
        }

        ColumnLayout {
            anchors.centerIn: parent
            width: parent.width - Kirigami.Units.largeSpacing * 2
            visible: card.art === ""
            spacing: Kirigami.Units.smallSpacing
            Kirigami.Icon {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: Kirigami.Units.iconSizes.huge
                Layout.preferredHeight: Kirigami.Units.iconSizes.huge
                source: card.game.icon || "applications-games"
                fallback: "applications-games"
            }
            PlasmaComponents.Label {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                text: card.game.name
                wrapMode: Text.Wrap
                maximumLineCount: 3
                elide: Text.ElideRight
            }
        }

        Rectangle {
            visible: card.art !== "" && (card.selected || mouse.containsMouse)
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: nameLabel.implicitHeight + Kirigami.Units.largeSpacing * 2
            radius: Kirigami.Units.cornerRadius * 2.5
            gradient: Gradient {
                GradientStop { position: 0; color: "transparent" }
                GradientStop { position: 1; color: Qt.rgba(0, 0, 0, 0.8) }
            }
            PlasmaComponents.Label {
                id: nameLabel
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: Kirigami.Units.largeSpacing
                text: card.game.name
                color: "white"
                font.weight: Font.DemiBold
                elide: Text.ElideRight
            }
        }

        Rectangle {
            visible: card.playing.length > 0
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.margins: Kirigami.Units.smallSpacing * 1.5
            implicitWidth: friendsRow.implicitWidth + Kirigami.Units.smallSpacing * 2
            implicitHeight: friendsRow.implicitHeight + Kirigami.Units.smallSpacing
            radius: height / 2
            color: Qt.rgba(0, 0, 0, 0.7)
            RowLayout {
                id: friendsRow
                anchors.centerIn: parent
                spacing: -Kirigami.Units.smallSpacing
                Repeater {
                    model: card.playing.slice(0, 3)
                    Components.Avatar {
                        required property var modelData
                        Layout.preferredWidth: Kirigami.Units.iconSizes.small
                        Layout.preferredHeight: Kirigami.Units.iconSizes.small
                        source: modelData.avatar || ""
                        name: modelData.name || ""
                    }
                }
                PlasmaComponents.Label {
                    Layout.leftMargin: Kirigami.Units.smallSpacing * 1.5
                    text: card.playing.length + ""
                    color: Kirigami.Theme.positiveTextColor
                    font.weight: Font.Bold
                    font.pointSize: Kirigami.Theme.smallFont.pointSize
                }
            }
        }
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onEntered: launcher.select(card.grid, card.index)
        onClicked: function(event) {
            launcher.select(card.grid, card.index)
            if (event.button === Qt.RightButton)
                card.openMenu()
            else
                card.activate()
        }
    }
}
