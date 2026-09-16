import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import "../components"
import "../components/monitors"
import "../previews"
import "../sections"
import "../catalog/Kdl.js" as Kdl
import "../components/monitors/MonitorSummary.js" as MonitorSummary

SettingsPage {
    id: page

    property string profilePath
    readonly property var profile: kcm.revision >= 0 ? kcm.node(profilePath) : ({})
    readonly property bool exists: profile.name !== undefined
    readonly property string profileName: exists && profile.args.length ? String(profile.args[0]) : ""
    readonly property var matches: kcm.revision >= 0 ? kcm.children(profilePath, "match") : []
    readonly property var usedBy: kcm.live.outputs.filter(output => kcm.revision >= 0 && kcm.profileForOutput(output) === profileName)
    readonly property var names: kcm.revision >= 0 ? kcm.children("", "monitor-profile").map(MonitorSummary.nameOf) : []

    title: exists ? "Profile: " + profileName : "Monitor profile"
    preview: kcm.live.outputs.length > 0 ? arrangement : null

    Component {
        id: arrangement
        MonitorArrangement {
            highlightProfile: page.profileName
        }
    }

    Component.onCompleted: kcm.live.refresh()

    NamePrompt {
        id: rename
        title: "Rename profile"
        label: "Profile name"
        taken: page.names
        onNamed: name => kcm.setNode(page.profilePath, { name: "monitor-profile", args: [name], props: {} })
    }

    Kirigami.PlaceholderMessage {
        visible: !page.exists
        Layout.fillWidth: true
        Layout.margins: Kirigami.Units.gridUnit * 2
        icon.name: "dialog-question"
        text: "This profile no longer exists"
    }

    Card {
        visible: page.exists
        Layout.topMargin: Kirigami.Units.gridUnit

        RowLayout {
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.largeSpacing
            spacing: Kirigami.Units.largeSpacing

            ColumnLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing

                Kirigami.Heading {
                    level: 3
                    text: page.profileName
                }

                QQC2.Label {
                    text: page.usedBy.length
                        ? "In use on " + page.usedBy.map(output => output.name).join(" and ")
                        : (kcm.live.running ? "No connected monitor uses this profile right now" : "Connected monitors show here while Konveyor is running")
                    wrapMode: Text.Wrap
                    opacity: 0.75
                    Layout.fillWidth: true
                }
            }

            QQC2.Button {
                icon.name: "edit-rename"
                text: "Rename"
                onClicked: rename.ask(page.profileName)
                QQC2.ToolTip.text: "Window rules that target this profile by name need the new name too"
                QQC2.ToolTip.visible: hovered
            }
        }
    }

    CardHeader {
        visible: page.exists
        title: "Which monitors"
    }

    Card {
        visible: page.exists

        QQC2.Label {
            visible: page.matches.length === 0
            text: "Matches every monitor that an earlier profile didn't already claim. Add a condition to narrow it down."
            wrapMode: Text.Wrap
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.largeSpacing
        }

        Repeater {
            model: page.matches.length

            MonitorMatchEditor {
                required property int index
                Layout.fillWidth: true
                heading: index === 0 ? "Monitors that are" : "Or monitors that are"
                path: page.matches[index] ? page.matches[index].path : ""
                node: page.matches[index] ? page.matches[index].node : ({ props: {} })
                onRemoveRequested: kcm.remove(path)
            }
        }

        QQC2.Button {
            Layout.margins: Kirigami.Units.largeSpacing
            icon.name: "list-add"
            text: page.matches.length === 0 ? "Add a condition…" : "Or also match…"
            onClicked: kcm.append(page.profilePath, Kdl.leaf("match", [], { "aspect-ratio-above": 2 }))
        }
    }

    CardHeader {
        visible: page.exists
        title: "Layout on these monitors"
    }

    Card {
        visible: page.exists

        LayoutOverrides {
            Layout.fillWidth: true
            scopePath: page.profilePath + "/layout"
            excluded: []
        }
    }
}
