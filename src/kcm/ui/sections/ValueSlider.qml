import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

RowLayout {
    id: control

    property real value
    property real from: 0
    property real to: 100
    property real stepSize: 1
    property int decimals: 0
    property string unit
    property real sliderWidth: Kirigami.Units.gridUnit * 9
    signal edited(real value)

    spacing: Kirigami.Units.smallSpacing

    Timer {
        id: commit
        interval: 120
        property real pending
        onTriggered: control.edited(pending)
    }

    function schedule(number) {
        commit.pending = number;
        commit.restart();
    }

    QQC2.Slider {
        id: slider
        from: control.from
        to: control.to
        stepSize: control.stepSize
        value: control.value
        Layout.preferredWidth: control.sliderWidth
        onMoved: control.schedule(value)
    }

    QQC2.SpinBox {
        readonly property real factor: Math.pow(10, control.decimals)
        from: Math.round(control.from * factor)
        to: Math.round(control.to * factor)
        stepSize: Math.max(1, Math.round(control.stepSize * factor))
        value: Math.round(slider.value * factor)
        editable: true
        textFromValue: (number, locale) => Number(number / factor).toFixed(control.decimals) + (control.unit.length ? " " + control.unit : "")
        valueFromText: (text, locale) => Math.round(parseFloat(text) * factor)
        onValueModified: control.schedule(value / factor)
        Layout.preferredWidth: Kirigami.Units.gridUnit * 5.5
    }
}
