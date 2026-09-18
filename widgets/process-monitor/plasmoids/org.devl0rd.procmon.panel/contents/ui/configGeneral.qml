import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.FormLayout {
    property alias cfg_panelIcon: iconField.text
    property alias cfg_updateInterval: intervalSpin.value
    property alias cfg_showKernelThreads: kernelCheck.checked
    property alias cfg_aggregateChildren: aggregateCheck.checked
    property alias cfg_hideSystemd: systemdCheck.checked
    property alias cfg_colorizeUsage: colorCheck.checked
    property alias cfg_showGpuColumn: gpuColumn.checked
    property alias cfg_showDecColumn: decColumn.checked
    property alias cfg_showEncColumn: encColumn.checked
    property alias cfg_showVramColumn: vramColumn.checked
    property alias cfg_showDiskColumn: diskColumn.checked
    property alias cfg_showThreadsColumn: threadsColumn.checked
    property alias cfg_showPidColumn: pidColumn.checked
    property string cfg_sortColumn
    property bool cfg_sortDescending
    property alias cfg_compactMaxWidth: nameWidth.value
    property alias cfg_compactShowCpu: compactCpu.checked
    property alias cfg_compactShowGpu: compactGpu.checked
    property alias cfg_compactShowFps: compactFps.checked
    property alias cfg_panelShrink: panelShrink.checked
    property string cfg_panelDetail
    property alias cfg_showFocusedCard: focusedCard.checked
    property alias cfg_fpsGood: fpsGood.value
    property alias cfg_fpsWarn: fpsWarn.value
    property bool cfg_treeView
    property string cfg_processFilter

    QQC2.CheckBox { id: compactCpu; Kirigami.FormData.label: i18n("Panel shows:"); text: i18n("CPU of the focused app") }
    QQC2.CheckBox { id: compactGpu; text: i18n("GPU of the focused app") }
    QQC2.CheckBox { id: compactFps; text: i18n("Frame rate, when the app reports one") }
    RowLayout {
        Kirigami.FormData.label: i18n("App name width:")
        QQC2.SpinBox { id: nameWidth; from: 0; to: 30 }
        QQC2.Label { text: nameWidth.value === 0 ? i18n("hidden") : i18n("grid units"); opacity: 0.6 }
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
    RowLayout {
        Kirigami.FormData.label: i18n("Frame rate colours:")
        QQC2.Label { text: i18n("green from"); opacity: 0.7 }
        QQC2.SpinBox { id: fpsGood; from: 1; to: 500 }
        QQC2.Label { text: i18n("amber from"); opacity: 0.7 }
        QQC2.SpinBox { id: fpsWarn; from: 1; to: 500 }
    }

    Item { Kirigami.FormData.isSection: true }

    QQC2.CheckBox { id: focusedCard; Kirigami.FormData.label: i18n("Popup:"); text: i18n("Show the focused app card") }
    QQC2.CheckBox { id: aggregateCheck; text: i18n("Include child processes in totals") }
    QQC2.CheckBox { id: systemdCheck; text: i18n("Hide systemd and start from its children") }
    QQC2.CheckBox { id: kernelCheck; text: i18n("Show kernel threads") }
    QQC2.CheckBox { id: colorCheck; text: i18n("Colour usage from green to red") }

    QQC2.CheckBox { id: gpuColumn; Kirigami.FormData.label: i18n("Columns:"); text: i18n("GPU") }
    QQC2.CheckBox { id: decColumn; text: i18n("Video decode") }
    QQC2.CheckBox { id: encColumn; text: i18n("Video encode") }
    QQC2.CheckBox { id: vramColumn; text: i18n("VRAM") }
    QQC2.CheckBox { id: diskColumn; text: i18n("Disk") }
    QQC2.CheckBox { id: threadsColumn; text: i18n("Threads") }
    QQC2.CheckBox { id: pidColumn; text: i18n("PID") }

    Item { Kirigami.FormData.isSection: true }

    RowLayout {
        Kirigami.FormData.label: i18n("Panel icon:")
        QQC2.TextField { id: iconField; placeholderText: i18n("icon name") }
    }
    RowLayout {
        Kirigami.FormData.label: i18n("Refresh interval:")
        QQC2.SpinBox { id: intervalSpin; from: 500; to: 10000; stepSize: 250 }
        QQC2.Label { text: i18n("ms"); opacity: 0.6 }
    }
    QQC2.Label {
        Kirigami.FormData.label: i18n("Frame rates:")
        text: i18n("Games and apps report their frame rate when they run with MangoHud. install.sh sets MangoHud up for this.")
        opacity: 0.6
        wrapMode: Text.Wrap
        Layout.maximumWidth: Kirigami.Units.gridUnit * 20
    }
}
