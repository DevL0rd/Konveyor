import QtCore
import QtQuick
import QtQuick.Layouts
import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.components as PlasmaComponents
import org.kde.plasma.plasma5support as P5Support
import org.kde.kirigami as Kirigami

PlasmoidItem {
    id: root

    readonly property string helper: String(StandardPaths.writableLocation(StandardPaths.HomeLocation)).replace("file://", "") + "/.local/bin/linux-plasma-screen-rotate"
    readonly property string readCommand: helper + " state"
    readonly property string toggleCommand: helper + " toggle"

    property bool portrait: false

    function toggleRotation() {
        runner.connectSource(toggleCommand)
    }

    Plasmoid.icon: "object-rotate-left"
    preferredRepresentation: compactRepresentation
    toolTipMainText: i18n("Screen Rotate")
    toolTipSubText: portrait ? i18n("Portrait - click for landscape") : i18n("Landscape - click for portrait")

    P5Support.DataSource {
        id: runner
        engine: "executable"
        connectedSources: []

        onNewData: function (source, data) {
            disconnectSource(source)
            if (source === root.readCommand) {
                root.portrait = (data["stdout"] || "").indexOf("portrait") !== -1
            } else {
                connectSource(root.readCommand)
            }
        }
    }

    Component.onCompleted: runner.connectSource(readCommand)

    compactRepresentation: MouseArea {
        id: compact
        implicitWidth: Kirigami.Units.gridUnit * 1.5
        implicitHeight: Kirigami.Units.gridUnit * 1.5
        hoverEnabled: true
        onClicked: root.toggleRotation()

        Rectangle {
            anchors.fill: parent
            radius: Kirigami.Units.cornerRadius
            color: Kirigami.Theme.highlightColor
            opacity: compact.containsMouse ? 0.25 : 0

            Behavior on opacity {
                NumberAnimation { duration: Kirigami.Units.shortDuration }
            }
        }

        Kirigami.Icon {
            anchors.fill: parent
            anchors.margins: Math.round(Math.min(compact.width, compact.height) * 0.18)
            source: root.portrait ? "object-rotate-right" : "object-rotate-left"
        }
    }

    fullRepresentation: ColumnLayout {
        Layout.minimumWidth: Kirigami.Units.gridUnit * 12
        Layout.minimumHeight: Kirigami.Units.gridUnit * 6
        spacing: Kirigami.Units.smallSpacing

        PlasmaComponents.Label {
            Layout.alignment: Qt.AlignHCenter
            text: root.portrait ? i18n("Portrait") : i18n("Landscape")
        }

        PlasmaComponents.Button {
            Layout.alignment: Qt.AlignHCenter
            icon.name: root.portrait ? "object-rotate-right" : "object-rotate-left"
            text: root.portrait ? i18n("Rotate to landscape") : i18n("Rotate to portrait")
            onClicked: root.toggleRotation()
        }
    }
}
