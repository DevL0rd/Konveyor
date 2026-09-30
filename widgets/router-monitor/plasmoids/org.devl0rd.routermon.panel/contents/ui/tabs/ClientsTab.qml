import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.plasma.plasmoid
import org.kde.plasma.components as PlasmaComponents
import org.kde.plasma.extras as PlasmaExtras
import "../lib"

ColumnLayout {
    id: clientsTab

    spacing: Kirigami.Units.largeSpacing

    readonly property var filters: [
        { key: "all", label: i18n("All") },
        { key: "online", label: i18n("Online") },
        { key: "wifi", label: i18n("WiFi") },
        { key: "wired", label: i18n("Wired") },
        { key: "blocked", label: i18n("Blocked") }
    ]

    Component.onCompleted: root.syncClients()

    RowLayout {
        Layout.fillWidth: true
        spacing: Kirigami.Units.largeSpacing

        PopTabs {
            Layout.fillWidth: true
            model: clientsTab.filters.map(filter => ({ label: filter.label }))
            currentIndex: Math.max(0, clientsTab.filters.findIndex(filter => filter.key === Plasmoid.configuration.clientFilter))
            onActivated: index => Plasmoid.configuration.clientFilter = clientsTab.filters[index].key
        }
        PlasmaComponents.ComboBox {
            textRole: "text"
            valueRole: "value"
            model: [
                { text: i18n("By traffic"), value: "traffic" },
                { text: i18n("By signal"), value: "signal" },
                { text: i18n("By name"), value: "name" },
                { text: i18n("By IP"), value: "ip" }
            ]
            Component.onCompleted: currentIndex = Math.max(0, indexOfValue(Plasmoid.configuration.sortBy))
            onActivated: Plasmoid.configuration.sortBy = currentValue
        }
    }

    PlasmaComponents.ScrollView {
        Layout.fillWidth: true
        Layout.fillHeight: true

        ListView {
            id: list
            model: root.clients
            spacing: Kirigami.Units.smallSpacing * 1.5
            reuseItems: true
            clip: true
            move: Transition { NumberAnimation { properties: "y"; duration: 220; easing.type: Easing.OutCubic } }
            displaced: Transition { NumberAnimation { properties: "y"; duration: 220; easing.type: Easing.OutCubic } }

            delegate: ClientRow {
                required property var model
                width: list.width - (list.QQC2.ScrollBar.vertical && list.QQC2.ScrollBar.vertical.visible ? list.QQC2.ScrollBar.vertical.width : 0)
                mac: model.mac
                name: model.name
                ip: model.ip
                connected: model.connected
                blocked: model.blocked
                traffic: model.traffic
                wireless: model.wireless
                band: model.band
                rssi: model.rssi
                txMbps: model.txMbps
                rxMbps: model.rxMbps
                phyMbps: model.phyMbps
                pinned: model.pinned
                onMenuRequested: {
                    clientMenu.client = { mac: mac, name: name, ip: ip, pinned: pinned, blocked: blocked, wireless: wireless }
                    clientMenu.popup()
                }
            }

            PlasmaExtras.PlaceholderMessage {
                anchors.centerIn: parent
                width: parent.width - Kirigami.Units.gridUnit * 4
                visible: list.count === 0 && root.ready
                iconName: "network-server"
                text: i18n("No devices here")
                explanation: i18n("Try another filter")
            }
        }
    }

    QQC2.Menu {
        id: clientMenu
        property var client: null
        QQC2.MenuItem {
            text: clientMenu.client && clientMenu.client.pinned ? i18n("Unpin") : i18n("Pin to top")
            icon.name: clientMenu.client && clientMenu.client.pinned ? "window-unpin" : "window-pin"
            onTriggered: root.togglePin(clientMenu.client.mac)
        }
        QQC2.MenuSeparator {}
        QQC2.MenuItem { text: i18n("SSH"); icon.name: "utilities-terminal"; onTriggered: root.launch("konsole -e ssh " + root.sshTarget(clientMenu.client.ip)) }
        QQC2.MenuItem { text: i18n("Browse files (SMB)"); icon.name: "folder-remote"; onTriggered: root.launch("xdg-open smb://" + clientMenu.client.ip + "/") }
        QQC2.MenuItem { text: i18n("Ping"); icon.name: "network-connect"; onTriggered: root.launch("konsole -e bash -c \"ping " + clientMenu.client.ip + "; read -n1 -p Done\"") }
        QQC2.MenuItem { text: i18n("Port scan (nmap)"); icon.name: "system-search"; onTriggered: root.launch("konsole -e bash -c \"nmap " + clientMenu.client.ip + " || echo nmap-not-installed; read -n1 -p Done\"") }
        QQC2.MenuSeparator {}
        QQC2.MenuItem { text: i18n("Copy IP"); icon.name: "edit-copy"; onTriggered: root.copy(clientMenu.client.ip) }
        QQC2.MenuItem { text: i18n("Copy MAC"); icon.name: "network-card"; onTriggered: root.copy(clientMenu.client.mac) }
        QQC2.MenuItem { text: i18n("Rename…"); icon.name: "edit-rename"; onTriggered: root.promptRequested("rename", clientMenu.client.mac, clientMenu.client.name) }
        QQC2.MenuItem { text: i18n("Reserve IP…"); icon.name: "bookmark-new"; onTriggered: root.promptRequested("reserve", clientMenu.client.mac, clientMenu.client.ip) }
        QQC2.MenuSeparator {}
        QQC2.MenuItem {
            visible: clientMenu.client && clientMenu.client.wireless
            height: visible ? implicitHeight : 0
            text: i18n("Disconnect (WiFi)")
            icon.name: "network-disconnect"
            onTriggered: root.ctlRun("disconnect " + clientMenu.client.mac)
        }
        QQC2.MenuItem {
            text: clientMenu.client && clientMenu.client.blocked ? i18n("Unblock internet") : i18n("Block internet")
            icon.name: clientMenu.client && clientMenu.client.blocked ? "dialog-ok-apply" : "dialog-cancel"
            onTriggered: root.ctlRun((clientMenu.client.blocked ? "unblock " : "block ") + clientMenu.client.mac)
        }
    }
}
