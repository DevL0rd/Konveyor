import QtQuick
import Qt5Compat.GraphicalEffects
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PlasmaComponents

Item {
    id: card
    property var game: ({})
    property int friendCount: 0
    property bool hovered: hover.hovered
    property bool showTitle: true
    property bool armed: false
    property bool disarmOnExit: true
    onHoveredChanged: if (!hovered && disarmOnExit) armed = false

    signal cardClicked()
    signal launchRequested()
    signal menuRequested()

    function fileUrl(p) { return p ? "file://" + encodeURI(p) : "" }
    readonly property bool hasPortrait: game && game.portrait
    readonly property bool hasHero: game && game.hero
    readonly property bool hasLogo: game && game.logo

    scale: hovered ? 1.05 : 1.0
    z: hovered ? 10 : 0
    Behavior on scale { NumberAnimation { duration: 130; easing.type: Easing.OutCubic } }

    RectangularGlow {
        anchors.fill: frame
        visible: card.friendCount > 0
        z: -1
        glowRadius: Kirigami.Units.largeSpacing
        spread: 0.25
        color: "#43a047"
        cornerRadius: frame.radius + glowRadius
    }

    Rectangle {
        id: frame
        anchors.fill: parent
        radius: Kirigami.Units.cornerRadius * 2
        color: Kirigami.Theme.backgroundColor
        clip: true
        border.width: card.hovered ? 2 : 0
        border.color: Kirigami.Theme.highlightColor

        Image {
            anchors.fill: parent
            visible: card.hasPortrait
            source: card.hasPortrait ? card.fileUrl(card.game.portrait) : ""
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
            cache: true
        }

        Item {
            anchors.fill: parent
            visible: !card.hasPortrait && card.hasHero
            Image {
                anchors.fill: parent
                source: card.hasHero ? card.fileUrl(card.game.hero) : ""
                fillMode: Image.PreserveAspectCrop
                asynchronous: true; cache: true
            }
            Rectangle {
                anchors.fill: parent
                gradient: Gradient {
                    GradientStop { position: 0.0; color: Qt.rgba(0, 0, 0, 0.15) }
                    GradientStop { position: 1.0; color: Qt.rgba(0, 0, 0, 0.65) }
                }
            }
            Image {
                anchors.centerIn: parent
                width: parent.width * 0.8
                source: card.hasLogo ? card.fileUrl(card.game.logo) : ""
                fillMode: Image.PreserveAspectFit
                asynchronous: true; cache: true
                visible: card.hasLogo
            }
        }

        Item {
            anchors.fill: parent
            visible: !card.hasPortrait && !card.hasHero
            Rectangle {
                anchors.fill: parent
                gradient: Gradient {
                    GradientStop { position: 0.0; color: Qt.darker(Kirigami.Theme.backgroundColor, 1.1) }
                    GradientStop { position: 1.0; color: Qt.darker(Kirigami.Theme.backgroundColor, 1.5) }
                }
            }
            Kirigami.Icon {
                anchors.centerIn: parent
                width: Math.min(parent.width, parent.height) * 0.5
                height: width
                source: card.hasLogo ? card.fileUrl(card.game.logo) : (card.game.icon || "applications-games")
            }
        }

        Rectangle {
            anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
            height: nameLabel.implicitHeight + Kirigami.Units.smallSpacing * 2
            visible: card.showTitle || card.hovered
            gradient: Gradient {
                GradientStop { position: 0.0; color: "transparent" }
                GradientStop { position: 1.0; color: Qt.rgba(0, 0, 0, 0.8) }
            }
            PlasmaComponents.Label {
                id: nameLabel
                anchors.fill: parent
                anchors.margins: Kirigami.Units.smallSpacing
                text: card.game ? (card.game.name || "") : ""
                color: "white"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
                maximumLineCount: 2
                wrapMode: Text.Wrap
                font.weight: Font.DemiBold
            }
        }
    }

    Rectangle {
        id: friendBadge
        anchors.top: frame.top; anchors.right: frame.right
        anchors.margins: Kirigami.Units.smallSpacing
        visible: card.friendCount > 0
        z: 20
        height: Math.round(Kirigami.Units.gridUnit * 1.1)
        width: badgeRow.implicitWidth + Kirigami.Units.smallSpacing * 1.5
        radius: height / 2
        color: "#43a047"
        border.width: 1
        border.color: Qt.rgba(0, 0, 0, 0.35)
        Row {
            id: badgeRow
            anchors.centerIn: parent
            spacing: 1
            Kirigami.Icon {
                anchors.verticalCenter: parent.verticalCenter
                width: Math.round(friendBadge.height * 0.7); height: width
                source: "im-user"; color: "white"
            }
            PlasmaComponents.Label {
                anchors.verticalCenter: parent.verticalCenter
                text: card.friendCount
                color: "white"; font.weight: Font.Bold
                font.pixelSize: Math.round(friendBadge.height * 0.6)
            }
        }
    }

    Rectangle {
        anchors.centerIn: parent
        visible: card.armed
        width: playRow.implicitWidth + Kirigami.Units.largeSpacing * 4
        height: playRow.implicitHeight + Kirigami.Units.largeSpacing * 1.6
        radius: Kirigami.Units.smallSpacing
        color: playTap.pressed ? "#2e7d32" : "#43a047"
        scale: playTap.pressed ? 0.95 : 1.0
        Behavior on scale { NumberAnimation { duration: 80 } }
        Row {
            id: playRow
            anchors.centerIn: parent
            spacing: Kirigami.Units.smallSpacing
            Kirigami.Icon {
                anchors.verticalCenter: parent.verticalCenter
                width: Kirigami.Units.iconSizes.smallMedium; height: width
                source: "media-playback-start"; color: "white"
            }
            PlasmaComponents.Label {
                anchors.verticalCenter: parent.verticalCenter
                text: i18n("Play"); color: "white"; font.weight: Font.Bold
            }
        }
        TapHandler { id: playTap; onTapped: card.launchRequested() }
    }

    HoverHandler { id: hover }
    TapHandler { acceptedButtons: Qt.LeftButton; onTapped: card.cardClicked() }
    TapHandler { acceptedButtons: Qt.RightButton; onTapped: card.menuRequested() }
}
