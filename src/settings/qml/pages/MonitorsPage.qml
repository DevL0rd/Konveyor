import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import "../components"
import "../components/monitors"
import "../previews"
import "../catalog/Kdl.js" as Kdl
import "../components/monitors/MonitorSummary.js" as MonitorSummary
import org.kde.konveyor.settings

SettingsPage {
    id: page

    title: "Monitors"
    readonly property var profiles: SettingsStore.revision >= 0 ? SettingsStore.children("", "monitor-profile") : []
    readonly property var overrides: SettingsStore.revision >= 0 ? SettingsStore.children("", "output") : []
    readonly property var profileNames: profiles.map(MonitorSummary.nameOf)
    readonly property var overrideNames: overrides.map(MonitorSummary.nameOf)

    function usedBy(name) {
        return SettingsStore.live.outputs.filter(output => SettingsStore.profileForOutput(output) === name).map(output => output.name);
    }

    preview: SettingsStore.live.outputs.length > 0 ? arrangement : null

    Component {
        id: arrangement
        MonitorArrangement {}
    }

    Component.onCompleted: SettingsStore.live.refresh()

    NamePrompt {
        id: newProfile
        title: "New monitor profile"
        label: "Profile name"
        taken: page.profileNames
        onNamed: name => {
            const path = SettingsStore.append("", Kdl.block("monitor-profile", [], [name]));
            if (path.length) {
                SettingsNavigation.push("pages/MonitorProfilePage.qml", { profilePath: path });
            }
        }
    }

    CardHeader {
        title: "Monitor profiles"
        trailing: QQC2.Button {
            icon.name: "list-add"
            text: "Add profile"
            onClicked: newProfile.ask("")
        }
    }

    Card {
        QQC2.Label {
            text: "Each monitor uses the first profile it matches, from the top. A profile's layout settings apply only on the monitors that use it, and window rules can target a profile too."
            wrapMode: Text.Wrap
            opacity: 0.75
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.largeSpacing
        }

        Kirigami.PlaceholderMessage {
            visible: page.profiles.length === 0
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.gridUnit
            icon.name: "video-display"
            text: "No profiles"
            explanation: "Every monitor uses your global layout settings."
        }

        Repeater {
            model: page.profiles

            ListCard {
                required property var modelData
                required property int index
                Layout.fillWidth: true
                iconName: "video-display"
                heading: MonitorSummary.nameOf(modelData)
                subtitle: MonitorSummary.describeProfile(modelData.node)
                detail: {
                    const used = page.usedBy(heading);
                    const count = MonitorSummary.overrideCount(modelData.node);
                    return (used.length ? "Used by " + used.join(" and ") : "Not used by a connected monitor")
                        + " · " + (count === 1 ? "1 layout change" : count + " layout changes");
                }
                active: page.usedBy(heading).length > 0
                canMoveUp: index > 0
                canMoveDown: index < page.profiles.length - 1
                onOpened: SettingsNavigation.push("pages/MonitorProfilePage.qml", { profilePath: modelData.path })
                onMovedUp: SettingsStore.move(modelData.path, -1)
                onMovedDown: SettingsStore.move(modelData.path, 1)
                onDeleted: SettingsStore.remove(modelData.path)
            }
        }
    }

    CardHeader {
        title: "Per-monitor overrides"
    }

    Card {
        QQC2.Label {
            text: "Settings for one specific monitor. They win over any profile it uses."
            wrapMode: Text.Wrap
            opacity: 0.75
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.largeSpacing
        }

        Repeater {
            model: page.overrides

            ListCard {
                required property var modelData
                Layout.fillWidth: true
                iconName: "monitor"
                heading: MonitorSummary.nameOf(modelData)
                subtitle: {
                    const connected = SettingsStore.live.outputs.find(output => output.name === heading);
                    return connected ? (connected.description || "Connected") : "Not connected right now";
                }
                detail: {
                    const count = MonitorSummary.overrideCount(modelData.node);
                    const corners = (modelData.node.children || []).some(child => child.name === "hot-corners");
                    return (count === 1 ? "1 layout change" : count + " layout changes") + (corners ? " · own hot corners" : "");
                }
                active: SettingsStore.live.outputs.some(output => output.name === heading)
                showOrder: false
                onOpened: SettingsNavigation.push("pages/OutputOverridePage.qml", { outputPath: modelData.path })
                onDeleted: SettingsStore.remove(modelData.path)
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.largeSpacing
            spacing: Kirigami.Units.smallSpacing

            OutputPicker {
                id: newOutput
                value: SettingsStore.live.outputs.map(output => output.name).find(name => !page.overrideNames.includes(name)) || ""
                onPicked: name => value = name
            }

            QQC2.Button {
                icon.name: "list-add"
                text: "Add override"
                enabled: newOutput.value.length > 0 && !page.overrideNames.includes(newOutput.value)
                onClicked: {
                    const path = SettingsStore.append("", Kdl.block("output", [], [newOutput.value]));
                    if (path.length) {
                        SettingsNavigation.push("pages/OutputOverridePage.qml", { outputPath: path });
                    }
                }
            }

            Item {
                Layout.fillWidth: true
            }
        }
    }
}
