import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import "../components"
import "../previews"
import org.kde.konveyor.settings

SettingsPage {
    id: page

    readonly property var named: SettingsStore.revision >= 0 ? SettingsStore.children("", "workspace") : []
    readonly property var names: named.map(entry => String((entry.node.args || [])[0] || ""))
    readonly property var layoutValues: SettingsStore.revision >= 0 ? SettingsStore.scope("layout") : ({})
    property string renamePath

    title: "Workspaces"
    preview: WorkspaceStack {
        names: page.names
        outputs: page.named.map(entry => {
            const child = (entry.node.children || []).find(node => node.name === "open-on-output");
            return child ? String(child.args[0]) : "";
        })
        emptyAbove: page.layoutValues["empty-workspace-above-first"] === true
        backAndForth: SettingsStore.values["input/workspace-auto-back-and-forth"] === true
    }

    CardHeader {
        title: "Switching"
    }

    Card {
        SwitchRow {
            label: "Go back when switching to the current workspace"
            description: "Pressing the shortcut for the workspace you're already on returns you to the one you came from."
            iconName: "go-previous"
            resetPaths: ["input/workspace-auto-back-and-forth"]
            isOn: SettingsStore.values["input/workspace-auto-back-and-forth"] === true
            onSwitched: on => SettingsStore.setFlag("input/workspace-auto-back-and-forth", on)
        }

        SwitchRow {
            label: "Keep an empty workspace above the first"
            description: "There's always a blank workspace at the top to start something new, as well as at the bottom."
            iconName: "list-add"
            resetPaths: ["layout/empty-workspace-above-first"]
            isOn: page.layoutValues["empty-workspace-above-first"] === true
            onSwitched: on => SettingsStore.setFlag("layout/empty-workspace-above-first", on)
        }
    }

    CardHeader {
        title: "Named workspaces"
        actions: [
            Kirigami.Action {
                text: "Add named workspace"
                icon.name: "list-add"
                onTriggered: {
                    page.renamePath = "";
                    nameDialog.openWith("");
                }
            }
        ]
    }

    Card {
        visible: page.named.length === 0

        Kirigami.PlaceholderMessage {
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.gridUnit
            icon.name: "virtual-desktops"
            text: "No named workspaces"
            explanation: "Give a workspace a name so it always exists and you can send apps to it with window rules or jump to it with a shortcut."
            helpfulAction: Kirigami.Action {
                text: "Add named workspace"
                icon.name: "list-add"
                onTriggered: {
                    page.renamePath = "";
                    nameDialog.openWith("");
                }
            }
        }
    }

    Repeater {
        model: page.named

        ColumnLayout {
            id: slot
            required property var modelData
            required property int index
            Layout.fillWidth: true
            Layout.bottomMargin: Kirigami.Units.largeSpacing * 2
            spacing: 0

            NamedWorkspaceCard {
                entry: slot.modelData
                index: slot.index
                count: page.named.length
                onRenameRequested: (path, name) => {
                    page.renamePath = path;
                    nameDialog.openWith(name);
                }
            }
        }
    }

    WorkspaceNameDialog {
        id: nameDialog
        takenNames: page.names
        onNameChosen: name => {
            if (page.renamePath.length) {
                SettingsStore.setNode(page.renamePath, { name: "workspace", args: [name], props: {} });
            } else {
                SettingsStore.append("", { name: "workspace", args: [name], props: {} });
            }
        }
    }
}
