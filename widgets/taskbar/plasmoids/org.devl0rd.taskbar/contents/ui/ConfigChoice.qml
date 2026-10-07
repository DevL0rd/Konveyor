import QtQuick
import QtQuick.Controls as QQC2

QQC2.ComboBox {
    id: control

    required property Item page
    required property string key
    property bool flag: false

    function bind() {
        currentIndex = Qt.binding(() => control.flag ? (control.page["cfg_" + control.key] ? 1 : 0) : Number(control.page["cfg_" + control.key]))
    }

    Component.onCompleted: bind()
    onActivated: index => {
        page["cfg_" + key] = flag ? index === 1 : index
        bind()
    }
}
