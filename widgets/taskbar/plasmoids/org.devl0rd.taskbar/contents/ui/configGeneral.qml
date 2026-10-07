import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

Kirigami.FormLayout {
    property string title: i18n("General")
    property alias cfg_groupMode: grouping.currentIndex
    property alias cfg_showWorkspaces: workspaces.checked
    property alias cfg_placePinnedLaunches: placement.checked
    property alias cfg_showShortcutBadges: badges.checked
    property var cfg_launchers
    property var cfg_launchersDefault
    property var cfg_groupModeDefault
    property var cfg_showWorkspacesDefault
    property var cfg_placePinnedLaunchesDefault
    property var cfg_showShortcutBadgesDefault

    QQC2.ComboBox {
        id: grouping
        Kirigami.FormData.label: i18n("Group windows:")
        model: [i18n("Like Konveyor's group-app-windows"), i18n("Always group an app's neighbouring columns"), i18n("One icon per column")]
    }

    QQC2.CheckBox {
        id: workspaces
        Kirigami.FormData.label: i18n("Show:")
        text: i18n("Workspaces of this screen")
    }

    QQC2.CheckBox {
        id: badges
        text: i18n("Shortcut numbers while Meta is held")
    }

    QQC2.CheckBox {
        id: placement
        Kirigami.FormData.label: i18n("Pinned apps:")
        text: i18n("Open new windows in pin order")
    }
}
