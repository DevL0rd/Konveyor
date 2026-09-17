import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import "../components"
import "../components/monitors"
import "../previews"
import "../sections"
import org.kde.konveyor.settings

SettingsPage {
    id: page

    property string outputPath
    readonly property var output: SettingsStore.revision >= 0 ? SettingsStore.node(outputPath) : ({})
    readonly property bool exists: output.name !== undefined
    readonly property string outputName: exists && output.args.length ? String(output.args[0]) : ""
    readonly property var connected: SettingsStore.live.outputs.find(entry => entry.name === outputName)

    title: exists ? "Monitor: " + outputName : "Monitor override"
    preview: SettingsStore.live.outputs.length > 0 ? arrangement : null

    Component {
        id: arrangement
        MonitorArrangement {}
    }

    Component.onCompleted: SettingsStore.live.refresh()

    Kirigami.PlaceholderMessage {
        visible: !page.exists
        Layout.fillWidth: true
        Layout.margins: Kirigami.Units.gridUnit * 2
        icon.name: "dialog-question"
        text: "This override no longer exists"
    }

    Card {
        visible: page.exists
        Layout.topMargin: Kirigami.Units.gridUnit

        ColumnLayout {
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.largeSpacing
            spacing: Kirigami.Units.smallSpacing

            Kirigami.Heading {
                level: 3
                text: page.outputName
            }

            QQC2.Label {
                text: page.connected
                    ? (page.connected.description || "") + " · " + page.connected.logical.width + " × " + page.connected.logical.height
                    : "Not connected right now. These settings apply whenever it's plugged in."
                wrapMode: Text.Wrap
                opacity: 0.75
                Layout.fillWidth: true
            }
        }
    }

    CardHeader {
        visible: page.exists
        title: "Hot corners"
    }

    Card {
        visible: page.exists

        OutputHotCorners {
            Layout.fillWidth: true
            blockPath: page.outputPath + "/hot-corners"
        }
    }

    CardHeader {
        visible: page.exists
        title: "Layout on this monitor"
    }

    Card {
        visible: page.exists

        LayoutOverrides {
            Layout.fillWidth: true
            scopePath: page.outputPath + "/layout"
            excluded: []
        }
    }
}
