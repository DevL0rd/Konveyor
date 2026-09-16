import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.plasma.plasmoid
import org.kde.plasma.components as PlasmaComponents
import org.kde.plasma.extras as PlasmaExtras
import "lib"
import "lib/PopStyle.js" as Style

Item {
    id: full

    Layout.minimumWidth: Kirigami.Units.gridUnit * 26
    Layout.minimumHeight: Kirigami.Units.gridUnit * 24
    Layout.preferredWidth: Kirigami.Units.gridUnit * 38
    Layout.preferredHeight: Kirigami.Units.gridUnit * 42

    readonly property var filters: [
        { key: "all", label: i18n("All") },
        { key: "apps", label: i18n("Apps") },
        { key: "gpu", label: i18n("Using GPU") },
        { key: "disk", label: i18n("Using disk") }
    ]

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

    component HeaderCell: MouseArea {
        id: header
        property string label
        property string key
        property int align: Text.AlignRight
        implicitHeight: headerLabel.implicitHeight + Kirigami.Units.smallSpacing
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.headerSort(key)
        PlasmaComponents.Label {
            id: headerLabel
            anchors.fill: parent
            horizontalAlignment: header.align
            verticalAlignment: Text.AlignVCenter
            text: header.label + (Plasmoid.configuration.sortColumn === header.key ? (Plasmoid.configuration.sortDescending ? " ▾" : " ▴") : "")
            font.pointSize: Kirigami.Theme.smallFont.pointSize
            font.weight: Font.DemiBold
            font.capitalization: Font.AllUppercase
            font.letterSpacing: 0.5
            opacity: header.containsMouse || Plasmoid.configuration.sortColumn === header.key ? 0.95 : 0.6
            elide: Text.ElideRight
        }
    }

    Component {
        id: shellComponent

        PopupShell {
            id: shell

            anchors.fill: parent
            icon: root.panelIcon
            title: i18n("Process Monitor")
            subtitle: root.hasData ? i18n("%1 processes · CPU %2% · VRAM %3", root.summary.count, Math.round(root.summary.cpu), Style.bytes(root.summary.vram)) : ""
            statusColor: root.hasData ? Kirigami.Theme.positiveTextColor : Kirigami.Theme.disabledTextColor
            statusText: root.hasData ? i18n("Live") : i18n("Waiting for the collector")
            searchPlaceholder: i18n("Search by name or PID…")
            matchCount: root.searchText.trim() === "" ? -1 : root.rows.count
            onCloseRequested: root.expanded = false
            onSearchAccepted: if (root.rows.count > 0) root.expandedPid = root.rows.get(0).pid

            Component.onCompleted: searchText = root.searchText
            onSearchTextChanged: root.searchText = searchText
            Connections {
                target: root
                function onSearchTextChanged() {
                    if (shell.searchText !== root.searchText)
                        shell.searchText = root.searchText
                }
            }

            headerActions: [
                PlasmaComponents.ToolButton {
                    icon.name: Plasmoid.configuration.treeView ? "view-list-tree" : "view-list-details"
                    display: PlasmaComponents.AbstractButton.IconOnly
                    text: Plasmoid.configuration.treeView ? i18n("Show as a flat list") : i18n("Show as a tree")
                    onClicked: Plasmoid.configuration.treeView = !Plasmoid.configuration.treeView
                    QQC2.ToolTip.visible: hovered
                    QQC2.ToolTip.text: text
                },
                PlasmaComponents.ToolButton {
                    visible: Plasmoid.configuration.treeView
                    icon.name: "format-indent-less"
                    display: PlasmaComponents.AbstractButton.IconOnly
                    text: i18n("Collapse all")
                    onClicked: root.collapseAll()
                    QQC2.ToolTip.visible: hovered
                    QQC2.ToolTip.text: text
                },
                PlasmaComponents.ToolButton {
                    id: columnsButton
                    icon.name: "view-split-left-right"
                    display: PlasmaComponents.AbstractButton.IconOnly
                    text: i18n("Columns")
                    onClicked: columnMenu.popup(columnsButton, 0, columnsButton.height)
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

            QQC2.Menu {
                id: columnMenu
                Repeater {
                    model: root.allColumns.filter(column => root.columnConfigKeys[column.key] !== undefined)
                    QQC2.MenuItem {
                        required property var modelData
                        text: modelData.label
                        checkable: true
                        checked: Plasmoid.configuration[root.columnConfigKeys[modelData.key]]
                        onTriggered: Plasmoid.configuration[root.columnConfigKeys[modelData.key]] = checked
                    }
                }
            }

            ColumnLayout {
                anchors.fill: parent
                spacing: Kirigami.Units.largeSpacing

                FocusCard {
                    visible: Plasmoid.configuration.showFocusedCard && root.searchText.trim() === ""
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.largeSpacing

                    PopTabs {
                        Layout.fillWidth: true
                        model: full.filters.map(filter => ({ label: filter.label }))
                        currentIndex: Math.max(0, full.filters.findIndex(filter => filter.key === Plasmoid.configuration.processFilter))
                        onActivated: index => Plasmoid.configuration.processFilter = full.filters[index].key
                    }
                    PlasmaComponents.Label {
                        visible: root.summary.gpuTop !== null && root.summary.gpuTop !== undefined
                        text: root.summary.gpuTop ? i18n("Top GPU: %1 %2%", root.summary.gpuTop.name, root.summary.gpuTop.gpu) : ""
                        font: Kirigami.Theme.smallFont
                        opacity: 0.65
                        elide: Text.ElideRight
                        Layout.maximumWidth: Kirigami.Units.gridUnit * 10
                    }
                    QQC2.CheckBox {
                        text: i18n("Kernel threads")
                        checked: Plasmoid.configuration.showKernelThreads
                        onToggled: Plasmoid.configuration.showKernelThreads = checked
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: Kirigami.Units.cornerRadius * 2
                    color: Qt.alpha(Kirigami.Theme.textColor, 0.03)
                    border.width: 1
                    border.color: Qt.alpha(Kirigami.Theme.textColor, 0.07)

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: Kirigami.Units.smallSpacing
                        spacing: 0

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.leftMargin: Kirigami.Units.smallSpacing
                            Layout.rightMargin: Kirigami.Units.smallSpacing + (list.QQC2.ScrollBar.vertical ? list.QQC2.ScrollBar.vertical.width : 0)
                            spacing: Kirigami.Units.smallSpacing
                            HeaderCell {
                                Layout.fillWidth: true
                                label: i18n("Name")
                                key: "name"
                                align: Text.AlignLeft
                            }
                            Repeater {
                                model: root.columns
                                HeaderCell {
                                    required property var modelData
                                    Layout.preferredWidth: Kirigami.Units.gridUnit * 3.6
                                    label: modelData.label
                                    key: modelData.key
                                }
                            }
                        }

                        Kirigami.Separator {
                            Layout.fillWidth: true
                            Layout.topMargin: Kirigami.Units.smallSpacing
                        }

                        ListView {
                            id: list
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            reuseItems: true
                            cacheBuffer: Kirigami.Units.gridUnit * 20
                            boundsBehavior: Flickable.StopAtBounds
                            model: root.rows
                            spacing: 1
                            QQC2.ScrollBar.vertical: PlasmaComponents.ScrollBar {}
                            Component.onCompleted: root.rowsView = list

                            delegate: ProcessRow {
                                width: list.width - (list.QQC2.ScrollBar.vertical.visible ? list.QQC2.ScrollBar.vertical.width : 0)
                                onMenuRequested: proc => {
                                    processMenu.proc = proc
                                    processMenu.popup()
                                }
                            }

                            PlasmaExtras.PlaceholderMessage {
                                anchors.centerIn: parent
                                width: parent.width - Kirigami.Units.gridUnit * 4
                                visible: list.count === 0
                                iconName: root.hasData ? "edit-find" : root.panelIcon
                                text: root.hasData ? i18n("No matching processes") : i18n("Waiting for the collector")
                                explanation: root.hasData ? i18n("Try another name, a PID, or a different filter") : ""
                            }
                        }
                    }
                }
            }

            QQC2.Menu {
                id: processMenu
                property var proc: ({})
                QQC2.MenuItem { text: i18n("Copy PID"); icon.name: "edit-copy"; onTriggered: root.copyText(String(processMenu.proc.pid)) }
                QQC2.MenuItem { text: i18n("Copy name"); icon.name: "edit-copy"; onTriggered: root.copyText(processMenu.proc.name || "") }
                QQC2.MenuItem { text: i18n("Copy command line"); icon.name: "edit-copy"; onTriggered: root.copyCmdline(processMenu.proc.pid) }
                QQC2.MenuSeparator {}
                QQC2.MenuItem { text: i18n("Open file location"); icon.name: "folder-open"; onTriggered: root.openLocation(processMenu.proc.pid) }
                QQC2.MenuItem { text: i18n("Open journal log"); icon.name: "utilities-log-viewer"; onTriggered: root.openJournal(processMenu.proc.name || "") }
                QQC2.MenuItem { text: i18n("Restart (best-effort)"); icon.name: "system-reboot"; onTriggered: root.restartProc(processMenu.proc.pid) }
                QQC2.MenuSeparator {}
                QQC2.MenuItem { text: i18n("Stop (SIGSTOP)"); icon.name: "media-playback-pause"; onTriggered: root.signalProc(processMenu.proc.pid, "STOP") }
                QQC2.MenuItem { text: i18n("Continue (SIGCONT)"); icon.name: "media-playback-start"; onTriggered: root.signalProc(processMenu.proc.pid, "CONT") }
                QQC2.MenuSeparator {}
                QQC2.MenuItem { text: i18n("Kill (SIGTERM)"); icon.name: "process-stop"; onTriggered: root.signalProc(processMenu.proc.pid, "TERM") }
                QQC2.MenuItem { text: i18n("Force kill (SIGKILL)"); icon.name: "process-stop"; onTriggered: root.signalProc(processMenu.proc.pid, "KILL") }
                QQC2.MenuItem { text: i18n("Force kill as root…"); icon.name: "process-stop"; onTriggered: root.forceKillAsRoot(processMenu.proc.pid) }
            }
        }
    }
}
