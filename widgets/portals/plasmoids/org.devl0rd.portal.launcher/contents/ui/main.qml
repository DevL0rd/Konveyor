import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.components as PlasmaComponents
import org.kde.plasma.plasma5support as P5Support

PlasmoidItem {
    id: root

    readonly property bool vertical: Plasmoid.formFactor === PlasmaCore.Types.Vertical
    readonly property string buttonIcon: Plasmoid.configuration.icon || "start-here-kde-plasma-symbolic"
    property string failure: ""

    function shq(text) {
        return "'" + String(text).replace(/'/g, "'\\''") + "'"
    }
    function call(method, argumentsText) {
        panel.connectSource("busctl --user call org.devl0rd.KontrolPanel /KontrolPanel org.devl0rd.KontrolPanel " + method + (argumentsText ? " " + argumentsText : "") + " # " + Date.now())
    }
    function toggle() {
        call("Toggle")
    }
    function pinFiles(urls) {
        const files = urls.map(url => decodeURIComponent(String(url).replace(/^file:\/\//, ""))).filter(path => path.endsWith(".desktop"))
        if (files.length === 0)
            return false
        call("Pin", "as " + files.length + " " + files.map(shq).join(" "))
        return true
    }

    P5Support.DataSource {
        id: panel
        engine: "executable"
        onNewData: function(source, result) {
            disconnectSource(source)
            root.failure = result["exit code"] === 0 ? "" : (result.stderr || "").trim() || i18n("The Kontrol Panel service did not answer")
        }
    }

    Plasmoid.icon: failure ? "dialog-error" : buttonIcon
    Plasmoid.title: i18n("Kontrol Panel")
    preferredRepresentation: compactRepresentation
    activationTogglesExpanded: false
    toolTipMainText: i18n("Kontrol Panel")
    toolTipSubText: failure ? i18n("Could not open the Kontrol Panel: %1", failure) : i18n("Apps, games, files and friends · Meta opens it · drop an app here to pin it")

    onExpandedChanged: function() {
        if (root.expanded)
            root.expanded = false
    }
    Component.onCompleted: root.expanded = false

    Connections {
        target: Plasmoid
        function onActivated() {
            root.toggle()
        }
    }

    compactRepresentation: MouseArea {
        id: button

        readonly property bool showLabel: Plasmoid.configuration.showLabel && Plasmoid.configuration.label !== "" && !root.vertical

        hoverEnabled: true
        onClicked: root.toggle()

        Layout.minimumWidth: root.vertical ? 0 : (showLabel ? buttonRow.implicitWidth + Kirigami.Units.smallSpacing * 2 : height)
        Layout.maximumWidth: root.vertical ? Infinity : Layout.minimumWidth
        Layout.minimumHeight: root.vertical ? width : 0
        Layout.maximumHeight: root.vertical ? width : Infinity

        DropArea {
            id: dropArea
            anchors.fill: parent
            keys: ["text/uri-list"]
            onEntered: function(drag) {
                drag.accepted = drag.hasUrls && drag.urls.some(url => String(url).endsWith(".desktop"))
            }
            onDropped: function(drop) {
                if (drop.hasUrls && root.pinFiles(drop.urls))
                    drop.acceptProposedAction()
            }
        }

        RowLayout {
            id: buttonRow
            anchors.centerIn: parent
            spacing: Kirigami.Units.smallSpacing
            Kirigami.Icon {
                Layout.preferredWidth: Kirigami.Units.iconSizes.smallMedium
                Layout.preferredHeight: Layout.preferredWidth
                source: Plasmoid.icon
                active: button.containsMouse || dropArea.containsDrag
            }
            PlasmaComponents.Label {
                visible: button.showLabel
                text: Plasmoid.configuration.label
            }
        }
    }

    fullRepresentation: Item {}
}
