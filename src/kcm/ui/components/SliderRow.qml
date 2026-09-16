import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

SettingRow {
    id: row

    property real value
    property real from: 0
    property real to: 100
    property real stepSize: 1
    property int decimals: 0
    property string unit
    property string lowLabel
    property string highLabel
    signal edited(real value)

    function format(number) {
        return Number(number).toFixed(row.decimals) + (row.unit.length ? " " + row.unit : "");
    }

    Timer {
        id: commit
        interval: 120
        property real pending
        onTriggered: row.edited(pending)
    }

    function schedule(number) {
        commit.pending = number;
        commit.restart();
    }

    RowLayout {
        spacing: Kirigami.Units.smallSpacing

        QQC2.Label {
            text: row.lowLabel
            visible: text.length > 0
            opacity: 0.7
            font: Kirigami.Theme.smallFont
        }

        QQC2.Slider {
            id: slider
            from: row.from
            to: row.to
            stepSize: row.stepSize
            value: row.value
            Layout.preferredWidth: Kirigami.Units.gridUnit * 10
            onMoved: row.schedule(value)
        }

        QQC2.Label {
            text: row.highLabel
            visible: text.length > 0
            opacity: 0.7
            font: Kirigami.Theme.smallFont
        }

        QQC2.SpinBox {
            id: spin
            readonly property real factor: Math.pow(10, row.decimals)
            from: Math.round(row.from * factor)
            to: Math.round(row.to * factor)
            stepSize: Math.max(1, Math.round(row.stepSize * factor))
            value: Math.round(slider.value * factor)
            editable: true
            textFromValue: (number, locale) => row.format(number / factor)
            valueFromText: (text, locale) => Math.round(parseFloat(text) * factor)
            onValueModified: row.schedule(value / factor)
            Layout.preferredWidth: Kirigami.Units.gridUnit * 6
        }
    }
}
