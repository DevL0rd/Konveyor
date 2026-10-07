import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import "../components"
import "../previews"
import "../sections"
import org.kde.konveyor.settings

SettingsPage {
    id: page

    title: "Taskbar"
    preview: TaskbarPreview {
        settings: TaskbarSettings.values
    }

    CardHeader {
        title: "What it shows"
    }

    Card {
        TaskbarSwitchRow {
            key: "showApps"
            label: "Show apps"
            description: TaskbarSettings.values.showWorkspaces === false ? "The taskbar needs to show something, so turn on the workspaces first to hide the apps." : "An icon for each window, in the order of your columns."
            iconName: "view-list-icons"
            enabled: !isOn || TaskbarSettings.values.showWorkspaces !== false
        }

        TaskbarSwitchRow {
            key: "showWorkspaces"
            label: "Show workspaces"
            description: TaskbarSettings.values.showApps === false ? "The taskbar needs to show something, so turn on the apps first to hide the workspaces." : "A strip with the workspaces of the panel's screen. Click one to switch."
            iconName: "virtual-desktops"
            enabled: !isOn || TaskbarSettings.values.showApps !== false
        }
    }

    TaskbarLookSection {}

    TaskbarWorkspacesSection {}

    TaskbarWindowsSection {}

    TaskbarPinsSection {}
}
