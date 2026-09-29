import QtQuick
import QtQuick.Window
import org.kde.kirigami as Kirigami
import org.kde.ksvg as KSvg
import org.kde.layershell as LayerShell

Item {
    id: overlay

    readonly property var targetScreen: dim.screen
    readonly property size screenSize: targetScreen ? Qt.size(targetScreen.width, targetScreen.height) : Qt.size(1920, 1080)
    readonly property int cardWidth: Math.round(Math.min(screenSize.width - Kirigami.Units.gridUnit * 6, Kirigami.Units.gridUnit * root.config.cardWidth))
    readonly property int cardHeight: Math.round(Math.min(screenSize.height - Kirigami.Units.gridUnit * 5, Kirigami.Units.gridUnit * root.config.cardHeight))

    Connections {
        target: root
        function onPageRequested(page) {
            view.goToPage(page)
        }
    }

    Window {
        id: dim

        transientParent: null
        visible: false
        color: "transparent"
        flags: Qt.FramelessWindowHint
        title: i18n("Kontrol Panel backdrop")

        LayerShell.Window.scope: "konveyor-kontrol-panel-backdrop"
        LayerShell.Window.layer: LayerShell.Window.LayerTop
        LayerShell.Window.anchors: LayerShell.Window.AnchorTop | LayerShell.Window.AnchorBottom | LayerShell.Window.AnchorLeft | LayerShell.Window.AnchorRight
        LayerShell.Window.exclusionZone: -1
        LayerShell.Window.keyboardInteractivity: LayerShell.Window.KeyboardInteractivityNone
        LayerShell.Window.wantsToBeOnActiveScreen: true

        Rectangle {
            anchors.fill: parent
            color: "black"
            opacity: view.progress * root.config.dimStrength
        }
        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
            onClicked: root.hide()
        }
    }

    Window {
        id: card

        transientParent: null
        visible: false
        color: "transparent"
        flags: Qt.FramelessWindowHint
        title: i18n("Kontrol Panel")
        width: overlay.cardWidth + frame.margins.left + frame.margins.right
        height: overlay.cardHeight + frame.margins.top + frame.margins.bottom

        LayerShell.Window.scope: "konveyor-kontrol-panel"
        LayerShell.Window.layer: LayerShell.Window.LayerOverlay
        LayerShell.Window.anchors: LayerShell.Window.AnchorNone
        LayerShell.Window.exclusionZone: -1
        LayerShell.Window.keyboardInteractivity: LayerShell.Window.KeyboardInteractivityOnDemand
        LayerShell.Window.wantsToBeOnActiveScreen: true

        function updateBlur() {
            if (visible)
                root.service.applyBackgroundEffects(card, frame.mask)
        }
        onVisibleChanged: updateBlur()
        onActiveChanged: {
            if (active)
                view.hadFocus = true
            else if (view.hadFocus && view.shown && root.open && !view.menuOpen)
                root.hide()
        }

        KSvg.FrameSvgItem {
            id: frame
            anchors.fill: parent
            imagePath: "dialogs/background"
            onMaskChanged: card.updateBlur()
        }

        LauncherView {
            id: view
            focus: true
            x: frame.margins.left
            y: frame.margins.top
            width: overlay.cardWidth
            height: overlay.cardHeight
            onActivateRequested: {
                dim.visible = true
                Qt.callLater(function() {
                    card.visible = true
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
