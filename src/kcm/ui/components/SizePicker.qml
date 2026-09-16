import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

RowLayout {
    id: picker

    property var size
    property bool allowAuto: true
    property string autoLabel: "App decides"
    property var presets: []
    signal edited(var size)

    readonly property string kind: size ? size.kind : "auto"

    spacing: Kirigami.Units.smallSpacing

    Segmented {
        currentValue: picker.kind
        options: (picker.allowAuto ? [{ value: "auto", label: picker.autoLabel }] : []).concat([
            { value: "proportion", label: "% of screen" },
            { value: "fixed", label: "Pixels" }
        ])
        onChosen: value => {
            if (value === "auto") {
                picker.edited(null);
            } else if (value === "proportion") {
                picker.edited({ kind: "proportion", value: picker.size && picker.size.kind === "proportion" ? picker.size.value : 0.5 });
            } else {
                picker.edited({ kind: "fixed", value: picker.size && picker.size.kind === "fixed" ? picker.size.value : 1000 });
            }
        }
    }

    QQC2.SpinBox {
        visible: picker.kind === "proportion"
        from: 5
        to: 100
        stepSize: 5
        editable: true
        value: picker.size && picker.size.kind === "proportion" ? Math.round(picker.size.value * 100) : 50
        textFromValue: (number, locale) => number + "%"
        valueFromText: (text, locale) => parseInt(text)
        onValueModified: picker.edited({ kind: "proportion", value: value / 100 })
    }

    QQC2.SpinBox {
        visible: picker.kind === "fixed"
        from: 100
        to: 16000
        stepSize: 50
        editable: true
        value: picker.size && picker.size.kind === "fixed" ? picker.size.value : 1000
        textFromValue: (number, locale) => number + " px"
        valueFromText: (text, locale) => parseInt(text)
        onValueModified: picker.edited({ kind: "fixed", value: value })
    }

    Repeater {
        model: picker.kind === "proportion" ? picker.presets.filter(preset => preset.kind === "proportion") : []

        QQC2.ToolButton {
            required property var modelData
            text: Math.round(modelData.value * 100) + "%"
            checkable: true
            checked: picker.size && Math.abs(picker.size.value - modelData.value) < 0.001
            onClicked: picker.edited({ kind: "proportion", value: modelData.value })
        }
    }
}
