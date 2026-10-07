import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import "../components"
import org.kde.konveyor.settings

ColumnLayout {
    Layout.fillWidth: true
    spacing: 0
    enabled: TaskbarSettings.values.showApps !== false

    CardHeader {
        title: "Windows"
    }

    Card {
        TaskbarChoiceRow {
            key: "groupMode"
            label: "Group an app's windows"
            description: "Windows that share a column always sit together in a capsule. Grouping joins an app's neighbouring columns into one icon; hover it for a list of its windows."
            iconName: "view-group"
            options: [
                { value: 0, title: "Like Konveyor", description: "Follows group-app-windows" },
                { value: 1, title: "Always", description: "One icon per app's neighbours" },
                { value: 2, title: "Never", description: "One icon per column" }
            ]
        }

        TaskbarSwitchRow {
            key: "onlyThisScreen"
            label: "Only windows on this screen"
            description: "Turn off to list the current workspace of every screen, with Bring to This Screen in the menu."
            iconName: "video-display"
        }

        TaskbarSwitchRow {
            key: "showFloating"
            label: "Show floating windows"
            iconName: "window-keep-above"
        }

        TaskbarSwitchRow {
            key: "showTooltips"
            label: "Tooltips"
            description: "The app's name and window titles when you hover an icon."
            iconName: "help-hint"
        }
    }

    CardHeader {
        title: "Clicks and keys"
    }

    Card {
        TaskbarChoiceRow {
            key: "activeClick"
            label: "Clicking the focused app"
            iconName: "input-mouse-click-left"
            previewFor: value => ({ activeClick: value, groupMode: 2 })
            options: [
                { value: 0, title: "Minimize it", description: "Click again to bring it back" },
                { value: 1, title: "Do nothing", description: "It stays focused" },
                { value: 2, title: "Next window", description: "Of the same app" }
            ]
        }

        SettingRow {
            label: "Middle-click"
            iconName: "input-mouse-click-middle"
            taskbarKeys: ["middleClick"]

            Segmented {
                currentValue: TaskbarSettings.values.middleClick
                options: [
                    { value: 0, label: "Close" },
                    { value: 1, label: "New window" },
                    { value: 2, label: "Nothing" }
                ]
                onChosen: value => TaskbarSettings.values.middleClick = value
            }
        }

        TaskbarSwitchRow {
            key: "wheelCyclesTasks"
            label: "Scroll over the apps to switch between them"
            iconName: "input-mouse"
        }

        TaskbarChoiceRow {
            key: "showShortcutBadges"
            label: "Shortcut numbers"
            description: "Hold Meta for the keys that switch workspaces, or Meta and Alt for the keys that jump to each app. They come from your shortcuts."
            iconName: "preferences-desktop-keyboard-shortcut"
            previewFor: value => ({ badges: value ? "columns" : "" })
            options: [
                { value: true, title: "Show them", description: "While the keys are held" },
                { value: false, title: "Hide them" }
            ]
        }
    }
}
