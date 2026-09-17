import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import Qt5Compat.GraphicalEffects
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PlasmaComponents
import "lib"

PopCard {
    id: card

    signal gameClicked(string game)

    title: i18n("Playing now")
    icon: "input-gamepad"
    trailing: i18np("%1 game", "%1 games", root.playingNow.length)
    trailingColor: root.cInGame

    readonly property real tileWidth: Kirigami.Units.gridUnit * 9.5
    readonly property real artHeight: Math.round(tileWidth * 69 / 184)
    readonly property real avatarSize: Math.round(Kirigami.Units.gridUnit * 1.3)

    ListView {
        id: strip
        Layout.fillWidth: true
        Layout.preferredHeight: card.artHeight + card.avatarSize + Kirigami.Units.gridUnit * 1.9
        orientation: ListView.Horizontal
        spacing: Kirigami.Units.largeSpacing
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        model: root.playingNow
        QQC2.ScrollBar.horizontal: QQC2.ScrollBar { policy: QQC2.ScrollBar.AlwaysOff }

        delegate: Item {
            id: tile
            required property var modelData
            width: card.tileWidth
            height: strip.height

            Rectangle {
                id: art
                width: parent.width
                height: card.artHeight
                radius: Kirigami.Units.cornerRadius * 1.5
                color: Qt.alpha(Kirigami.Theme.textColor, 0.08)
                border.width: tileHover.hovered ? 2 : 0
                border.color: root.cInGame
                scale: tileHover.hovered ? 1.03 : 1
                Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
                HoverHandler { id: tileHover; cursorShape: Qt.PointingHandCursor }
                TapHandler {
                    acceptedButtons: Qt.LeftButton
                    onTapped: card.gameClicked(tile.modelData.game)
                }
                QQC2.ToolTip.visible: tileHover.hovered
                QQC2.ToolTip.delay: 600
                QQC2.ToolTip.text: i18n("Show who's playing %1", tile.modelData.game)

                Image {
                    id: capsule
                    anchors.fill: parent
                    source: tile.modelData.capsule
                    fillMode: Image.PreserveAspectCrop
                    asynchronous: true
                    cache: true
                    visible: false
                }
                Rectangle {
                    id: capsuleMask
                    anchors.fill: parent
                    radius: art.radius
                    visible: false
                }
                OpacityMask {
                    anchors.fill: parent
                    anchors.margins: art.border.width
                    source: capsule
                    maskSource: capsuleMask
                    visible: capsule.status === Image.Ready
                }
                Kirigami.Icon {
                    anchors.centerIn: parent
                    width: parent.height * 0.5
                    height: width
                    visible: capsule.status !== Image.Ready
                    source: "applications-games"
                    opacity: 0.6
                }
                Rectangle {
                    anchors.top: parent.top
                    anchors.right: parent.right
                    anchors.margins: Kirigami.Units.smallSpacing
                    height: Math.round(Kirigami.Units.gridUnit * 1.05)
                    width: countRow.implicitWidth + Kirigami.Units.smallSpacing * 2
                    radius: height / 2
                    color: root.cInGame
                    Row {
                        id: countRow
                        anchors.centerIn: parent
                        spacing: 2
                        Kirigami.Icon {
                            anchors.verticalCenter: parent.verticalCenter
                            width: Math.round(parent.parent.height * 0.7)
                            height: width
                            source: "im-user"
                            color: "white"
                        }
                        PlasmaComponents.Label {
                            anchors.verticalCenter: parent.verticalCenter
                            text: tile.modelData.friends.length
                            color: "white"
                            font.weight: Font.Bold
                            font.pixelSize: Math.round(parent.parent.height * 0.62)
                        }
                    }
                }
            }

            PlasmaComponents.Label {
                id: gameName
                anchors.top: art.bottom
                anchors.topMargin: Kirigami.Units.smallSpacing
                width: parent.width
                text: tile.modelData.game
                elide: Text.ElideRight
                font.weight: Font.DemiBold
                font.pointSize: Kirigami.Theme.smallFont.pointSize
            }

            Row {
                anchors.top: gameName.bottom
                anchors.topMargin: 2
                spacing: -Math.round(card.avatarSize * 0.3)
                Repeater {
                    model: tile.modelData.friends.slice(0, 6)
                    delegate: Item {
                        required property var modelData
                        width: card.avatarSize
                        height: width
                        Rectangle {
                            anchors.fill: parent
                            radius: width / 2
                            color: Kirigami.Theme.backgroundColor
                        }
                        Image {
                            id: face
                            anchors.fill: parent
                            anchors.margins: 2
                            source: modelData.avatar || ""
                            sourceSize.width: card.avatarSize * 2
                            sourceSize.height: card.avatarSize * 2
                            fillMode: Image.PreserveAspectCrop
                            asynchronous: true
                            cache: true
                            visible: false
                        }
                        Rectangle {
                            id: faceMask
                            anchors.fill: face
                            radius: width / 2
                            visible: false
                        }
                        OpacityMask {
                            anchors.fill: face
                            source: face
                            maskSource: faceMask
                        }
                        HoverHandler { id: faceHover }
                        TapHandler {
                            acceptedButtons: Qt.LeftButton | Qt.RightButton
                            onTapped: root.menuRequested(modelData)
                        }
                        QQC2.ToolTip.visible: faceHover.hovered
                        QQC2.ToolTip.text: modelData.name
                    }
                }
                PlasmaComponents.Label {
                    visible: tile.modelData.friends.length > 6
                    anchors.verticalCenter: parent.verticalCenter
                    leftPadding: Math.round(card.avatarSize * 0.4)
                    text: "+" + (tile.modelData.friends.length - 6)
                    font.pointSize: Kirigami.Theme.smallFont.pointSize
                    opacity: 0.7
                }
            }

        }
    }
}
