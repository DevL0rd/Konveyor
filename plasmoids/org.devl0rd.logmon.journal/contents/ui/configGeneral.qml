import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kquickcontrols as KQuickControls

Kirigami.FormLayout {
    property alias cfg_pollInterval: pollSpin.value
    property alias cfg_maxRows: rowsSpin.value
    property alias cfg_searchLimit: searchSpin.value
    property alias cfg_stickLines: stickSpin.value
    property alias cfg_defaultLevel: levelCombo.currentIndex
    property alias cfg_mutedApps: mutedField.text
    property alias cfg_showApp: showApp.checked
    property alias cfg_wrapMessages: wrap.checked
    property alias cfg_accentColor: accent.text
    property alias cfg_compactShowWarnings: compactShowWarnings.checked
    property alias cfg_middleClickPause: middleClickPause.checked
    property string cfg_lastSeen
    property bool cfg_showActivity

    QQC2.CheckBox {
        id: compactShowWarnings
        Kirigami.FormData.label: i18n("Panel button:")
        text: i18n("Show new warnings next to new errors")
    }
    QQC2.CheckBox { id: middleClickPause; text: i18n("Middle-click pauses and resumes following") }

    Item { Kirigami.FormData.isSection: true }


    RowLayout {
        Kirigami.FormData.label: i18n("Poll interval:")
        QQC2.SpinBox {
            id: pollSpin
            from: 100; to: 5000; stepSize: 50
            textFromValue: function(v) { return (v / 1000).toFixed(2) + " s" }
            valueFromText: function(t) { return parseFloat(t) * 1000 }
        }
    }
    RowLayout {
        Kirigami.FormData.label: i18n("Live buffer:")
        QQC2.SpinBox { id: rowsSpin; from: 100; to: 2000; stepSize: 100 }
        QQC2.Label { text: i18n("lines"); opacity: 0.6 }
    }
    RowLayout {
        Kirigami.FormData.label: i18n("Search limit:")
        QQC2.SpinBox { id: searchSpin; from: 500; to: 50000; stepSize: 500 }
        QQC2.Label { text: i18n("matching lines"); opacity: 0.6 }
    }
    RowLayout {
        Kirigami.FormData.label: i18n("Stick to bottom within:")
        QQC2.SpinBox { id: stickSpin; from: 0; to: 30; stepSize: 1 }
        QQC2.Label { text: i18n("lines"); opacity: 0.6 }
    }
    QQC2.ComboBox {
        id: levelCombo
        Kirigami.FormData.label: i18n("Open on:")
        model: [i18n("All"), i18n("Info"), i18n("Warnings"), i18n("Errors")]
    }

    Item { Kirigami.FormData.isSection: true }

    QQC2.CheckBox { id: showApp; Kirigami.FormData.label: i18n("Show:"); text: i18n("Application column") }
    QQC2.CheckBox { id: wrap; text: i18n("Wrap long messages") }

    Item { Kirigami.FormData.isSection: true }

    QQC2.TextField {
        id: mutedField
        Kirigami.FormData.label: i18n("Muted apps:")
        Layout.preferredWidth: Kirigami.Units.gridUnit * 16
        placeholderText: i18n("comma-separated, e.g. discord, pipewire")
    }
    QQC2.Label {
        text: i18n("Hidden from the log. Right-click a line → Mute to add one.")
        font: Kirigami.Theme.smallFont; opacity: 0.6
    }

    Item { Kirigami.FormData.isSection: true }

    RowLayout {
        Kirigami.FormData.label: i18n("Accent colour:")
        QQC2.CheckBox {
            id: useAccent; text: i18n("Custom")
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
