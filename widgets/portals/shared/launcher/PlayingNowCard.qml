import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.components as Components
import org.kde.plasma.components as PlasmaComponents
import "lib"

Item {
    id: card

    required property int index
    required property var modelData
    readonly property var grid: GridView.view
    readonly property var entry: modelData
    readonly property bool selected: GridView.isCurrentItem && grid.sectionActive

    width: grid.cellWidth
    height: grid.cellHeight

    function entries() {
        const list = []
        if (entry.game)
            list.push({ text: i18n("Play %1", entry.name), icon: "media-playback-start", run: () => launcher.launchGame(entry.game) })
        for (const friend of entry.friends) {
            if (list.length > 0)
                list.push({ separator: true })
            list.push({ text: friend.name, disabled: true })
            if (friend.join)
                list.push({ text: i18n("Join %1", friend.name), icon: "media-playback-start", run: () => launcher.openUrl(friend.join) })
            list.push({ text: i18n("Chat with %1", friend.name), icon: "dialog-messages", run: () => launcher.openUrl(friend.chat) })
        }
        if (entry.appid) {
            list.push({ separator: true })
            list.push({ text: i18n("Store page"), icon: "internet-web-browser", run: () => launcher.openUrl("steam://store/" + entry.appid) })
        }
        return list
    }
    function activate() {
        launcher.openMenu(entries(), card)
    }
    function openMenu() {
        activate()
    }

    Item {
        id: frame
        anchors.fill: parent
        anchors.margins: Kirigami.Units.largeSpacing * 0.75
        scale: card.selected ? 1.03 : mouse.containsMouse ? 1.015 : 1
        Behavior on scale { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }

        Kirigami.ShadowedRectangle {
            anchors.fill: parent
            radius: Kirigami.Units.cornerRadius * 2.5
            color: "transparent"
            shadow.size: card.selected ? Kirigami.Units.gridUnit : Kirigami.Units.largeSpacing
            shadow.color: Qt.rgba(0, 0, 0, card.selected ? 0.5 : 0.3)
            border.width: card.selected ? 2 : 0
            border.color: Qt.alpha(Kirigami.Theme.textColor, 0.9)
        }

        GameArt {
            anchors.fill: parent
            visible: card.entry.game !== null
            game: card.entry.game || ({})
            wide: true
            radius: Kirigami.Units.cornerRadius * 2.5
        }
        Kirigami.ShadowedImage {
            anchors.fill: parent
            visible: card.entry.game === null && status === Image.Ready
            radius: Kirigami.Units.cornerRadius * 2.5
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
            sourceSize.width: Math.round(width * 1.5)
            source: card.entry.game === null ? card.entry.header : ""
        }
        Rectangle {
            anchors.fill: parent
            visible: card.entry.game === null
            z: -1
            radius: Kirigami.Units.cornerRadius * 2.5
            color: Qt.alpha(Kirigami.Theme.textColor, 0.08)
        }
        Rectangle {
            anchors.fill: parent
            radius: Kirigami.Units.cornerRadius * 2.5
            gradient: Gradient {
                GradientStop { position: 0.35; color: "transparent" }
                GradientStop { position: 1; color: Qt.rgba(0, 0, 0, 0.82) }
            }
        }

        RowLayout {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: Kirigami.Units.largeSpacing
            spacing: Kirigami.Units.smallSpacing

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0
                PlasmaComponents.Label {
                    Layout.fillWidth: true
                    text: card.entry.name
                    color: "white"
                    font.weight: Font.DemiBold
                    elide: Text.ElideRight
                }
                PlasmaComponents.Label {
                    Layout.fillWidth: true
                    text: card.entry.friends.length === 1 ? card.entry.friends[0].name : i18np("%1 friend", "%1 friends", card.entry.friends.length)
                    color: "white"
                    opacity: 0.75
                    font: Kirigami.Theme.smallFont
                    elide: Text.ElideRight
                }
            }
            Item {
                Layout.preferredWidth: Kirigami.Units.iconSizes.smallMedium + (Math.min(3, card.entry.friends.length) - 1) * Kirigami.Units.iconSizes.smallMedium * 0.6
                Layout.preferredHeight: Kirigami.Units.iconSizes.smallMedium
                Repeater {
                    model: card.entry.friends.slice(0, 3)
                    Item {
                        required property var modelData
                        required property int index
                        x: index * Kirigami.Units.iconSizes.smallMedium * 0.6
                        z: 3 - index
                        width: Kirigami.Units.iconSizes.smallMedium
                        height: width
                        Rectangle {
                            anchors.fill: parent
                            anchors.margins: -1.5
                            radius: width / 2
                            color: "black"
                            border.width: 1.5
                            border.color: Kirigami.Theme.positiveTextColor
                        }
                        Components.Avatar {
                            anchors.fill: parent
                            source: modelData.avatar || ""
                            name: modelData.name || ""
                        }
                    }
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
        onClicked: card.activate()
    }
}
