import QtQuick
import QtQuick.Layouts
import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.plasma5support as P5Support
import org.kde.kirigami as Kirigami

PlasmoidItem {
    id: root

    readonly property string readCommand: "sh -c \"kscreen-doctor -j | jq -r '[.outputs[] | select(.enabled == true) | .rotation] | first'\""
    readonly property string rotateLeftCommand: "sh -c \"for output in $(kscreen-doctor -j | jq -r '.outputs[] | select(.enabled == true) | .name'); do kscreen-doctor \\\"output.$output.rotation.left\\\"; done\""
    readonly property string rotateNoneCommand: "sh -c \"for output in $(kscreen-doctor -j | jq -r '.outputs[] | select(.enabled == true) | .name'); do kscreen-doctor \\\"output.$output.rotation.none\\\"; done\""

    property bool portrait: false

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
                root.portrait = (data["stdout"] || "").trim() !== "1"
            } else {
                connectSource(root.readCommand)
            }
        }
    }

    Component.onCompleted: runner.connectSource(readCommand)

    compactRepresentation: MouseArea {
        Layout.minimumWidth: Kirigami.Units.iconSizes.small
        Layout.minimumHeight: Kirigami.Units.iconSizes.small
        hoverEnabled: true

        Kirigami.Icon {
            anchors.fill: parent
            source: root.portrait ? "object-rotate-right" : "object-rotate-left"
            active: parent.containsMouse
        }

        onClicked: runner.connectSource(root.portrait ? root.rotateNoneCommand : root.rotateLeftCommand)
    }
}
