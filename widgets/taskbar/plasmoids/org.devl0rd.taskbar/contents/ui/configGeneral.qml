import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.plasma.plasma5support as P5Support

Kirigami.FormLayout {
    id: form

    property string title
    property int calls: 0

    function openSettings() {
        service.connectSource("kcmshell6 kcm_konveyor --args taskbar # " + (++form.calls))
    }

    P5Support.DataSource {
        id: service
        engine: "executable"
        onNewData: source => disconnectSource(source)
    }

    QQC2.Label {
        text: i18n("The taskbar's settings and pinned apps live in Konveyor Settings, on the Taskbar page, with a preview of each one.")
        wrapMode: Text.Wrap
        Layout.maximumWidth: Kirigami.Units.gridUnit * 24
    }

    QQC2.Button {
        id: open
        text: i18n("Open Konveyor Settings…")
        icon.name: "configure"
        onClicked: form.openSettings()
    }
}
