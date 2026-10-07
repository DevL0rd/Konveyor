import QtQuick
import QtQuick.Controls as QQC2

QQC2.SpinBox {
    id: control

    required property Item page
    required property string key

    function bind() {
        value = Qt.binding(() => Number(control.page["cfg_" + control.key]))
    }

    editable: true
    textFromValue: number => i18n("%1 px", number)
    valueFromText: text => parseInt(text)
    Component.onCompleted: bind()
    onValueModified: {
        page["cfg_" + key] = value
        bind()
    }
}
