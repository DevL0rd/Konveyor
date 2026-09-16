import QtQuick
import QtQuick.Layouts
import QtQuick.Window
import org.kde.kirigami as Kirigami
import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.components as PlasmaComponents

PlasmoidItem {
    id: root

    property bool open: false
    property bool openedByKey: false
    property bool created: false
    property string openScreen

    readonly property bool vertical: Plasmoid.formFactor === PlasmaCore.Types.Vertical
    readonly property string buttonIcon: Plasmoid.configuration.icon || "start-here-kde-plasma"

    Plasmoid.icon: buttonIcon
    Plasmoid.title: i18n("Portal Launcher")
    preferredRepresentation: compactRepresentation
    activationTogglesExpanded: false
    toolTipMainText: i18n("Portal Launcher")
    toolTipSubText: i18n("Apps, games, files and friends")

    function show(byKey, screen) {
        openedByKey = byKey
        openScreen = screen
        created = true
        open = true
    }
    function hide() {
        open = false
    }
    function toggle(byKey, screen) {
        if (open)
            hide()
        else
            show(byKey, screen)
    }

    Connections {
        target: Plasmoid
        function onActivated() {
            root.toggle(true, "")
        }
    }

    compactRepresentation: MouseArea {
        id: button

        readonly property bool showLabel: Plasmoid.configuration.showLabel && Plasmoid.configuration.label !== "" && !root.vertical
        property bool wasOpen: false

        hoverEnabled: true
        onPressed: wasOpen = root.open
        onClicked: {
            if (wasOpen)
                root.hide()
            else
                root.show(false, button.Window.window && button.Window.window.screen ? button.Window.window.screen.name : "")
        }

        Layout.minimumWidth: root.vertical ? 0 : (showLabel ? buttonRow.implicitWidth + Kirigami.Units.smallSpacing * 2 : height)
        Layout.maximumWidth: root.vertical ? Infinity : Layout.minimumWidth
        Layout.minimumHeight: root.vertical ? width : 0
        Layout.maximumHeight: root.vertical ? width : Infinity

        RowLayout {
            id: buttonRow
            anchors.centerIn: parent
            spacing: Kirigami.Units.smallSpacing
            Kirigami.Icon {
                Layout.preferredWidth: Math.round(Math.min(button.width, button.height) * 0.72)
                Layout.preferredHeight: Layout.preferredWidth
                source: root.buttonIcon
                active: button.containsMouse || root.open
            }
            PlasmaComponents.Label {
                visible: button.showLabel
                text: Plasmoid.configuration.label
            }
        }
    }

    fullRepresentation: Item {}

    Loader {
        active: root.created
        sourceComponent: Launcher {}
    }
}
