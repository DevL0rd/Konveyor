import QtQuick
import org.kde.konveyor.settings

Item {
    id: settingsPage

    readonly property var sections: [view.rail]

    function undo() {
        SettingsStore.undo()
    }

    SettingsView {
        id: view
        anchors.fill: parent
        pageId: launcherData.settingsTarget.page
        reveal: launcherData.settingsTarget.label || ""
        section: launcherData.settingsTarget.section
        active: launcher.shown && settingsPage.visible
        onPageChosen: id => launcherData.settingsTarget = { page: id, section: "", label: "" }
        onRevealed: launcherData.settingsTarget = { page: view.current.id, section: "", label: "" }
        onConfigFileOpened: launcher.hide()
    }
}
