import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.iconthemes as KIconThemes

Kirigami.FormLayout {
    id: form

    property string cfg_icon
    property alias cfg_label: labelField.text
    property alias cfg_showLabel: showLabel.checked
    property alias cfg_widthFraction: widthSlider.value
    property alias cfg_heightFraction: heightSlider.value
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

    QQC2.Button {
        Kirigami.FormData.label: i18n("Icon:")
        implicitWidth: Kirigami.Units.iconSizes.large + Kirigami.Units.largeSpacing * 2
        implicitHeight: implicitWidth
        icon.name: form.cfg_icon || "start-here-kde-plasma"
        icon.width: Kirigami.Units.iconSizes.large
        icon.height: Kirigami.Units.iconSizes.large
        onClicked: iconDialog.open()
        KIconThemes.IconDialog {
            id: iconDialog
            onIconNameChanged: form.cfg_icon = iconName || "start-here-kde-plasma"
        }
    }
    QQC2.Button {
        text: i18n("Use the default Plasma icon")
        enabled: form.cfg_icon !== "start-here-kde-plasma"
        onClicked: form.cfg_icon = "start-here-kde-plasma"
    }
    RowLayout {
        Kirigami.FormData.label: i18n("Label:")
        QQC2.CheckBox { id: showLabel; text: i18n("Show") }
        QQC2.TextField { id: labelField; enabled: showLabel.checked; placeholderText: i18n("Start") }
    }

    Item { Kirigami.FormData.isSection: true }

    RowLayout {
        Kirigami.FormData.label: i18n("Width:")
        QQC2.Slider { id: widthSlider; from: 0.4; to: 0.95; stepSize: 0.01 }
        QQC2.Label { text: Math.round(widthSlider.value * 100) + "%" }
    }
    RowLayout {
        Kirigami.FormData.label: i18n("Height:")
        QQC2.Slider { id: heightSlider; from: 0.4; to: 0.95; stepSize: 0.01 }
        QQC2.Label { text: Math.round(heightSlider.value * 100) + "%" }
    }
    RowLayout {
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
}
