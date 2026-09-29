import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

QQC2.ApplicationWindow {
    id: window

    transientParent: null
    width: Kirigami.Units.gridUnit * 36
    height: Kirigami.Units.gridUnit * 40
    visible: true
    title: i18n("Kontrol Panel Settings")

    function initialValues() {
        const values = { portal: false }
        for (const key of root.config.keys())
            values["cfg_" + key] = root.config[key]
        return values
    }
    function apply() {
        const form = page.item
        for (const key of root.config.keys()) {
            const name = "cfg_" + key
            if (name in form && form[name] !== root.config[key])
                root.config[key] = form[name]
        }
    }

    QQC2.ScrollView {
        anchors.fill: parent
        anchors.bottomMargin: buttons.height
        contentWidth: availableWidth

        Loader {
            id: page
            width: parent.width
            Component.onCompleted: setSource("configGeneral.qml", window.initialValues())
        }
    }

    QQC2.DialogButtonBox {
        id: buttons
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        standardButtons: QQC2.DialogButtonBox.Ok | QQC2.DialogButtonBox.Apply | QQC2.DialogButtonBox.Cancel
        onAccepted: {
            window.apply()
            window.close()
        }
        onApplied: window.apply()
        onRejected: window.close()
    }
}
