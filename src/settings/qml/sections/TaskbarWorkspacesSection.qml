import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import "../components"
import org.kde.konveyor.settings

ColumnLayout {
    Layout.fillWidth: true
    spacing: 0
    enabled: TaskbarSettings.values.showWorkspaces !== false

    CardHeader {
        title: "Workspaces"
    }

    Card {
        TaskbarChoiceRow {
            key: "workspacesAfterTasks"
            label: "Position"
            iconName: "object-columns"
            options: [
                { value: false, title: "Before the apps" },
                { value: true, title: "After the apps" }
            ]
        }

        TaskbarChoiceRow {
            key: "pillContent"
            label: "Each workspace shows"
            iconName: "virtual-desktops"
            options: [
                { value: 0, title: "Its columns", description: "A dot for each, the focused one wider" },
                { value: 1, title: "Its name", description: "Or its number when unnamed" },
                { value: 2, title: "Its number", description: "The key that switches to it" }
            ]
        }

        TaskbarSwitchRow {
            key: "showSeparator"
            label: "Line between the workspaces and the apps"
            iconName: "format-text-strikethrough"
        }

        TaskbarSwitchRow {
            key: "showEmptyWorkspaces"
            label: "Show empty workspaces"
            description: "Including the empty one Konveyor keeps at the end."
            iconName: "folder-new"
        }

        TaskbarSwitchRow {
            key: "wheelSwitchesWorkspaces"
            label: "Scroll over the workspaces to switch"
            iconName: "input-mouse"
        }
    }
}
