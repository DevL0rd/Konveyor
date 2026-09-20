import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kquickcontrols as KQuickControls
import org.kde.plasma.plasmoid
import org.kde.plasma.plasma5support as P5Support

Kirigami.FormLayout {
    id: form

    property string title: i18n("General")
    property alias cfg_accentColor: accent.text
    property alias cfg_showCharts: showCharts.checked
    property alias cfg_showPerCore: showPerCore.checked
    property alias cfg_showTemps: showTemps.checked
    property alias cfg_showReboot: showReboot.checked
    property alias cfg_showRestartWifi: showRestartWifi.checked
    property alias cfg_showWebUI: showWebUI.checked
    property alias cfg_maxMbps: maxMbps.value
    property alias cfg_showInterference: showInterference.checked
    property alias cfg_showProtection: showProtection.checked
    property alias cfg_showOpenUI: showOpenUI.checked
    property alias cfg_showChart: showChart.checked
    property alias cfg_showTopLists: showTopLists.checked
    property string cfg_sortBy
    property alias cfg_sshUser: sshUser.text
    property string cfg_pinnedMacs
    property alias cfg_planDownMbps: planDownMbps.value
    property string cfg_lastResult
    property real cfg_peakDown
    property real cfg_peakUp
    property string cfg_compactExtra
    property alias cfg_panelShrink: panelShrink.checked
    property string cfg_panelDetail
    property alias cfg_middleClickPause: middleClickPause.checked
    property string cfg_defaultTab
    property string cfg_currentTab
    property string cfg_clientFilter
    property string cfg_speedHistory
    property alias cfg_rememberTab: rememberTab.checked
    property var cfg_accentColorDefault
    property var cfg_clientFilterDefault
    property var cfg_compactExtraDefault
    property var cfg_currentTabDefault
    property var cfg_defaultTabDefault
    property var cfg_lastResultDefault
    property var cfg_maxMbpsDefault
    property var cfg_middleClickPauseDefault
    property var cfg_panelDetailDefault
    property var cfg_panelShrinkDefault
    property var cfg_peakDownDefault
    property var cfg_peakUpDefault
    property var cfg_pinnedMacsDefault
    property var cfg_planDownMbpsDefault
    property var cfg_rememberTabDefault
    property var cfg_showChartDefault
    property var cfg_showChartsDefault
    property var cfg_showInterferenceDefault
    property var cfg_showOpenUIDefault
    property var cfg_showPerCoreDefault
    property var cfg_showProtectionDefault
    property var cfg_showRebootDefault
    property var cfg_showRestartWifiDefault
    property var cfg_showTempsDefault
    property var cfg_showTopListsDefault
    property var cfg_showWebUIDefault
    property var cfg_sortByDefault
    property var cfg_speedHistoryDefault
    property var cfg_sshUserDefault
    readonly property bool tabbed: Plasmoid.metaData.pluginId === "org.devl0rd.routermon.panel"
    property bool connectionLoaded: false
    property bool connectionBusy: false
    property string connectionResult
    property bool connectionError: false

    function shq(value) {
        return "'" + String(value).replace(/'/g, "'\\''") + "'"
    }
    function connectionArguments(mode, password) {
        let command = "$HOME/.local/bin/routermon-config " + mode
            + " " + shq(routerHost.text.trim())
            + " " + shq(routerUser.text.trim())
            + " " + shq(routerKey.text.trim())
            + " " + shq(remoteScript.text.trim())
        if (password !== undefined)
            command += " " + shq(Qt.btoa(password))
        return command + " # " + Date.now()
    }
    function runConnectionAction(mode, password) {
        connectionBusy = true
        connectionResult = i18n("Working…")
        connectionError = false
        connectionAction.connectSource(connectionArguments(mode, password))
    }
    function openRouterSettings() {
        const host = routerHost.text.trim()
        if (host !== "")
            launcher.connectSource("xdg-open " + shq(/^https?:\/\//.test(host) ? host : "http://" + host))
    }

    Component.onCompleted: connectionReader.connectSource("$HOME/.local/bin/routermon-config get # " + Date.now())

    P5Support.DataSource {
        id: connectionReader
        engine: "executable"
        onNewData: function(source, result) {
            disconnectSource(source)
            form.connectionBusy = false
            try {
                const config = JSON.parse(result.stdout || "{}")
                routerHost.text = config.host || ""
                routerUser.text = config.user || "admin"
                routerKey.text = config.ssh_key || "~/.ssh/id_ed25519"
                remoteScript.text = config.remote_script || "/jffs/lrm-collect.sh"
                form.connectionLoaded = true
            } catch (error) {
                form.connectionResult = i18n("Could not read the router connection settings")
                form.connectionError = true
            }
        }
    }
    P5Support.DataSource {
        id: connectionAction
        engine: "executable"
        onNewData: function(source, result) {
            disconnectSource(source)
            const failed = Number(result["exit code"] || 0) !== 0
            form.connectionError = failed
            form.connectionResult = String(failed ? (result.stderr || result.stdout) : result.stdout).trim()
            if (!failed && form.connectionResult === "")
                form.connectionResult = i18n("Done")
        }
    }
    P5Support.DataSource {
        id: launcher
        engine: "executable"
        onNewData: function(source, result) { disconnectSource(source) }
    }

    Kirigami.Heading {
        Kirigami.FormData.isSection: true
        text: i18n("Router connection")
        level: 3
    }
    Kirigami.InlineMessage {
        Layout.fillWidth: true
        visible: true
        type: Kirigami.MessageType.Information
        text: i18n("Router Monitor connects to ASUS / Asuswrt-Merlin over SSH. 1. Open the router settings. 2. Go to Administration → System → SSH Daemon. 3. Set Enable SSH to LAN only and allow password login. 4. Enter the login below and select Set up automatically. The password is not saved, and password login can be turned off afterward.")
    }
    QQC2.TextField {
        id: routerHost
        Kirigami.FormData.label: i18n("Router host:")
        placeholderText: "192.168.50.1"
    }
    QQC2.TextField {
        id: routerUser
        Kirigami.FormData.label: i18n("Router SSH user:")
        placeholderText: "admin"
    }
    QQC2.TextField {
        id: routerKey
        Kirigami.FormData.label: i18n("Private key:")
        placeholderText: "~/.ssh/id_ed25519"
    }
    QQC2.TextField {
        id: remoteScript
        Kirigami.FormData.label: i18n("Remote collector:")
        placeholderText: "/jffs/lrm-collect.sh"
    }
    QQC2.TextField {
        id: routerPassword
        Kirigami.FormData.label: i18n("Router password (not saved):")
        placeholderText: i18n("Used once while connecting")
        echoMode: TextInput.Password
        passwordCharacter: "•"
    }
    QQC2.Button {
        Kirigami.FormData.label: i18n("Router settings:")
        text: i18n("Open router settings")
        enabled: routerHost.text.trim() !== ""
        onClicked: form.openRouterSettings()
    }
    QQC2.Button {
        Kirigami.FormData.label: i18n("Recommended:")
        text: form.connectionBusy ? i18n("Setting up…") : i18n("Set up automatically")
        enabled: form.connectionLoaded && !form.connectionBusy && routerPassword.text !== ""
        onClicked: {
            const password = routerPassword.text
            routerPassword.text = ""
            form.runConnectionAction("connect", password)
        }
    }
    RowLayout {
        Kirigami.FormData.label: i18n("Manual setup:")
        QQC2.Button {
            text: i18n("Save")
            enabled: form.connectionLoaded && !form.connectionBusy
            onClicked: form.runConnectionAction("save")
        }
        QQC2.Button {
            text: i18n("Authorize key")
            enabled: form.connectionLoaded && !form.connectionBusy && routerPassword.text !== ""
            onClicked: {
                const password = routerPassword.text
                routerPassword.text = ""
                form.runConnectionAction("authorize", password)
            }
        }
        QQC2.Button {
            text: i18n("Test SSH")
            enabled: form.connectionLoaded && !form.connectionBusy
            onClicked: form.runConnectionAction("test")
        }
        QQC2.Button {
            text: i18n("Install collector")
            enabled: form.connectionLoaded && !form.connectionBusy
            onClicked: form.runConnectionAction("install")
        }
    }
    QQC2.Label {
        Kirigami.FormData.label: i18n("Status:")
        Layout.fillWidth: true
        text: form.connectionResult || i18n("Enter the router login above, then select Set up automatically.")
        color: form.connectionError ? Kirigami.Theme.negativeTextColor : Kirigami.Theme.textColor
        opacity: form.connectionError ? 1 : 0.65
        wrapMode: Text.Wrap
    }

    Item { Kirigami.FormData.isSection: true }

    QQC2.ComboBox {
        Kirigami.FormData.label: i18n("Panel also shows:")
        textRole: "text"
        valueRole: "value"
        model: [
            { text: i18n("Ping"), value: "ping" },
            { text: i18n("Devices online"), value: "clients" },
            { text: i18n("DNS blocked %"), value: "blocked" },
            { text: i18n("Nothing else"), value: "none" }
        ]
        Component.onCompleted: currentIndex = Math.max(0, indexOfValue(cfg_compactExtra))
        onActivated: cfg_compactExtra = currentValue
    }
    QQC2.CheckBox { id: panelShrink; Kirigami.FormData.label: i18n("Panel space:"); text: i18n("Shrink to fit the panel") }
    QQC2.ComboBox {
        Kirigami.FormData.label: i18n("Detail level:")
        textRole: "text"
        valueRole: "value"
        model: [
            { text: i18n("Automatic"), value: "auto" },
            { text: i18n("Label and value"), value: "full" },
            { text: i18n("Icon and value"), value: "medium" },
            { text: i18n("Value only"), value: "small" },
            { text: i18n("Indicator only"), value: "tiny" }
        ]
        Component.onCompleted: currentIndex = Math.max(0, indexOfValue(cfg_panelDetail))
        onActivated: cfg_panelDetail = currentValue
    }
    QQC2.CheckBox { id: middleClickPause; text: i18n("Middle-click pauses and resumes monitoring") }

    Item { Kirigami.FormData.isSection: true }

    QQC2.ComboBox {
        Kirigami.FormData.label: i18n("Open on:")
        visible: tabbed
        textRole: "text"
        valueRole: "value"
        enabled: !rememberTab.checked
        model: [
            { text: i18n("Overview"), value: "overview" },
            { text: i18n("Network"), value: "network" },
            { text: i18n("WiFi"), value: "wifi" },
            { text: i18n("Clients"), value: "clients" },
            { text: i18n("DNS"), value: "dns" },
            { text: i18n("Speed"), value: "speed" },
            { text: i18n("System"), value: "system" }
        ]
        Component.onCompleted: currentIndex = Math.max(0, indexOfValue(cfg_defaultTab))
        onActivated: cfg_defaultTab = currentValue
    }
    QQC2.CheckBox { id: rememberTab; visible: tabbed; text: i18n("Reopen on the last tab") }

    QQC2.CheckBox { id: showCharts; Kirigami.FormData.label: i18n("Show:"); text: i18n("History graphs") }
    QQC2.CheckBox { id: showPerCore; text: i18n("Router per-core bars") }
    QQC2.CheckBox { id: showTemps; text: i18n("Temperatures") }
    QQC2.CheckBox { id: showInterference; text: i18n("WiFi airtime") }
    QQC2.CheckBox { id: showProtection; text: i18n("AdGuard protection switch") }
    QQC2.CheckBox { id: showOpenUI; text: i18n("Open AdGuard button") }
    QQC2.CheckBox { id: showChart; text: i18n("DNS query graph") }
    QQC2.CheckBox { id: showTopLists; text: i18n("DNS top lists") }
    QQC2.CheckBox { id: showWebUI; text: i18n("Open router web UI button") }
    QQC2.CheckBox { id: showReboot; text: i18n("Reboot button") }
    QQC2.CheckBox { id: showRestartWifi; text: i18n("Restart WiFi button") }

    Item { Kirigami.FormData.isSection: true }

    RowLayout {
        Kirigami.FormData.label: i18n("Speed graph scale:")
        QQC2.SpinBox { id: maxMbps; from: 0; to: 10000; stepSize: 50 }
        QQC2.Label { text: maxMbps.value === 0 ? i18n("automatic") : i18n("Mb/s"); opacity: 0.6 }
    }
    RowLayout {
        Kirigami.FormData.label: i18n("Internet plan:")
        QQC2.SpinBox { id: planDownMbps; from: 0; to: 10000; stepSize: 50 }
        QQC2.Label { text: planDownMbps.value === 0 ? i18n("gauge tops out at 1000") : i18n("Mb/s download"); opacity: 0.6 }
    }
    QQC2.ComboBox {
        Kirigami.FormData.label: i18n("Sort clients:")
        textRole: "text"
        valueRole: "value"
        model: [
            { text: i18n("By traffic"), value: "traffic" },
            { text: i18n("By signal"), value: "signal" },
            { text: i18n("By name"), value: "name" },
            { text: i18n("By IP"), value: "ip" }
        ]
        Component.onCompleted: currentIndex = Math.max(0, indexOfValue(cfg_sortBy))
        onActivated: cfg_sortBy = currentValue
    }
    QQC2.TextField {
        id: sshUser
        Kirigami.FormData.label: i18n("Client SSH user:")
        placeholderText: i18n("For opening terminals on client devices")
    }

    Item { Kirigami.FormData.isSection: true }

    RowLayout {
        Kirigami.FormData.label: i18n("Accent colour:")
        QQC2.CheckBox {
            id: useAccent
            text: i18n("Custom")
            checked: accent.text !== ""
            onToggled: if (!checked) accent.text = ""
        }
        KQuickControls.ColorButton {
            enabled: useAccent.checked
            color: accent.text !== "" ? accent.text : Kirigami.Theme.highlightColor
            onColorChanged: if (useAccent.checked) accent.text = color
        }
        QQC2.Label { id: accent; visible: false; text: "" }
    }
}
