import QtQuick
import QtQuick.Controls as QQC2

QQC2.CheckBox {
    id: control

    required property Item page
    required property string key

    function bind() {
        checked = Qt.binding(() => !!control.page["cfg_" + control.key])
    }

    Component.onCompleted: bind()
    onToggled: {
        page["cfg_" + key] = checked
        bind()
    }
}
