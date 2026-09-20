/*
 * Covers a widget with a centered message when monitoring is paused or the
 * router is unreachable. Driven by the shared RouterData state, so all widgets
 * show the same status at once. When paused it offers a Resume button.
 */
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PlasmaComponents
import org.kde.plasma.plasma5support as P5Support

Rectangle {
    id: overlay

    property bool online: true
    property bool paused: false
    readonly property string ctl: "$HOME/.local/bin/routermon-ctl"
    property string sshKey: "~/.ssh/id_ed25519"
    property string remoteScript: "/jffs/lrm-collect.sh"
    property bool connectionLoaded: false
    property bool connectionBusy: false
    property bool connectionError: false
    property string connectionResult: ""
    property bool showInstructions: false

    function shq(value) {
        return "'" + String(value).replace(/'/g, "'\\''") + "'"
    }
    function connectRouter() {
        if (connectionBusy || routerHost.text.trim() === "" || routerUser.text.trim() === "" || routerPassword.text === "")
            return
        connectionBusy = true
        connectionError = false
        connectionResult = i18n("Connecting…")
        const command = "$HOME/.local/bin/routermon-config connect"
            + " " + shq(routerHost.text.trim())
            + " " + shq(routerUser.text.trim())
            + " " + shq(sshKey)
            + " " + shq(remoteScript)
            + " " + shq(Qt.btoa(routerPassword.text))
            + " # " + Date.now()
        connectionAction.connectSource(command)
    }

    z: 100
    visible: paused || !online
    color: Kirigami.Theme.backgroundColor
    radius: Kirigami.Units.smallSpacing

    // swallow clicks so the (paused) widget underneath can't be interacted with
    MouseArea { anchors.fill: parent }

    P5Support.DataSource {
        id: ctlRun
        engine: "executable"
        onNewData: function(source, d) { disconnectSource(source) }
    }
    P5Support.DataSource {
        id: connectionReader
        engine: "executable"
        onNewData: function(source, result) {
            disconnectSource(source)
            try {
                const config = JSON.parse(result.stdout || "{}")
                routerHost.text = config.host || ""
                routerUser.text = config.user || "admin"
                overlay.sshKey = config.ssh_key || "~/.ssh/id_ed25519"
                overlay.remoteScript = config.remote_script || "/jffs/lrm-collect.sh"
                overlay.connectionLoaded = true
            } catch (error) {
                overlay.connectionError = true
                overlay.connectionResult = i18n("Could not load the router settings")
            }
        }
    }
    P5Support.DataSource {
        id: connectionAction
        engine: "executable"
        onNewData: function(source, result) {
            disconnectSource(source)
            overlay.connectionBusy = false
            const failed = Number(result["exit code"] || 0) !== 0
            overlay.connectionError = failed
            overlay.connectionResult = String(failed ? (result.stderr || result.stdout) : i18n("Connected")).trim()
            if (!failed)
                routerPassword.text = ""
        }
    }

    Component.onCompleted: connectionReader.connectSource("$HOME/.local/bin/routermon-config get # " + Date.now())

    ColumnLayout {
        anchors.centerIn: parent
        width: parent.width - Kirigami.Units.largeSpacing * 2
        spacing: Kirigami.Units.smallSpacing

        Kirigami.Icon {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: Kirigami.Units.iconSizes.large
            Layout.preferredHeight: Kirigami.Units.iconSizes.large
            source: overlay.paused ? "media-playback-pause" : "network-disconnect"
            opacity: 0.8
        }
        PlasmaComponents.Label {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            font.weight: Font.DemiBold
            text: overlay.paused ? i18n("Paused") : i18n("Router not connected")
        }
        PlasmaComponents.Label {
            visible: !overlay.paused
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            text: i18n("Router Monitor uses SSH. Enable SSH on the router before connecting.")
            opacity: 0.75
        }
        QQC2.Button {
            visible: !overlay.paused
            Layout.alignment: Qt.AlignHCenter
            flat: true
            text: overlay.showInstructions ? i18n("Hide instructions") : i18n("How to enable SSH")
            onClicked: overlay.showInstructions = !overlay.showInstructions
        }
        PlasmaComponents.Label {
            visible: !overlay.paused && overlay.showInstructions
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            text: i18n("ASUS / Asuswrt-Merlin: open Administration → System → SSH Daemon. Set Enable SSH to LAN only and allow password login. Connect authorizes a key and does not save your password, so password login can be turned off afterward.")
            opacity: 0.75
        }
        QQC2.TextField {
            id: routerHost
            visible: !overlay.paused
            Layout.fillWidth: true
            placeholderText: i18n("Router address")
        }
        QQC2.TextField {
            id: routerUser
            visible: !overlay.paused
            Layout.fillWidth: true
            placeholderText: i18n("Username")
        }
        QQC2.TextField {
            id: routerPassword
            visible: !overlay.paused
            Layout.fillWidth: true
            placeholderText: i18n("Password")
            echoMode: TextInput.Password
            onAccepted: overlay.connectRouter()
        }
        QQC2.Button {
            visible: !overlay.paused
            Layout.alignment: Qt.AlignHCenter
            text: overlay.connectionBusy ? i18n("Connecting…") : i18n("Connect")
            enabled: overlay.connectionLoaded && !overlay.connectionBusy
                && routerHost.text.trim() !== "" && routerUser.text.trim() !== "" && routerPassword.text !== ""
            onClicked: overlay.connectRouter()
        }
        PlasmaComponents.Label {
            visible: !overlay.paused && overlay.connectionResult !== ""
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            text: overlay.connectionResult
            color: overlay.connectionError ? Kirigami.Theme.negativeTextColor : Kirigami.Theme.textColor
        }
        QQC2.Button {
            Layout.alignment: Qt.AlignHCenter
            visible: overlay.paused
            icon.name: "media-playback-start"
            text: i18n("Resume")
            onClicked: ctlRun.connectSource(overlay.ctl + " pause off")
        }
    }
}
