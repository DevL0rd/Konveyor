import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.plasma.plasmoid
import org.kde.plasma.components as PlasmaComponents
import org.kde.plasma.extras as PlasmaExtras
import "lib"
import "lib/Format.js" as Fmt

Item {
    id: full

    Layout.minimumWidth: Kirigami.Units.gridUnit * 13
    Layout.minimumHeight: Kirigami.Units.gridUnit * 12
    Layout.preferredWidth: Kirigami.Units.gridUnit * (root.locked ? 24 : 30)
    Layout.preferredHeight: Kirigami.Units.gridUnit * (root.locked ? 28 : 40)

    readonly property var tabDefs: [
        { key: "overview", label: i18n("Overview"), file: "OverviewTab.qml" },
        { key: "network", label: i18n("Network"), file: "NetworkTab.qml" },
        { key: "wifi", label: i18n("WiFi"), file: "WifiTab.qml" },
        { key: "clients", label: i18n("Clients"), file: "ClientsTab.qml" },
        { key: "dns", label: i18n("DNS"), file: "DnsTab.qml" },
        { key: "speed", label: i18n("Speed"), file: "SpeedTab.qml" },
        { key: "system", label: i18n("System"), file: "SystemTab.qml" }
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

    Component {
        id: shellComponent

        PopupShell {
            id: shell

            readonly property int tabIndex: Math.max(0, full.tabDefs.findIndex(tab => tab.key === root.activeTab))

            anchors.fill: parent
            icon: Plasmoid.icon
            title: root.locked ? root.locked.label : root.info.model || i18n("Router")
            subtitle: {
                const parts = []
                if (root.locked && root.info.model)
                    parts.push(root.info.model)
                if (root.info.fw)
                    parts.push(i18n("Firmware %1", root.info.fw))
                if (root.info.uptime)
                    parts.push(i18n("up %1", Fmt.duration(root.info.uptime)))
                if (root.network.wan_ip)
                    parts.push(root.network.wan_ip)
                return parts.join("  ·  ")
            }
            statusColor: root.stateColor
            statusText: root.stateText
            searchPlaceholder: i18n("Search devices, radios, domains, actions…")
            tabs: root.locked ? [] : full.tabDefs.map(tab => ({ key: tab.key, label: tab.label, badge: tab.key === "clients" ? root.onlineCount + "" : "" }))
            currentTab: tabIndex
            matchCount: results.item ? results.item.count : -1
            onTabActivated: index => root.tabKey = full.tabDefs[index].key
            Connections {
                target: root
                function onTabKeyChanged() { if (shell.searchText !== "") shell.clearSearch() }
            }
            onCloseRequested: root.expanded = false
            onSearchAccepted: if (results.item) results.item.activateFirst()

            headerActions: [
                PlasmaComponents.ToolButton {
                    visible: Plasmoid.configuration.showWebUI && !!root.info.admin_url
                    icon.name: "internet-web-browser"
                    display: PlasmaComponents.AbstractButton.IconOnly
                    text: i18n("Open router web UI")
                    onClicked: root.launch("xdg-open " + root.info.admin_url)
                    QQC2.ToolTip.visible: hovered
                    QQC2.ToolTip.text: text
                },
                PlasmaComponents.ToolButton {
                    visible: Plasmoid.configuration.showOpenUI && !!root.info.agh_url
                    icon.name: "security-high"
                    display: PlasmaComponents.AbstractButton.IconOnly
                    text: i18n("Open AdGuard Home")
                    onClicked: root.launch("xdg-open " + root.info.agh_url)
                    QQC2.ToolTip.visible: hovered
                    QQC2.ToolTip.text: text
                },
                PlasmaComponents.ToolButton {
                    icon.name: root.paused ? "media-playback-start" : "media-playback-pause"
                    display: PlasmaComponents.AbstractButton.IconOnly
                    text: root.paused ? i18n("Resume monitoring") : i18n("Pause monitoring")
                    onClicked: root.ctlRun("pause toggle")
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
                spacing: Kirigami.Units.smallSpacing

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    Loader {
                        id: tabLoader
                        anchors.fill: parent
                        visible: shell.searchText === ""
                        active: shell.searchText === ""
                        source: Qt.resolvedUrl("tabs/" + full.tabDefs[shell.tabIndex].file)
                        opacity: status === Loader.Ready ? 1 : 0
                        Behavior on opacity { NumberAnimation { duration: 160 } }
                    }

                    Loader {
                        id: results
                        anchors.fill: parent
                        active: shell.searchText !== ""
                        source: Qt.resolvedUrl("tabs/SearchResults.qml")
                        onLoaded: item.query = Qt.binding(() => shell.searchText.trim())
                    }

                    StatusOverlay {
                        anchors.fill: parent
                        online: root.online
                        paused: root.paused
                        color: Qt.alpha(Kirigami.Theme.backgroundColor, 0.92)
                        radius: Kirigami.Units.cornerRadius * 2
                    }

                    PlasmaExtras.PlaceholderMessage {
                        anchors.centerIn: parent
                        width: parent.width - Kirigami.Units.gridUnit * 4
                        visible: !root.ready && root.online && !root.paused
                        iconName: "network-wireless-hotspot"
                        text: i18n("Waiting for the router")
                        explanation: i18n("The router collector hasn't written a snapshot yet.")
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: messageLabel.implicitHeight + Kirigami.Units.smallSpacing * 2
                    visible: opacity > 0
                    opacity: root.message !== "" ? 1 : 0
                    radius: height / 2
                    color: Qt.alpha(root.messageError ? Kirigami.Theme.negativeTextColor : Kirigami.Theme.highlightColor, 0.18)
                    Behavior on opacity { NumberAnimation { duration: 220 } }
                    PlasmaComponents.Label {
                        id: messageLabel
                        anchors.centerIn: parent
                        width: parent.width - Kirigami.Units.largeSpacing * 2
                        horizontalAlignment: Text.AlignHCenter
                        elide: Text.ElideRight
                        text: root.message
                    }
                }
            }

            Kirigami.PromptDialog {
                id: prompt
                property string mac
                property string mode
                parent: shell
                title: mode === "rename" ? i18n("Rename device") : i18n("Reserve IP address")
                standardButtons: Kirigami.Dialog.Ok | Kirigami.Dialog.Cancel
                QQC2.TextField {
                    id: promptField
                    implicitWidth: Kirigami.Units.gridUnit * 14
                    onAccepted: prompt.accept()
                }
                onAccepted: {
                    const value = promptField.text.trim()
                    if (!value)
                        return
                    if (mode === "rename")
                        root.ctlRun("rename " + mac + " '" + value.replace(/'/g, "") + "'")
                    else
                        root.ctlRun("reserve " + mac + " " + value)
                }
            }
            Connections {
                target: root
                function onPromptRequested(mode, mac, value) {
                    prompt.mode = mode
                    prompt.mac = mac
                    promptField.text = value
                    prompt.open()
                    promptField.forceActiveFocus()
                }
            }
        }
    }
}
