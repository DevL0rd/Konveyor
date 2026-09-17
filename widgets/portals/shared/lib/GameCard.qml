import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.components as Components
import org.kde.plasma.components as PlasmaComponents

Item {
    id: card

    property var game: ({})
    property int friendCount: 0
    property var friends: []
    property bool showTitle: true
    property bool wide: false
    property bool selected: false
    property bool armed: false
    property bool disarmOnExit: true
    property bool lift: true
    readonly property bool hovered: hover.hovered
    readonly property real radius: Kirigami.Units.cornerRadius * 2.5

    signal cardClicked()
    signal launchRequested()
    signal menuRequested()

    onHoveredChanged: if (!hovered && disarmOnExit) armed = false

    scale: lift ? (selected ? 1.035 : hovered ? 1.02 : 1) : 1
    z: selected || hovered ? 10 : 0
    Behavior on scale { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }

    Kirigami.ShadowedRectangle {
        anchors.fill: parent
        radius: card.radius
        color: "transparent"
        shadow.size: card.selected ? Kirigami.Units.gridUnit * 1.2 : Kirigami.Units.largeSpacing
        shadow.yOffset: card.selected ? 4 : 2
        shadow.color: Qt.rgba(0, 0, 0, card.selected ? 0.55 : 0.35)
    }

    GameArt {
        id: artwork
        anchors.fill: parent
        game: card.game
        wide: card.wide
        radius: card.radius
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: Math.min(parent.height, titleLabel.implicitHeight + Kirigami.Units.largeSpacing * 3)
        visible: card.showTitle || card.hovered || card.selected || artwork.mode === "icon"
        radius: card.radius
        gradient: Gradient {
            GradientStop { position: 0; color: "transparent" }
            GradientStop { position: 0.55; color: Qt.rgba(0, 0, 0, 0.55) }
            GradientStop { position: 1; color: Qt.rgba(0, 0, 0, 0.85) }
        }
        PlasmaComponents.Label {
            id: titleLabel
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: Kirigami.Units.largeSpacing
            text: card.game ? (card.game.name || "") : ""
            color: "white"
            font.weight: Font.DemiBold
            horizontalAlignment: card.wide ? Text.AlignLeft : Text.AlignHCenter
            elide: Text.ElideRight
            maximumLineCount: 2
            wrapMode: Text.Wrap
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: card.radius
        color: "transparent"
        border.width: card.selected ? 2 : 1
        border.color: card.selected ? Qt.rgba(1, 1, 1, 0.92)
                    : card.hovered ? Qt.rgba(1, 1, 1, 0.35) : Qt.rgba(1, 1, 1, 0.08)
    }

    Rectangle {
        visible: card.friendCount > 0
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: Kirigami.Units.smallSpacing * 1.5
        implicitWidth: friendsRow.implicitWidth + Kirigami.Units.smallSpacing * 2.5
        implicitHeight: Math.round(Kirigami.Units.gridUnit * 1.25)
        radius: height / 2
        color: Qt.rgba(0, 0, 0, 0.72)
        border.width: 1
        border.color: Qt.rgba(1, 1, 1, 0.14)
        RowLayout {
            id: friendsRow
            anchors.centerIn: parent
            spacing: Kirigami.Units.smallSpacing
            Rectangle {
                Layout.preferredWidth: Math.round(Kirigami.Units.smallSpacing * 1.6)
                Layout.preferredHeight: Layout.preferredWidth
                radius: width / 2
                color: Kirigami.Theme.positiveTextColor
            }
            Repeater {
                model: card.friends.slice(0, 3)
                Components.Avatar {
                    required property var modelData
                    Layout.preferredWidth: Math.round(Kirigami.Units.gridUnit * 0.9)
                    Layout.preferredHeight: Layout.preferredWidth
                    Layout.leftMargin: -Kirigami.Units.smallSpacing
                    source: modelData.avatar || ""
                    name: modelData.name || ""
                }
            }
            PlasmaComponents.Label {
                text: card.friendCount + ""
                color: "white"
                font.weight: Font.Bold
                font.pointSize: Kirigami.Theme.smallFont.pointSize
            }
        }
    }

    Rectangle {
        anchors.centerIn: parent
        visible: card.armed
        width: playRow.implicitWidth + Kirigami.Units.largeSpacing * 3
        height: playRow.implicitHeight + Kirigami.Units.largeSpacing * 1.4
        radius: height / 2
        color: playTap.pressed ? Qt.rgba(1, 1, 1, 0.78) : Qt.rgba(1, 1, 1, 0.94)
        RowLayout {
            id: playRow
            anchors.centerIn: parent
            spacing: Kirigami.Units.smallSpacing
            Kirigami.Icon {
                Layout.preferredWidth: Kirigami.Units.iconSizes.small
                Layout.preferredHeight: Layout.preferredWidth
                source: "media-playback-start"
                color: "black"
                isMask: true
            }
            PlasmaComponents.Label {
                text: i18n("Play")
                color: "black"
                font.weight: Font.Bold
            }
        }
        TapHandler { id: playTap; onTapped: card.launchRequested() }
    }

    HoverHandler { id: hover }
    TapHandler { acceptedButtons: Qt.LeftButton; acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad | PointerDevice.Stylus | PointerDevice.Airbrush | PointerDevice.Puck; onTapped: card.cardClicked() }
    TapHandler { acceptedDevices: PointerDevice.TouchScreen; onTapped: card.cardClicked(); onLongPressed: card.menuRequested() }
    TapHandler { acceptedButtons: Qt.RightButton; acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad | PointerDevice.Stylus | PointerDevice.Airbrush | PointerDevice.Puck; onTapped: card.menuRequested() }
}
