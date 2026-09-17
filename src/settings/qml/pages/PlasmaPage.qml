import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import "../components"
import "../previews"
import org.kde.konveyor.settings

SettingsPage {
    id: page

    title: "Plasma Integration"
    preview: PlasmaPreview {
        hideWidgets: SettingsStore.values["hide-desktop-widgets"]
        fillPanels: SettingsStore.values["fill-panels-on-maximize"]
        noMinimize: SettingsStore.values["disable-minimize"]
    }

    CardHeader {
        title: "Desktop"
    }

    Card {
        SwitchRow {
            label: "Hide desktop widgets behind windows"
            description: "Widgets fade away while a window is open on that monitor's workspace, and come back when it's empty. Needs a desktop layout that supports it."
            iconName: "preferences-desktop-plasma"
            resetPaths: ["hide-desktop-widgets"]
            isOn: SettingsStore.values["hide-desktop-widgets"]
            onSwitched: on => SettingsStore.setFlag("hide-desktop-widgets", on)
        }
    }

    CardHeader {
        title: "Panels and windows"
    }

    Card {
        SwitchRow {
            label: "Stretch panels while a window is maximized"
            description: "Floating or fit-content panels span the full screen width while a window on that monitor is maximized, then return to how you set them."
            iconName: "configure-toolbars"
            resetPaths: ["fill-panels-on-maximize"]
            isOn: SettingsStore.values["fill-panels-on-maximize"]
            onSwitched: on => SettingsStore.setFlag("fill-panels-on-maximize", on)
        }

        SwitchRow {
            label: "Don't allow minimizing"
            description: "There's no taskbar list of hidden windows in a scrolling layout, so minimized windows are easy to lose. This also removes the minimize button from title bars."
            iconName: "window-minimize"
            resetPaths: ["disable-minimize"]
            isOn: SettingsStore.values["disable-minimize"]
            onSwitched: on => SettingsStore.setFlag("disable-minimize", on)
        }
    }

    CardHeader {
        title: "Notifications"
    }

    Card {
        SwitchRow {
            label: "Notify me when the config has an error"
            description: "Shows a notification when a saved config can't be loaded. Konveyor keeps using the last working config either way."
            iconName: "dialog-warning"
            resetPaths: ["config-notification"]
            isOn: !SettingsStore.values["config-notification/disable-failed"]
            onSwitched: on => SettingsStore.setFlag("config-notification/disable-failed", !on)
        }
    }
}
