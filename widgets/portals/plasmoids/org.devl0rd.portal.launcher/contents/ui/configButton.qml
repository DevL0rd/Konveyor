import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.iconthemes as KIconThemes
import org.kde.plasma.plasma5support as P5Support

Kirigami.FormLayout {
    id: form

    readonly property string defaultIcon: "start-here-kde-plasma-symbolic"
    property string cfg_icon
    property alias cfg_label: labelField.text
    property alias cfg_showLabel: showLabel.checked

    P5Support.DataSource {
        id: service
        engine: "executable"
        onNewData: source => disconnectSource(source)
    }

    QQC2.Button {
        Kirigami.FormData.label: i18n("Icon:")
        implicitWidth: Kirigami.Units.iconSizes.large + Kirigami.Units.largeSpacing * 2
        implicitHeight: implicitWidth
        icon.name: form.cfg_icon || form.defaultIcon
        icon.width: Kirigami.Units.iconSizes.large
        icon.height: Kirigami.Units.iconSizes.large
        onClicked: iconDialog.open()
        KIconThemes.IconDialog {
            id: iconDialog
            onIconNameChanged: form.cfg_icon = iconName || form.defaultIcon
        }
    }
    QQC2.Button {
        text: i18n("Use the default Plasma icon")
        enabled: form.cfg_icon !== form.defaultIcon
        onClicked: form.cfg_icon = form.defaultIcon
    }
    RowLayout {
        Kirigami.FormData.label: i18n("Label:")
        QQC2.CheckBox { id: showLabel; text: i18n("Show") }
        QQC2.TextField { id: labelField; enabled: showLabel.checked; placeholderText: i18n("Start") }
    }

    Item { Kirigami.FormData.isSection: true }

    QQC2.Button {
        Kirigami.FormData.label: i18n("Kontrol Panel:")
        text: i18n("Kontrol Panel settings…")
        icon.name: "configure"
        onClicked: service.connectSource("busctl --user call org.devl0rd.KontrolPanel /KontrolPanel org.devl0rd.KontrolPanel Configure # " + Date.now())
    }
}
