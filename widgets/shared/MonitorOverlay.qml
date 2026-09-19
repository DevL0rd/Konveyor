import QtQuick
import QtQuick.Window
import QtCore
import org.kde.kirigami as Kirigami
import org.kde.plasma.core as PlasmaCore

Item {
    id: overlay

    required property int slot
    required property Component content
    required property Component popupContent
    property bool active: true
    property var targets: []
    readonly property var pids: targets.map(target => target.pid).filter(pid => pid > 0)
    readonly property string statePath: String(StandardPaths.writableLocation(StandardPaths.RuntimeLocation)).replace(/^file:\/\//, "") + "/Konveyor-Monitor-Overlay/state.json"
    readonly property real overlayHeight: Math.round(Kirigami.Units.gridUnit * 1.75)

    function readState() {
        if (!active)
            return
        const xhr = new XMLHttpRequest()
        xhr.open("GET", "file://" + statePath)
        xhr.onreadystatechange = function() {
            if (xhr.readyState !== XMLHttpRequest.DONE || !xhr.responseText)
                return
            try {
                const state = JSON.parse(xhr.responseText)
                overlay.targets = Array.isArray(state.targets) ? state.targets : []
            } catch (error) {
                overlay.targets = []
            }
        }
        xhr.send()
    }

    FileWatcher {
        path: overlay.active ? overlay.statePath : ""
        onChanged: overlay.readState()
    }

    Component.onCompleted: readState()
    onActiveChanged: {
        if (active)
            readState()
        else
            targets = []
    }

    Repeater {
        model: overlay.active ? overlay.targets : []

        delegate: Item {
            id: holder

            required property var modelData
            readonly property var target: modelData
            width: 0
            height: 0

            Timer {
                id: showTimer

                interval: 50
                onTriggered: {
                    if (!Number.isFinite(card.desiredWidth) || card.desiredWidth <= 0)
                        return
                    if (Math.abs(card.desiredWidth - card.preparedWidth) > 0.5) {
                        card.preparedWidth = card.desiredWidth
                        restart()
                        return
                    }
                    card.readyToShow = true
                }
            }

            PlasmaCore.Dialog {
                id: card

                property bool readyToShow: false
                property real preparedWidth: 0
                readonly property real desiredWidth: compactLoader.item
                    ? Math.min(holder.target.width / 3, compactLoader.item.overlayPreferredWidth)
                    : 0

                function prepareToShow() {
                    if (readyToShow || compactLoader.status !== Loader.Ready || !Number.isFinite(desiredWidth) || desiredWidth <= 0)
                        return
                    preparedWidth = desiredWidth
                    showTimer.restart()
                }

                visible: readyToShow
                type: PlasmaCore.Dialog.Normal
                location: PlasmaCore.Types.Floating
                backgroundHints: PlasmaCore.Dialog.NoBackground
                flags: Qt.FramelessWindowHint
                hideOnWindowDeactivate: false
                title: "Konveyor Monitor Overlay " + holder.target.windowId + " " + overlay.slot
                onDesiredWidthChanged: prepareToShow()

                mainItem: Item {
                    id: surface

                    width: card.readyToShow ? card.desiredWidth : card.preparedWidth
                    height: overlay.overlayHeight

                    Loader {
                        id: compactLoader
                        anchors.fill: parent
                        sourceComponent: overlay.content
                        onLoaded: {
                            if (item && "overlayMode" in item)
                                item.overlayMode = true
                            if (item && "overlayTargetPid" in item)
                                item.overlayTargetPid = holder.target.pid
                            card.prepareToShow()
                        }
                    }

                    Binding {
                        target: compactLoader.item
                        property: "overlayBackgroundOpacity"
                        value: overlay.slot === 1 ? (holder.target.fullscreen ? 0.7 : 1) : 0.97
                        when: compactLoader.item !== null && "overlayBackgroundOpacity" in compactLoader.item
                    }

                    Connections {
                        target: compactLoader.item
                        ignoreUnknownSignals: true
                        function onOverlayClicked() { popup.open() }
                    }

                    PlasmaCore.Dialog {
                        id: popup

                        property bool hadFocus: false
                        visible: false
                        type: PlasmaCore.Dialog.AppletPopup
                        location: PlasmaCore.Types.Floating
                        backgroundHints: PlasmaCore.Dialog.StandardBackground
                        flags: Qt.FramelessWindowHint
                        hideOnWindowDeactivate: false
                        title: "Konveyor Monitor Panel " + holder.target.windowId + " " + overlay.slot

                        function open() {
                            visible = true
                            Qt.callLater(function() { popup.requestActivate() })
                        }

                        function close() {
                            visible = false
                        }

                        onActiveChanged: {
                            if (active)
                                hadFocus = true
                            else if (hadFocus && visible)
                                close()
                        }
                        onVisibleChanged: if (!visible) hadFocus = false

                        mainItem: Item {
                            width: popupLoader.item ? popupLoader.item.overlayPreferredWidth : Kirigami.Units.gridUnit * 30
                            height: popupLoader.item ? popupLoader.item.overlayPreferredHeight : Kirigami.Units.gridUnit * 40

                            Loader {
                                id: popupLoader
                                anchors.fill: parent
                                active: popup.visible
                                sourceComponent: overlay.popupContent
                                onLoaded: if (item && "overlayMode" in item) item.overlayMode = true
                            }

                            Connections {
                                target: popupLoader.item
                                ignoreUnknownSignals: true
                                function onOverlayCloseRequested() { popup.close() }
                            }
                        }
                    }
                }
            }
        }
    }
}
