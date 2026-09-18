import QtQuick
import QtQuick.Window
import org.kde.kirigami as Kirigami
import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import org.kde.layershell as LayerShell

Item {
    id: overlay

    property var targetScreen: null
    readonly property rect screenRect: targetScreen ? Qt.rect(targetScreen.virtualX, targetScreen.virtualY, targetScreen.width, targetScreen.height) : Qt.rect(0, 0, 1920, 1080)
    readonly property int cardWidth: Math.round(Math.min(screenRect.width - Kirigami.Units.gridUnit * 6, Kirigami.Units.gridUnit * Plasmoid.configuration.cardWidth))
    readonly property int cardHeight: Math.round(Math.min(screenRect.height - Kirigami.Units.gridUnit * 5, Kirigami.Units.gridUnit * Plasmoid.configuration.cardHeight))

    function pickScreen() {
        const screens = Qt.application.screens
        if (root.openedByKey && dim.screen)
            return dim.screen
        return screens.find(entry => entry.name === root.openScreen) || screens[0]
    }
    function syncScreen() {
        const screen = pickScreen()
        if (!screen)
            return
        targetScreen = screen
    }
    function placeCard() {
        card.x = Math.round(screenRect.x + (screenRect.width - card.width) / 2)
        card.y = Math.round(screenRect.y + (screenRect.height - card.height) / 2)
    }

    Connections {
        target: root
        function onPageRequested(page) {
            view.goToPage(page)
        }
    }

    Window {
        id: dim

        visible: false
        color: "transparent"
        flags: Qt.FramelessWindowHint
        title: i18n("Kontrol Panel backdrop")

        LayerShell.Window.scope: "portal-launcher-backdrop"
        LayerShell.Window.layer: LayerShell.Window.LayerTop
        LayerShell.Window.anchors: LayerShell.Window.AnchorTop | LayerShell.Window.AnchorBottom | LayerShell.Window.AnchorLeft | LayerShell.Window.AnchorRight
        LayerShell.Window.exclusionZone: -1
        LayerShell.Window.keyboardInteractivity: LayerShell.Window.KeyboardInteractivityNone
        LayerShell.Window.wantsToBeOnActiveScreen: true

        onScreenChanged: if (root.open && root.openedByKey) Qt.callLater(overlay.syncScreen)

        Rectangle {
            anchors.fill: parent
            color: "black"
            opacity: view.progress * Plasmoid.configuration.dimStrength
        }
        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
            onClicked: root.hide()
        }
    }

    PlasmaCore.Dialog {
        id: card

        visible: false
        type: PlasmaCore.Dialog.AppletPopup
        location: PlasmaCore.Types.Floating
        backgroundHints: PlasmaCore.Dialog.StandardBackground
        flags: Qt.FramelessWindowHint
        hideOnWindowDeactivate: false
        title: i18n("Kontrol Panel")

        onWidthChanged: if (visible) overlay.placeCard()
        onHeightChanged: if (visible) overlay.placeCard()
        onActiveChanged: {
            if (active)
                view.hadFocus = true
            else if (view.hadFocus && view.shown && root.open && !view.menuOpen)
                root.hide()
        }

        mainItem: LauncherView {
            id: view
            width: overlay.cardWidth
            height: overlay.cardHeight
            onActivateRequested: {
                dim.visible = true
                Qt.callLater(function() {
                    overlay.syncScreen()
                    card.visible = true
                    overlay.placeCard()
                    card.requestActivate()
                })
            }
            onPageChanged: root.currentPage = page
            onCloseFinished: {
                card.visible = false
                dim.visible = false
            }
        }
    }
}
