import QtQuick
import org.kde.kirigami as Kirigami
import "lib"

Item {
    id: dragGhost
    readonly property var drag: launcher.sidebarDrag
    readonly property var game: drag ? launcherData.sidebarGame(drag.entry) : null
    visible: drag !== null
    width: launcher.railPinIcon
    height: width
    x: drag ? drag.x - width / 2 : 0
    y: drag ? drag.y - height / 2 : 0
    opacity: drag && drag.removing ? 0.55 : 0.9
    Loader {
        anchors.fill: parent
        active: dragGhost.game !== null && !!dragGhost.game.appid
        sourceComponent: GameArt {
            game: dragGhost.game
            wide: false
            showLogo: false
            radius: Kirigami.Units.cornerRadius
        }
    }
    Kirigami.Icon {
        anchors.fill: parent
        visible: !(dragGhost.game !== null && !!dragGhost.game.appid)
        source: dragGhost.drag ? dragGhost.drag.icon || (dragGhost.drag.entry.kind === "path" ? "folder" : "application-x-executable") : ""
        fallback: "application-x-executable"
    }
    Rectangle {
        visible: dragGhost.drag !== null && (dragGhost.drag.removing || (dragGhost.drag.from < 0 && dragGhost.drag.over))
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: -Kirigami.Units.smallSpacing
        width: Math.round(parent.width * 0.5)
        height: width
        radius: width / 2
        color: dragGhost.drag && dragGhost.drag.removing ? Kirigami.Theme.negativeBackgroundColor : Kirigami.Theme.positiveBackgroundColor
        Kirigami.Icon {
            anchors.fill: parent
            anchors.margins: 2
            source: dragGhost.drag && dragGhost.drag.removing ? "list-remove-symbolic" : "list-add-symbolic"
        }
    }
}
