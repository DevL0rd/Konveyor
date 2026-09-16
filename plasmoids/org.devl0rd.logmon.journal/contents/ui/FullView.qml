import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.plasma.plasmoid
import org.kde.plasma.components as PlasmaComponents
import org.kde.plasma.extras as PlasmaExtras
import "lib"

Item {
    id: full

    Layout.minimumWidth: Kirigami.Units.gridUnit * 13
    Layout.minimumHeight: Kirigami.Units.gridUnit * 12
    Layout.preferredWidth: Kirigami.Units.gridUnit * 36
    Layout.preferredHeight: Kirigami.Units.gridUnit * 38

    Loader {
        id: loader
        anchors.fill: parent
        active: root.popupAlive
        sourceComponent: shellComponent
        onLoaded: if (root.expanded) item.focusSearch()
    }

    Connections {
        target: root
        function onExpandedChanged() {
            if (root.expanded && loader.item)
                loader.item.focusSearch()
        }
    }

    component SourceChip: MouseArea {
        id: chip
        property string text
        property string count
        property string iconName
        property bool active: false
        implicitWidth: chipRow.implicitWidth + Kirigami.Units.smallSpacing * 3
        implicitHeight: chipRow.implicitHeight + Kirigami.Units.smallSpacing
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        Rectangle {
            anchors.fill: parent
            radius: height / 2
            color: chip.active ? Qt.alpha(Kirigami.Theme.highlightColor, 0.24)
                 : Qt.alpha(Kirigami.Theme.textColor, chip.containsMouse ? 0.12 : 0.06)
            border.width: chip.active ? 1 : 0
            border.color: Qt.alpha(Kirigami.Theme.highlightColor, 0.5)
            Behavior on color { ColorAnimation { duration: 120 } }
        }
        RowLayout {
            id: chipRow
            anchors.centerIn: parent
            spacing: Kirigami.Units.smallSpacing
            Kirigami.Icon {
                visible: chip.iconName !== ""
                source: chip.iconName
                Layout.preferredWidth: Kirigami.Units.iconSizes.small * 0.8
                Layout.preferredHeight: Layout.preferredWidth
                opacity: 0.7
            }
            PlasmaComponents.Label {
                text: chip.text
                font.pointSize: Kirigami.Theme.smallFont.pointSize
                font.weight: chip.active ? Font.DemiBold : Font.Normal
            }
            PlasmaComponents.Label {
                visible: chip.count !== ""
                text: chip.count
                font.pointSize: Kirigami.Theme.smallFont.pointSize
                font.features: { "tnum": 1 }
                opacity: 0.55
            }
        }
    }

    Component {
        id: shellComponent

        PopupShell {
            id: shell

            anchors.fill: parent
            icon: Plasmoid.icon
            title: i18n("System Log")
            subtitle: {
                const lines = i18np("%1 line", "%1 lines", root.rows.count)
                if (root.stateKey === "offline")
                    return i18n("Collector not running")
                if (root.searchMode)
                    return root.querying ? i18n("Searching the whole journal…") : i18n("Whole journal · %1", lines)
                return root.paused ? i18n("Paused · %1", lines) : i18n("Live · %1", lines)
            }
            statusColor: root.stateColor
            statusText: root.stateKey === "live" ? i18n("Following the journal") : root.stateKey === "paused" ? i18n("Paused") : i18n("Collector not running")
            searchPlaceholder: i18n("Search messages and apps in the whole journal…")
            matchCount: root.search === "" || root.querying ? -1 : root.rows.count
            tabs: [
                { key: "all", label: i18n("All") },
                { key: "info", label: i18n("Info") },
                { key: "warnings", label: i18n("Warnings"), badge: root.levelCounts[2] > 0 ? root.levelCounts[2] + "" : "" },
                { key: "errors", label: i18n("Errors"), badge: root.levelCounts[3] > 0 ? root.levelCounts[3] + "" : "" }
            ]
            showTabs: true
            currentTab: root.level
            onTabActivated: index => root.level = index
            onSearchTextChanged: root.search = searchText.trim()
            onCloseRequested: root.expanded = false
            onSearchAccepted: {
                if (root.rows.count === 0)
                    return
                const last = root.rows.count - 1
                if (!root.rows.get(last).expanded)
                    root.toggleExpand(last)
            }
            Component.onCompleted: if (root.search !== "") searchText = root.search

            Connections {
                target: root
                function onSearchRequested(text) { shell.searchText = text }
            }

            headerActions: [
                PlasmaComponents.ToolButton {
                    enabled: !root.searchMode
                    icon.name: root.paused ? "media-playback-start" : "media-playback-pause"
                    display: PlasmaComponents.AbstractButton.IconOnly
                    text: root.paused ? i18n("Resume following") : i18n("Pause")
                    onClicked: root.paused = !root.paused
                    QQC2.ToolTip.visible: hovered
                    QQC2.ToolTip.text: text
                },
                PlasmaComponents.ToolButton {
                    icon.name: "edit-copy"
                    display: PlasmaComponents.AbstractButton.IconOnly
                    text: i18n("Copy all shown lines")
                    enabled: root.rows.count > 0
                    onClicked: root.copyAll()
                    QQC2.ToolTip.visible: hovered
                    QQC2.ToolTip.text: text
                },
                PlasmaComponents.ToolButton {
                    icon.name: "edit-clear-history"
                    display: PlasmaComponents.AbstractButton.IconOnly
                    text: i18n("Clear")
                    onClicked: root.clearLog()
                    QQC2.ToolTip.visible: hovered
                    QQC2.ToolTip.text: text
                },
                PlasmaComponents.ToolButton {
                    icon.name: "configure"
                    display: PlasmaComponents.AbstractButton.IconOnly
                    text: i18n("Configure…")
                    onClicked: Plasmoid.internalAction("configure").trigger()
                    QQC2.ToolTip.visible: hovered
                    QQC2.ToolTip.text: text
                }
            ]

            ColumnLayout {
                anchors.fill: parent
                spacing: Kirigami.Units.smallSpacing * 2

                PopCard {
                    title: i18n("Activity")
                    icon: "view-statistics"
                    trailing: i18n("%1 lines/min", Math.round(root.linesPerMinute))
                    collapsible: true
                    collapsed: !Plasmoid.configuration.showActivity
                    onCollapseToggled: Plasmoid.configuration.showActivity = !Plasmoid.configuration.showActivity

                    Sparkline {
                        Layout.fillWidth: true
                        Layout.preferredHeight: Kirigami.Units.gridUnit * 2.6
                        values: root.activity
                        values2: root.activityAlerts
                        lineColor: Kirigami.Theme.highlightColor
                        lineColor2: Kirigami.Theme.neutralTextColor
                        gradient: false
                        peakMarker: false
                        rangeFloor: 4
                        tipText: (value, index, total) => i18n("%1 lines, %2 warnings or worse · %3 s ago", value, root.activityAlerts[index] || 0,
                                                              (total - 1 - index) * root.activityBucketSeconds)
                    }
                }

                Flickable {
                    Layout.fillWidth: true
                    Layout.preferredHeight: sources.implicitHeight
                    visible: root.topSources.length > 0 || root.mutedList.length > 0
                    contentWidth: sources.implicitWidth
                    flickableDirection: Flickable.HorizontalFlick
                    boundsBehavior: Flickable.StopAtBounds
                    clip: true

                    Row {
                        id: sources
                        spacing: Kirigami.Units.smallSpacing

                        PlasmaComponents.Label {
                            anchors.verticalCenter: parent.verticalCenter
                            text: i18n("Busiest")
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                            font.weight: Font.DemiBold
                            font.capitalization: Font.AllUppercase
                            font.letterSpacing: 0.5
                            opacity: 0.55
                            rightPadding: Kirigami.Units.smallSpacing
                        }
                        Repeater {
                            model: root.topSources
                            SourceChip {
                                required property var modelData
                                text: modelData.app
                                count: modelData.count + ""
                                active: root.search.toLowerCase() === modelData.app.toLowerCase()
                                onClicked: shell.searchText = active ? "" : modelData.app
                                QQC2.ToolTip.visible: containsMouse
                                QQC2.ToolTip.text: active ? i18n("Show everything again") : i18n("Show only %1", modelData.app)
                                QQC2.ToolTip.delay: 500
                            }
                        }
                        SourceChip {
                            id: mutedChip
                            visible: root.mutedList.length > 0
                            iconName: "audio-volume-muted-symbolic"
                            text: i18np("%1 muted", "%1 muted", root.mutedList.length)
                            onClicked: mutedMenu.popup(mutedChip, 0, mutedChip.height)
                            QQC2.Menu {
                                id: mutedMenu
                                Instantiator {
                                    model: root.mutedList
                                    delegate: QQC2.MenuItem {
                                        required property string modelData
                                        text: i18n("Unmute \"%1\"", modelData)
                                        icon.name: "audio-volume-high"
                                        onTriggered: root.unmuteApp(modelData)
                                    }
                                    onObjectAdded: (index, object) => mutedMenu.insertItem(index, object)
                                    onObjectRemoved: (index, object) => mutedMenu.removeItem(object)
                                }
                                QQC2.MenuSeparator {}
                                QQC2.MenuItem {
                                    text: i18n("Unmute all")
                                    icon.name: "edit-undo"
                                    onTriggered: root.setMuted([])
                                }
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: Kirigami.Units.cornerRadius * 2
                    color: Qt.alpha(Kirigami.Theme.textColor, 0.03)
                    border.width: 1
                    border.color: Qt.alpha(Kirigami.Theme.textColor, 0.07)
                    clip: true

                    PlasmaComponents.ScrollView {
                        anchors.fill: parent
                        anchors.margins: 1
                        QQC2.ScrollBar.horizontal.policy: QQC2.ScrollBar.AlwaysOff

                        ListView {
                            id: view
                            model: root.rows
                            reuseItems: true
                            boundsBehavior: Flickable.StopAtBounds
                            spacing: 1

                            readonly property real stickPx: Math.max(0, Plasmoid.configuration.stickLines) * Kirigami.Units.gridUnit * 1.6

                            function updateFollowing() {
                                root.atBottom = (contentHeight <= height) || (contentY >= contentHeight - height - stickPx)
                                root.farFromEnd = contentY < contentHeight - height * 2
                                if (root.atBottom)
                                    root.hasNew = false
                            }
                            onContentYChanged: updateFollowing()
                            onHeightChanged: updateFollowing()
                            function followTail(added) {
                                if (added <= 0 || root.paused)
                                    return
                                if (root.atBottom) {
                                    if (!moving && !dragging)
                                        Qt.callLater(positionViewAtEnd)
                                } else {
                                    root.hasNew = true
                                }
                            }
                            Connections {
                                target: root
                                function onRowsAppended(added) { view.followTail(added) }
                                function onRowRevealRequested(index) { Qt.callLater(() => view.positionViewAtIndex(index, ListView.Contain)) }
                            }
                            Component.onCompleted: {
                                updateFollowing()
                                Qt.callLater(positionViewAtEnd)
                            }

                            delegate: LogRow {
                                width: view.width
                                query: root.search
                            }
                        }
                    }

                    PlasmaExtras.PlaceholderMessage {
                        anchors.centerIn: parent
                        width: parent.width - Kirigami.Units.gridUnit * 4
                        visible: root.rows.count === 0 && root.stateKey !== "offline"
                        iconName: root.querying ? "search" : root.searchMode ? "edit-find" : "utilities-log-viewer"
                        text: root.querying ? i18n("Searching the journal…")
                            : root.search !== "" ? i18n("No matches")
                            : root.searchMode ? i18n("Nothing at this level")
                            : i18n("Nothing new")
                        explanation: root.querying ? "" : root.search !== "" ? i18n("Nothing in the whole journal matches \"%1\".", root.search)
                            : root.searchMode ? "" : i18n("New journal lines will appear here as they are logged.")
                    }

                    StatusOverlay {
                        anchors.fill: parent
                        online: root.collectorOnline || root.searchMode
                        radius: parent.radius
                    }

                    Rectangle {
                        id: jumpPill
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: Kirigami.Units.largeSpacing
                        visible: opacity > 0
                        opacity: !root.atBottom && !root.paused && root.rows.count > 0 && (root.hasNew || root.farFromEnd) ? 1 : 0
                        Behavior on opacity { NumberAnimation { duration: 180 } }
                        width: jumpRow.implicitWidth + Kirigami.Units.largeSpacing * 2
                        height: jumpRow.implicitHeight + Kirigami.Units.smallSpacing * 2
                        radius: height / 2
                        color: root.hasNew ? Kirigami.Theme.highlightColor : Kirigami.Theme.backgroundColor
                        border.width: 1
                        border.color: Qt.alpha(Kirigami.Theme.textColor, 0.15)
                        RowLayout {
                            id: jumpRow
                            anchors.centerIn: parent
                            spacing: Kirigami.Units.smallSpacing
                            Kirigami.Icon {
                                source: "go-down-symbolic"
                                Layout.preferredWidth: Kirigami.Units.iconSizes.small
                                Layout.preferredHeight: Layout.preferredWidth
                                color: root.hasNew ? Kirigami.Theme.highlightedTextColor : Kirigami.Theme.textColor
                            }
                            PlasmaComponents.Label {
                                text: root.hasNew ? i18n("New lines") : i18n("Jump to newest")
                                font.weight: Font.DemiBold
                                color: root.hasNew ? Kirigami.Theme.highlightedTextColor : Kirigami.Theme.textColor
                            }
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                view.positionViewAtEnd()
                                root.hasNew = false
                            }
                        }
                    }

                    Rectangle {
                        id: copyToast
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: Kirigami.Units.largeSpacing * 4
                        radius: height / 2
                        color: Qt.alpha(Kirigami.Theme.positiveTextColor, 0.2)
                        border.width: 1
                        border.color: Qt.alpha(Kirigami.Theme.positiveTextColor, 0.5)
                        width: toastLabel.implicitWidth + Kirigami.Units.largeSpacing * 2
                        height: toastLabel.implicitHeight + Kirigami.Units.smallSpacing * 2
                        opacity: 0
                        visible: opacity > 0
                        Connections {
                            target: root
                            function onLineCopied() { fade.restart() }
                        }
                        PlasmaComponents.Label {
                            id: toastLabel
                            anchors.centerIn: parent
                            text: i18n("Copied to clipboard")
                            font.weight: Font.DemiBold
                        }
                        SequentialAnimation {
                            id: fade
                            NumberAnimation { target: copyToast; property: "opacity"; to: 1; duration: 120 }
                            PauseAnimation { duration: 900 }
                            NumberAnimation { target: copyToast; property: "opacity"; to: 0; duration: 300 }
                        }
                    }
                }
            }
        }
    }
}
