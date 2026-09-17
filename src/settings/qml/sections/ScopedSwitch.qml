import QtQuick
import QtQuick.Controls as QQC2

QQC2.Switch {
    id: control

    property bool isOn
    signal switched(bool on)

    checked: isOn
    onToggled: switched(checked)
}
