import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.plasma.plasmoid
import org.kde.plasma.components as PlasmaComponents
import "../lib"
import "../lib/Format.js" as Fmt
import "../lib/Highlight.js" as Highlight

Rectangle {
    id: row

    property string mac
    property string name
    property string ip
    property bool connected
    property bool blocked
    property real traffic: -1
    property bool wireless
    property string band
    property int rssi
    property real txMbps
    property real rxMbps
    property real phyMbps
    property bool pinned
    property string query
    readonly property bool expanded: root.expandedMac === mac
    signal menuRequested()

    implicitHeight: column.implicitHeight + Kirigami.Units.smallSpacing * 4
    radius: Kirigami.Units.cornerRadius * 2
    color: blocked ? Qt.alpha(Kirigami.Theme.negativeTextColor, 0.1)
         : Qt.alpha(Kirigami.Theme.textColor, expanded ? 0.075 : mouse.containsMouse ? 0.06 : 0.04)
    border.width: 1
    border.color: expanded ? Qt.alpha(Kirigami.Theme.highlightColor, 0.45) : Qt.alpha(Kirigami.Theme.textColor, 0.07)
    Behavior on color { ColorAnimation { duration: 150 } }
    Behavior on implicitHeight { NumberAnimation { duration: 220; easing.type: Easing.OutCubic } }
    clip: true

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        cursorShape: Qt.PointingHandCursor
        onClicked: function(event) {
            if (event.button === Qt.RightButton)
                row.menuRequested()
            else
                root.expandedMac = row.expanded ? "" : row.mac
        }
    }

    ColumnLayout {
        id: column
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: Kirigami.Units.smallSpacing * 2
        spacing: Kirigami.Units.smallSpacing

        RowLayout {
            Layout.fillWidth: true
            spacing: Kirigami.Units.smallSpacing * 1.5

            Kirigami.Icon {
                visible: row.pinned
                source: "window-pin"
                color: root.accent
                Layout.preferredWidth: Kirigami.Units.iconSizes.small
                Layout.preferredHeight: Kirigami.Units.iconSizes.small
            }
            Kirigami.Icon {
                source: row.wireless ? "network-wireless" : "network-wired"
                Layout.preferredWidth: Kirigami.Units.iconSizes.small
                Layout.preferredHeight: Kirigami.Units.iconSizes.small
                opacity: 0.75
            }
            PlasmaComponents.Label {
                text: Highlight.mark(row.name, row.query, Kirigami.Theme.highlightColor)
                textFormat: Text.StyledText
                font.weight: Font.DemiBold
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
            Rectangle {
                width: Kirigami.Units.smallSpacing * 1.5
                height: width
                radius: width / 2
                color: row.blocked ? Kirigami.Theme.negativeTextColor : row.connected ? Kirigami.Theme.positiveTextColor : Kirigami.Theme.disabledTextColor
            }
            PlasmaComponents.Label {
                text: Fmt.rate(row.traffic)
                font.pointSize: Kirigami.Theme.smallFont.pointSize
                font.features: { "tnum": 1 }
                color: row.traffic > 0 ? root.accent : Kirigami.Theme.textColor
                opacity: row.traffic > 0 ? 1 : 0.5
            }
            PlasmaComponents.Label {
                visible: row.wireless
                text: root.shortBand(row.band)
                font: Kirigami.Theme.smallFont
                opacity: 0.6
            }
            SignalBars {
                visible: row.wireless
                level: Fmt.rssiBars(row.rssi)
                activeColor: Fmt.rssiColor(row.rssi, Kirigami.Theme)
                tip: row.rssi + " dBm"
            }
        }

        Sparkline {
            visible: Plasmoid.configuration.showCharts && row.connected && values.some(v => v > 0)
            Layout.fillWidth: true
            Layout.preferredHeight: Kirigami.Units.gridUnit * 1.8
            values: root.clientSeries(row.mac)
            lineColor: root.accent
            gradient: false
            peakMarker: false
            rangeFloor: 125000
            tipText: v => Fmt.rate(v)
        }

        GridLayout {
            visible: row.expanded
            Layout.fillWidth: true
            Layout.topMargin: Kirigami.Units.smallSpacing
            columns: 2
            columnSpacing: Kirigami.Units.largeSpacing
            rowSpacing: 2

            Repeater {
                model: [
                    { label: i18n("IP address"), value: row.ip },
                    { label: i18n("MAC address"), value: row.mac },
                    { label: i18n("Connection"), value: row.wireless ? i18n("WiFi %1 · %2 dBm", row.band, row.rssi) : i18n("Wired") },
                    { label: i18n("Link rate"), value: row.wireless ? i18n("↓ %1  ↑ %2 Mb/s", Math.round(row.txMbps), Math.round(row.rxMbps)) : "" },
                    { label: i18n("Status"), value: row.blocked ? i18n("Internet blocked") : row.connected ? i18n("Online") : i18n("Offline") }
                ].filter(entry => entry.value !== "")
                RowLayout {
                    required property var modelData
                    Layout.columnSpan: 2
                    Layout.fillWidth: true
                    PlasmaComponents.Label {
                        text: modelData.label
                        opacity: 0.6
                        font: Kirigami.Theme.smallFont
                        Layout.preferredWidth: Kirigami.Units.gridUnit * 6
                    }
                    PlasmaComponents.Label {
                        text: Highlight.mark(modelData.value, row.query, Kirigami.Theme.highlightColor)
                        textFormat: Text.StyledText
                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                        font.features: { "tnum": 1 }
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }
                }
            }
        }

        PopActions {
            visible: row.expanded
            Layout.topMargin: Kirigami.Units.smallSpacing
            model: [
                { icon: row.pinned ? "window-unpin" : "window-pin", text: row.pinned ? i18n("Unpin") : i18n("Pin"), run: () => root.togglePin(row.mac) },
                { icon: "utilities-terminal", text: i18n("SSH"), run: () => root.launch("konsole -e ssh " + root.sshTarget(row.ip)) },
                { icon: "folder-remote", text: i18n("Files"), run: () => root.launch("xdg-open smb://" + row.ip + "/") },
                { icon: "network-connect", text: i18n("Ping"), run: () => root.launch("konsole -e bash -c \"ping " + row.ip + "; read -n1 -p Done\"") },
                { icon: "system-search", text: i18n("Port scan"), run: () => root.launch("konsole -e bash -c \"nmap " + row.ip + " || echo nmap-not-installed; read -n1 -p Done\"") },
                { icon: "edit-copy", text: i18n("Copy IP"), run: () => root.copy(row.ip) },
                { icon: "network-card", text: i18n("Copy MAC"), run: () => root.copy(row.mac) },
                { icon: "edit-rename", text: i18n("Rename…"), run: () => root.promptRequested("rename", row.mac, row.name) },
                { icon: "bookmark-new", text: i18n("Reserve IP…"), run: () => root.promptRequested("reserve", row.mac, row.ip) },
                { icon: "network-disconnect", text: i18n("Disconnect"), visible: row.wireless, run: () => root.ctlRun("disconnect " + row.mac) },
                { icon: row.blocked ? "dialog-ok-apply" : "dialog-cancel", text: row.blocked ? i18n("Unblock internet") : i18n("Block internet"),
                  destructive: !row.blocked, run: () => root.ctlRun((row.blocked ? "unblock " : "block ") + row.mac) }
            ]
        }
    }
}
