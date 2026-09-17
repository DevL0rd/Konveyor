import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.iconthemes as KIconThemes
import org.kde.plasma.plasma5support as P5Support
import org.kde.plasma.plasmoid

Kirigami.FormLayout {
    id: form

    property string cfg_icon
    property alias cfg_label: labelField.text
    property alias cfg_showLabel: showLabel.checked
    property alias cfg_cardWidth: widthBox.value
    property alias cfg_cardHeight: heightBox.value
    property alias cfg_dimStrength: dimSlider.value
    property string cfg_defaultPage
    property alias cfg_tileSize: tileSize.value
    property alias cfg_showRecentApps: showRecentApps.checked
    property alias cfg_showRecentFiles: showRecentFiles.checked
    property alias cfg_showGames: showGames.checked
    property alias cfg_showFriends: showFriends.checked
    property alias cfg_searchFiles: searchFiles.checked
    property alias cfg_searchSettings: searchSettings.checked
    property alias cfg_searchCalculator: searchCalculator.checked
    property alias cfg_searchCommands: searchCommands.checked
    property alias cfg_searchWeb: searchWeb.checked
    property alias cfg_searchPackages: searchPackages.checked
    property string cfg_gamesView
    property string cfg_gamesSort
    property int cfg_gameCardSize
    property string cfg_appsSort
    property string cfg_appsView
    property alias cfg_searchWindows: searchWindows.checked
    property var hiddenApps: []

    P5Support.DataSource {
        id: hiddenSource
        engine: "executable"
        onNewData: function(source, result) {
            disconnectSource(source)
            if (source.indexOf("--hidden") < 0) {
                form.loadHidden()
                return
            }
            try {
                form.hiddenApps = JSON.parse(result.stdout || "[]")
            } catch (error) {
                form.hiddenApps = []
            }
        }
    }
    function loadHidden() {
        hiddenSource.connectSource("$HOME/.local/bin/portal-games --hidden # " + Date.now())
    }
    function unhide(id) {
        hiddenSource.connectSource("$HOME/.local/bin/portal-games --unhide '" + String(id).replace(/'/g, "'\\''") + "'")
    }
    Component.onCompleted: loadHidden()
    property string cfg_learnedRanking
    property alias cfg_popupWidth: popupWidthBox.value
    property alias cfg_popupHeight: popupHeightBox.value
    property alias cfg_showFriendsBadge: friendsBadge.checked
    readonly property bool portal: Plasmoid.metaData.pluginId === "org.devl0rd.portal"
    readonly property string defaultIcon: portal ? "view-app-grid-symbolic" : "start-here-kde-plasma-symbolic"
    property string cfg_searchOrder

    readonly property var groupLabels: ({
        answer: i18n("Answers (calculator, units)"),
        apps: i18n("Applications"),
        games: i18n("Games"),
        windows: i18n("Open windows"),
        settings: i18n("System Settings"),
        files: i18n("Files and places"),
        friends: i18n("Friends"),
        commands: i18n("Commands"),
        other: i18n("Everything else")
    })
    readonly property var orderList: {
        const known = ["answer", "apps", "games", "windows", "settings", "files", "friends", "commands", "other"]
        const wanted = String(form.cfg_searchOrder || "").split(",").map(key => key.trim()).filter(key => known.indexOf(key) >= 0)
        for (const key of known) {
            if (wanted.indexOf(key) < 0)
                wanted.push(key)
        }
        return wanted
    }
    function moveGroup(index, delta) {
        const list = orderList.slice()
        const target = index + delta
        if (target < 0 || target >= list.length)
            return
        const item = list.splice(index, 1)[0]
        list.splice(target, 0, item)
        form.cfg_searchOrder = list.join(",")
    }

    QQC2.Button {
        Kirigami.FormData.label: i18n("Icon:")
        implicitWidth: Kirigami.Units.iconSizes.large + Kirigami.Units.largeSpacing * 2
        implicitHeight: implicitWidth
        icon.name: form.cfg_icon || form.defaultIcon
        icon.width: Kirigami.Units.iconSizes.large
        icon.height: Kirigami.Units.iconSizes.large
        onClicked: iconDialog.open()
        KIconThemes.IconDialog {
            id: iconDialog
            onIconNameChanged: form.cfg_icon = iconName || form.defaultIcon
        }
    }
    QQC2.Button {
        text: form.portal ? i18n("Use the default icon") : i18n("Use the default Plasma icon")
        enabled: form.cfg_icon !== form.defaultIcon
        onClicked: form.cfg_icon = form.defaultIcon
    }
    QQC2.CheckBox {
        id: friendsBadge
        visible: form.portal
        text: i18n("Show how many friends are in game on the panel icon")
    }
    RowLayout {
        visible: !form.portal
        Kirigami.FormData.label: i18n("Label:")
        QQC2.CheckBox { id: showLabel; text: i18n("Show") }
        QQC2.TextField { id: labelField; enabled: showLabel.checked; placeholderText: i18n("Start") }
    }

    Item { Kirigami.FormData.isSection: true }

    RowLayout {
        visible: form.portal
        Kirigami.FormData.label: i18n("Popup width:")
        QQC2.SpinBox { id: popupWidthBox; from: 26; to: 120 }
        QQC2.Label { text: i18n("grid units"); opacity: 0.6 }
    }
    RowLayout {
        visible: form.portal
        Kirigami.FormData.label: i18n("Popup height:")
        QQC2.SpinBox { id: popupHeightBox; from: 20; to: 90 }
        QQC2.Label { text: i18n("grid units"); opacity: 0.6 }
    }
    RowLayout {
        visible: !form.portal
        Kirigami.FormData.label: i18n("Width:")
        QQC2.SpinBox { id: widthBox; from: 50; to: 160 }
        QQC2.Label { text: i18n("grid units, never wider than the screen"); opacity: 0.6 }
    }
    RowLayout {
        visible: !form.portal
        Kirigami.FormData.label: i18n("Height:")
        QQC2.SpinBox { id: heightBox; from: 30; to: 100 }
        QQC2.Label { text: i18n("grid units, never taller than the screen"); opacity: 0.6 }
    }
    RowLayout {
        visible: !form.portal
        Kirigami.FormData.label: i18n("Dim the desktop:")
        QQC2.Slider { id: dimSlider; from: 0; to: 0.9; stepSize: 0.05 }
        QQC2.Label { text: Math.round(dimSlider.value * 100) + "%" }
    }
    QQC2.SpinBox {
        id: tileSize
        Kirigami.FormData.label: i18n("App icon size:")
        from: 32
        to: 96
        stepSize: 4
    }
    QQC2.ComboBox {
        Kirigami.FormData.label: i18n("Open on:")
        textRole: "text"
        valueRole: "value"
        model: [
            { text: i18n("Home"), value: "home" },
            { text: i18n("Apps"), value: "apps" },
            { text: i18n("Games"), value: "games" },
            { text: i18n("Files"), value: "files" },
            { text: i18n("Friends"), value: "friends" }
        ]
        Component.onCompleted: currentIndex = Math.max(0, indexOfValue(form.cfg_defaultPage))
        onActivated: form.cfg_defaultPage = currentValue
    }

    Item { Kirigami.FormData.isSection: true }

    QQC2.CheckBox { id: showRecentApps; Kirigami.FormData.label: i18n("Home shows:"); text: i18n("Recently used apps") }
    QQC2.CheckBox { id: showRecentFiles; text: i18n("Recent files") }
    QQC2.CheckBox { id: showGames; Kirigami.FormData.label: i18n("Steam:"); text: i18n("Games page and Continue playing") }
    QQC2.CheckBox { id: showFriends; text: i18n("Friends page and friend presence") }

    Item { Kirigami.FormData.isSection: true }

    QQC2.CheckBox { id: searchFiles; Kirigami.FormData.label: i18n("Search also finds:"); text: i18n("Files, folders and places") }
    QQC2.CheckBox { id: searchSettings; text: i18n("System Settings pages") }
    QQC2.CheckBox { id: searchCalculator; text: i18n("Calculations and unit conversions") }
    QQC2.CheckBox { id: searchCommands; text: i18n("Shell commands") }
    QQC2.CheckBox { id: searchWeb; text: i18n("Web search shortcuts") }
    QQC2.CheckBox { id: searchPackages; text: i18n("Packages to install with Shelly (always listed last)") }
    QQC2.CheckBox { id: searchWindows; text: i18n("Open windows") }

    ColumnLayout {
        Kirigami.FormData.label: i18n("Result order:")
        spacing: 0
        Repeater {
            model: form.orderList
            RowLayout {
                required property string modelData
                required property int index
                QQC2.Label {
                    Layout.preferredWidth: Kirigami.Units.gridUnit * 12
                    text: (index + 1) + ".  " + (form.groupLabels[modelData] || modelData)
                }
                QQC2.ToolButton {
                    icon.name: "go-up"
                    enabled: index > 0
                    onClicked: form.moveGroup(index, -1)
                }
                QQC2.ToolButton {
                    icon.name: "go-down"
                    enabled: index < form.orderList.length - 1
                    onClicked: form.moveGroup(index, 1)
                }
            }
        }
        QQC2.Label {
            text: i18n("Best match always comes first and Shelly packages always last.")
            opacity: 0.6
        }
    }
    RowLayout {
        Kirigami.FormData.label: i18n("Learned results:")
        QQC2.Button {
            text: i18n("Forget what I usually open")
            enabled: form.cfg_learnedRanking !== "" && form.cfg_learnedRanking !== "{}"
            onClicked: form.cfg_learnedRanking = ""
        }
    }

    Item { Kirigami.FormData.isSection: true }

    QQC2.ComboBox {
        Kirigami.FormData.label: i18n("Apps sorted by:")
        textRole: "text"
        valueRole: "value"
        model: [
            { text: i18n("Name"), value: "name" },
            { text: i18n("Most used"), value: "popular" },
            { text: i18n("Recently used"), value: "recent" },
            { text: i18n("Recently installed"), value: "installed" }
        ]
        Component.onCompleted: currentIndex = Math.max(0, indexOfValue(form.cfg_appsSort))
        onActivated: form.cfg_appsSort = currentValue
    }
    QQC2.ComboBox {
        Kirigami.FormData.label: i18n("Apps shown as:")
        textRole: "text"
        valueRole: "value"
        model: [
            { text: i18n("Grid"), value: "grid" },
            { text: i18n("List"), value: "list" }
        ]
        Component.onCompleted: currentIndex = Math.max(0, indexOfValue(form.cfg_appsView))
        onActivated: form.cfg_appsView = currentValue
    }
    ColumnLayout {
        Kirigami.FormData.label: i18n("Hidden apps:")
        spacing: 0
        QQC2.Label {
            visible: form.hiddenApps.length === 0
            text: i18n("None. Right-click an app and choose Hide from launcher. Hidden apps are shared with App Portal.")
            opacity: 0.6
        }
        Repeater {
            model: form.hiddenApps
            RowLayout {
                required property string modelData
                QQC2.Label {
                    Layout.preferredWidth: Kirigami.Units.gridUnit * 12
                    text: modelData
                    elide: Text.ElideRight
                }
                QQC2.Button {
                    text: i18n("Unhide")
                    icon.name: "view-visible"
                    onClicked: form.unhide(modelData)
                }
            }
        }
    }
}
