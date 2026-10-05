import QtQuick
import QtQuick.Layouts
import QtQuick.Window
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.components as PlasmaComponents
import "lib"

LauncherNavigation {
    id: launcher

    readonly property bool wanted: root.open
    property bool warm: false
    Timer {
        id: warmTimer
        interval: Kirigami.Units.longDuration * 2
        onTriggered: launcher.warm = true
    }

    LauncherData {
        id: launcherData
        applet: root
        live: launcher.shown
        query: launcher.term
        searchMode: launcher.mode
    }

    function applyPendingPins() {
        if (root.pendingPins.length === 0)
            return
        for (const path of root.pendingPins) {
            if (!launcherData.favorites.isFavorite(path))
                launcherData.favorites.addFavorite(path)
        }
        root.pendingPins = []
    }
    Connections {
        target: root
        function onPinsRequested() { launcher.applyPendingPins() }
    }
    onWantedChanged: wanted ? openNow() : closeNow()
    Timer {
        interval: 0
        running: true
        onTriggered: {
            launcher.applyPendingPins()
            if (launcher.wanted && !launcher.shown)
                launcher.openNow()
        }
    }

    function openNow() {
        closeAnimation.stop()
        const wantedPage = root.requestedPage || launcherData.config.defaultPage
        root.requestedPage = ""
        page = shownPage(wantedPage)
        markVisited(page)
        openFolder = ""
        field.text = ""
        hadFocus = false
        shown = true
        activateRequested()
        field.forceActiveFocus()
        openAnimation.restart()
        warmTimer.restart()
        Qt.callLater(resetSelection)
    }
    function closeNow() {
        activationPending = false
        hoveredPin = null
        sidebarDrag = null
        railIndex = -1
        openAnimation.stop()
        menu.close()
        closeAnimation.restart()
    }

    ParallelAnimation {
        id: openAnimation
        NumberAnimation { target: launcher; property: "progress"; to: 1; duration: Kirigami.Units.longDuration * 1.4; easing.type: Easing.OutCubic }
        SequentialAnimation {
            PauseAnimation { duration: Kirigami.Units.shortDuration * 0.5 }
            NumberAnimation { target: launcher; property: "contentProgress"; to: 1; duration: Kirigami.Units.longDuration * 1.3; easing.type: Easing.OutCubic }
        }
    }
    SequentialAnimation {
        id: closeAnimation
        ParallelAnimation {
            NumberAnimation { target: launcher; property: "progress"; to: 0; duration: Kirigami.Units.longDuration * 0.8; easing.type: Easing.InCubic }
            NumberAnimation { target: launcher; property: "contentProgress"; to: 0; duration: Kirigami.Units.shortDuration; easing.type: Easing.InCubic }
        }
        ScriptAction {
            script: {
                launcher.shown = false
                field.text = ""
                launcher.closeFinished()
            }
        }
    }

    FocusScope {
        id: content

        anchors.fill: parent
        opacity: launcher.progress
        focus: true

        Keys.forwardTo: [field]

        Rectangle {
            anchors.fill: parent
            radius: Kirigami.Units.cornerRadius * 2
            color: Qt.alpha(Kirigami.Theme.backgroundColor, 0.14)
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: launcher.compact ? Kirigami.Units.smallSpacing * 1.5 : Kirigami.Units.largeSpacing
            spacing: launcher.compact ? Kirigami.Units.largeSpacing : Kirigami.Units.largeSpacing * 1.5
            scale: 0.97 + 0.03 * launcher.progress
            transformOrigin: Item.Top

            Item {
                id: topBar
                Layout.fillWidth: true
                Layout.preferredHeight: Kirigami.Units.gridUnit * (launcher.compact ? 2.2 : 2.6)
                readonly property real gap: Kirigami.Units.largeSpacing * 2
                readonly property real sideWidth: Math.max(Kirigami.Units.gridUnit * 13, statusRow.implicitWidth)

                UserIdentity {
                    id: identity
                    visible: !launcher.compact
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    width: topBar.sideWidth
                }

                Rectangle {
                    id: searchBox
                    anchors.centerIn: launcher.compact ? undefined : parent
                    anchors.verticalCenter: launcher.compact ? parent.verticalCenter : undefined
                    x: 0
                    width: launcher.compact ? parent.width - statusRow.implicitWidth - Kirigami.Units.largeSpacing : Math.max(Kirigami.Units.gridUnit * 16, Math.min(Kirigami.Units.gridUnit * 44, parent.width - (topBar.sideWidth + topBar.gap) * 2))
                    height: parent.height
                    radius: height / 2
                    color: field.activeFocus ? Qt.alpha(launcher.ink, 0.09) : launcher.well
                    border.width: 1
                    border.color: field.activeFocus ? Qt.alpha(launcher.ink, 0.22) : launcher.hairline

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Kirigami.Units.largeSpacing * 1.5
                        anchors.rightMargin: Kirigami.Units.smallSpacing
                        spacing: Kirigami.Units.largeSpacing

                        Kirigami.Icon {
                            Layout.preferredWidth: Kirigami.Units.iconSizes.small
                            Layout.preferredHeight: Kirigami.Units.iconSizes.small
                            source: "search-symbolic"
                            color: launcher.ink
                            isMask: true
                            opacity: 0.6
                        }
                        SearchField {
                            id: field
                        }
                        PlasmaComponents.Label {
                            visible: launcher.searching && searchLoader.item !== null && searchLoader.item.totalResults > 0
                            text: searchLoader.item ? i18np("%1 result", "%1 results", searchLoader.item.totalResults) : ""
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                            opacity: 0.45
                        }
                        Rectangle {
                            visible: launcher.searching && launcher.mode !== "all"
                            implicitWidth: modeLabel.implicitWidth + Kirigami.Units.largeSpacing * 1.5
                            implicitHeight: modeLabel.implicitHeight + Kirigami.Units.smallSpacing
                            radius: height / 2
                            color: Qt.alpha(launcher.ink, 0.12)
                            PlasmaComponents.Label {
                                id: modeLabel
                                anchors.centerIn: parent
                                text: ({ games: i18n("Games"), files: i18n("Files"), apps: i18n("Apps"), packages: i18n("Packages"), friends: i18n("Friends"), calc: i18n("Calculator"), command: i18n("Command") })[launcher.mode] || ""
                                font.pointSize: Kirigami.Theme.smallFont.pointSize
                                font.weight: Font.DemiBold
                            }
                        }
                        PlasmaComponents.ToolButton {
                            visible: field.text !== ""
                            icon.name: "edit-clear-symbolic"
                            display: PlasmaComponents.AbstractButton.IconOnly
                            text: i18n("Clear")
                            onClicked: {
                                field.text = ""
                                field.forceActiveFocus()
                            }
                        }
                    }
                }

                RowLayout {
                    id: statusRow
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: Kirigami.Units.smallSpacing

                    FriendsPill {
                        id: friendsPill
                        Layout.rightMargin: launcher.compact ? 0 : Kirigami.Units.largeSpacing
                    }

                    ColumnLayout {
                        id: clock
                        visible: !launcher.compact && launcherData.config.showTopBarClock
                        spacing: 0
                        Layout.rightMargin: Kirigami.Units.largeSpacing
                        PlasmaComponents.Label {
                            Layout.alignment: Qt.AlignRight
                            text: Qt.formatTime(launcherData.now, Qt.locale().timeFormat(Locale.ShortFormat))
                            font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.1
                            font.weight: Font.DemiBold
                        }
                        PlasmaComponents.Label {
                            Layout.alignment: Qt.AlignRight
                            text: Qt.formatDate(launcherData.now, "ddd d MMM")
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                            opacity: 0.55
                        }
                    }
                    PlasmaComponents.ToolButton {
                        id: settingsButton
                        visible: launcherData.config.showTopBarSettings
                        icon.name: "configure-symbolic"
                        display: PlasmaComponents.AbstractButton.IconOnly
                        text: i18n("System Settings")
                        onClicked: launcher.closeAndRun(() => launcherData.run("systemsettings"))
                        QQC2.ToolTip.visible: hovered
                        QQC2.ToolTip.text: text
                    }
                    PlasmaComponents.ToolButton {
                        id: powerButton
                        visible: !launcher.compact && launcherData.config.showTopBarPower
                        icon.name: "system-shutdown-symbolic"
                        display: PlasmaComponents.AbstractButton.IconOnly
                        text: i18n("Power and session")
                        onClicked: launcher.openMenu(launcher.powerEntries(), powerButton)
                        QQC2.ToolTip.visible: hovered && !menu.visible
                        QQC2.ToolTip.text: text
                    }
                    PlasmaComponents.ToolButton {
                        visible: !launcher.compact
                        icon.name: "window-close-symbolic"
                        display: PlasmaComponents.AbstractButton.IconOnly
                        text: i18n("Close (Esc)")
                        onClicked: root.hide()
                        QQC2.ToolTip.visible: hovered
                        QQC2.ToolTip.text: text
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: Kirigami.Units.largeSpacing * 1.5
                opacity: launcher.contentProgress
                transform: Translate { y: (1 - launcher.contentProgress) * Kirigami.Units.gridUnit * 0.8 }

                ColumnLayout {
                    id: rail
                    z: 2
                    Layout.fillHeight: true
                    Layout.preferredWidth: Kirigami.Units.gridUnit * (launcher.compact ? 3.1 : 3.8)
                    Layout.maximumWidth: Layout.preferredWidth
                    spacing: Kirigami.Units.smallSpacing

                    Repeater {
                        model: launcher.pageDefs.filter(def => def.key !== "settings" && launcher.onSidebar(def.key))
                        delegate: RailButton {}
                    }
                    Rectangle {
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredWidth: rail.width * 0.5
                        Layout.preferredHeight: 1
                        color: launcher.hairline
                        opacity: pinsView.count > 0 || (launcher.sidebarDrag !== null && launcher.sidebarDrag.over) ? 1 : 0
                    }
                    RailPins {
                        id: pinsView
                    }
                    Repeater {
                        model: launcher.pageDefs.filter(def => def.key === "settings" && launcher.onSidebar(def.key))
                        delegate: RailButton {}
                    }
                }

                Rectangle {
                    Layout.fillHeight: true
                    Layout.preferredWidth: 1
                    color: launcher.hairline
                }

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.preferredWidth: 0
                    Layout.minimumWidth: 0

                    Repeater {
                        id: pageLoaders
                        model: launcher.pageDefs
                        delegate: Loader {
                            required property var modelData
                            anchors.fill: parent
                            active: launcher.visited[modelData.key] === true || (launcher.warm && modelData.key === "apps")
                            asynchronous: !(launcher.page === modelData.key && !launcher.searching)
                            visible: !launcher.searching && launcher.page === modelData.key
                            source: Qt.resolvedUrl("pages/" + modelData.key.charAt(0).toUpperCase() + modelData.key.substring(1) + "Page.qml")
                            onLoaded: if (visible) Qt.callLater(launcher.resetSelection)
                        }
                    }

                    Loader {
                        id: searchLoader
                        anchors.fill: parent
                        active: launcher.shown || item !== null
                        asynchronous: true
                        visible: launcher.searching
                        source: Qt.resolvedUrl("pages/SearchPage.qml")
                    }
                }
            }

            Rectangle {
                visible: !launcher.compact
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: launcher.hairline
                opacity: launcher.contentProgress
            }

            KeyHintBar {}
        }

        PinHint {}

        DragGhost {}

        QQC2.Menu {
            id: menu
            popupType: QQC2.Popup.Window

            property var entries: []

            onEntriesChanged: {
                while (count > 0)
                    takeItem(0).destroy()
                for (const entry of entries) {
                    if (entry.separator)
                        addItem(separatorComponent.createObject(null))
                    else
                        addItem(itemComponent.createObject(null, { text: entry.text, "icon.name": entry.icon || "", "icon.source": entry.iconSource || "", enabled: entry.disabled !== true, entry: entry }))
                }
            }
            onClosed: field.forceActiveFocus()
        }

        TouchModeWatch {}
    }

    Component {
        id: itemComponent
        PlasmaComponents.MenuItem {
            property var entry
            onTriggered: if (entry && entry.run) entry.run()
        }
    }
    Component {
        id: separatorComponent
        PlasmaComponents.MenuSeparator {}
    }
}
