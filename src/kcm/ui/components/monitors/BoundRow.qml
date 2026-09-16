import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import ".."
import "MonitorSummary.js" as MonitorSummary

SettingRow {
    id: row

    property var bound
    property bool aspect: false
    property real fallback: aspect ? 2 : 1920
    signal changed(var value)

    readonly property bool isSet: bound !== undefined && bound !== null

    wideControl: aspect

    RowLayout {
        width: row.aspect ? parent.width : implicitWidth
        spacing: Kirigami.Units.smallSpacing

        Segmented {
            currentValue: row.isSet ? "set" : "any"
            options: [
                { value: "any", label: "Any" },
                { value: "set", label: "Set" }
            ]
            onChosen: value => row.changed(value === "any" ? null : row.fallback)
        }

        QQC2.Slider {
            id: slider
            visible: row.aspect && row.isSet
            from: 1
            to: 4
            stepSize: 0.01
            value: row.isSet ? row.bound : row.fallback
            Layout.fillWidth: true
            Layout.minimumWidth: Kirigami.Units.gridUnit * 8
            onPressedChanged: {
                if (!pressed) {
                    row.changed(Number(value.toFixed(2)));
                }
            }
        }

        QQC2.Label {
            visible: slider.visible
            text: MonitorSummary.ratioLabel(slider.value)
            font.bold: true
            Layout.preferredWidth: Kirigami.Units.gridUnit * 3.5
        }

        Repeater {
            model: row.aspect && row.isSet ? MonitorSummary.ratios : []

            QQC2.ToolButton {
                required property var modelData
                text: modelData.label
                checkable: true
                checked: Math.abs(slider.value - modelData.value) < 0.04
                onClicked: row.changed(Number(modelData.value.toFixed(2)))
            }
        }

        QQC2.SpinBox {
            visible: !row.aspect && row.isSet
            from: 1
            to: 100000
            stepSize: 10
            editable: true
            value: row.isSet ? row.bound : row.fallback
            textFromValue: (number, locale) => number + " px"
            valueFromText: (text, locale) => parseInt(text)
            onValueModified: row.changed(value)
        }
    }
}
