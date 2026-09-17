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
    property string panelScreen
    property var pendingPins: []
    signal pinsRequested()

    function pinFiles(urls) {
        const files = urls.map(url => decodeURIComponent(String(url).replace(/^file:\/\//, ""))).filter(path => path.endsWith(".desktop"))
        if (files.length === 0)
            return false
        pendingPins = pendingPins.concat(files)
        created = true
        pinsRequested()
        return true
    }

    readonly property bool vertical: Plasmoid.formFactor === PlasmaCore.Types.Vertical
    readonly property string buttonIcon: Plasmoid.configuration.icon || "start-here-kde-plasma-symbolic"

    Plasmoid.icon: buttonIcon
    Plasmoid.title: i18n("Portal Launcher")
    preferredRepresentation: compactRepresentation
    activationTogglesExpanded: false
    toolTipMainText: i18n("Portal Launcher")
    toolTipSubText: i18n("Apps, games, files and friends · Meta opens it · drop an app here to pin it")

    function show(byKey, screen) {
        openedByKey = byKey
        openScreen = screen || panelScreen
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

    onExpandedChanged: function() {
        if (root.expanded)
            root.expanded = false
    }
    Component.onCompleted: root.expanded = false

    Connections {
        target: Plasmoid
        function onActivated() {
            root.toggle(true, "")
        }
    }

    compactRepresentation: MouseArea {
        id: button

        readonly property bool showLabel: Plasmoid.configuration.showLabel && Plasmoid.configuration.label !== "" && !root.vertical
        readonly property string screenName: Window.window && Window.window.screen ? Window.window.screen.name : ""
        property bool wasOpen: false
        onScreenNameChanged: root.panelScreen = screenName
        Component.onCompleted: root.panelScreen = screenName

        hoverEnabled: true
        onPressed: wasOpen = root.open
        onClicked: {
            if (wasOpen)
                root.hide()
            else
                root.show(false, button.screenName)
        }

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
                source: root.buttonIcon
                active: button.containsMouse || root.open || dropArea.containsDrag
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
