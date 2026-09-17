import QtQuick
import org.kde.kirigami as Kirigami

Item {
    id: art

    property var game: ({})
    property bool wide: false
    property real radius: Kirigami.Units.cornerRadius * 2
    property bool showLogo: true

    function fileUrl(path) {
        return path ? "file://" + encodeURI(path) : ""
    }

    readonly property bool hasPortrait: !!(game && game.portrait)
    readonly property bool hasHero: !!(game && game.hero)
    readonly property bool hasHeader: !!(game && game.header)
    readonly property bool hasLogo: !!(game && game.logo)
    readonly property string mode: wide ? (hasHeader ? "header" : hasHero ? "hero" : hasPortrait ? "portraitWide" : "icon")
                                        : (hasPortrait ? "portrait" : hasHero ? "hero" : hasHeader ? "headerTall" : "icon")
    readonly property bool artReady: mode === "icon" || image.status === Image.Ready

    Rectangle {
        anchors.fill: parent
        radius: art.radius
        gradient: Gradient {
            GradientStop { position: 0; color: Qt.lighter(Kirigami.Theme.backgroundColor, 1.35) }
            GradientStop { position: 1; color: Qt.darker(Kirigami.Theme.backgroundColor, 1.25) }
        }
    }

    Kirigami.ShadowedImage {
        id: image
        anchors.fill: parent
        visible: art.mode !== "icon" && status === Image.Ready
        radius: art.radius
        asynchronous: true
        fillMode: Image.PreserveAspectCrop
        sourceSize.width: Math.round(width * 1.5)
        sourceSize.height: Math.round(height * 1.5)
        source: art.mode === "portrait" ? art.fileUrl(art.game.portrait)
              : art.mode === "hero" ? art.fileUrl(art.game.hero)
              : art.mode === "header" || art.mode === "headerTall" ? art.fileUrl(art.game.header)
              : art.mode === "portraitWide" ? art.fileUrl(art.game.portrait)
              : ""
    }

    Rectangle {
        anchors.fill: parent
        visible: image.visible && (art.mode === "hero" || art.mode === "headerTall" || art.mode === "portraitWide")
        radius: art.radius
        gradient: Gradient {
            GradientStop { position: 0; color: Qt.rgba(0, 0, 0, 0.05) }
            GradientStop { position: 1; color: Qt.rgba(0, 0, 0, 0.55) }
        }
    }

    Image {
        anchors.centerIn: parent
        width: parent.width * (art.wide ? 0.62 : 0.78)
        height: parent.height * (art.wide ? 0.62 : 0.42)
        visible: art.showLogo && art.hasLogo && image.visible && (art.mode === "hero" || art.mode === "portraitWide")
        source: visible ? art.fileUrl(art.game.logo) : ""
        fillMode: Image.PreserveAspectFit
        asynchronous: true
        sourceSize.width: Math.round(width * 1.5)
    }

    Kirigami.Icon {
        anchors.centerIn: parent
        width: Math.round(Math.min(parent.width, parent.height) * (art.wide ? 0.42 : 0.36))
        height: width
        visible: art.mode === "icon" || !image.visible
        source: art.game ? (art.game.icon || "applications-games") : "applications-games"
        fallback: "applications-games"
    }
}
