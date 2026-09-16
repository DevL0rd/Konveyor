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

    readonly property string showCommand: String(StandardPaths.writableLocation(StandardPaths.HomeLocation)).replace("file://", "") + "/.local/bin/linux-plasma-keyboard-toggle"
    readonly property bool inPanel: Plasmoid.formFactor === PlasmaCore.Types.Horizontal || Plasmoid.formFactor === PlasmaCore.Types.Vertical

    signal triggered()

    Plasmoid.icon: "input-keyboard"
    Plasmoid.title: i18n("Keyboard Toggle")
    preferredRepresentation: inPanel ? compactRepresentation : fullRepresentation
    toolTipMainText: i18n("Virtual Keyboard")
    toolTipSubText: i18n("Click to show the on-screen keyboard")

    function showKeyboard() {
        runner.connectSource(showCommand)
        triggered()
    }

    P5Support.DataSource {
        id: runner
        engine: "executable"
        connectedSources: []
        onNewData: function (source, data) {
            disconnectSource(source)
        }
    }

    component Ripple: Rectangle {
        id: ripple

        property color tint: Kirigami.Theme.highlightColor

        anchors.centerIn: parent
        width: Math.min(parent.width, parent.height)
        height: width
        radius: width / 2
        color: "transparent"
        border.width: Math.max(1.5, width * 0.06)
        border.color: tint
        opacity: 0
        scale: 0.4

        Connections {
            target: root
            function onTriggered() { flash.restart() }
        }

        ParallelAnimation {
            id: flash
            NumberAnimation { target: ripple; property: "scale"; from: 0.4; to: 1.15; duration: 450; easing.type: Easing.OutCubic }
            NumberAnimation { target: ripple; property: "opacity"; from: 0.9; to: 0; duration: 450; easing.type: Easing.OutCubic }
        }
    }

    compactRepresentation: MouseArea {
        id: compact

        readonly property bool vertical: Plasmoid.formFactor === PlasmaCore.Types.Vertical
        readonly property real thickness: vertical ? width : height

        Layout.minimumWidth: vertical ? 0 : thickness
        Layout.maximumWidth: vertical ? -1 : thickness
        Layout.preferredWidth: Layout.minimumWidth
        Layout.minimumHeight: vertical ? thickness : 0
        Layout.maximumHeight: vertical ? thickness : -1
        Layout.preferredHeight: Layout.minimumHeight
        implicitWidth: Kirigami.Units.gridUnit * 1.5
        implicitHeight: Kirigami.Units.gridUnit * 1.5

        hoverEnabled: true
        onClicked: root.showKeyboard()

        Rectangle {
            anchors.fill: parent
            anchors.margins: 1
            radius: Kirigami.Units.cornerRadius
            color: Qt.alpha(compact.containsMouse ? Kirigami.Theme.highlightColor : Kirigami.Theme.textColor,
                            compact.pressed ? 0.28 : compact.containsMouse ? 0.14 : 0)
            Behavior on color { ColorAnimation { duration: 150 } }
        }

        Ripple {}

        Kirigami.Icon {
            anchors.centerIn: parent
            width: Math.round(Math.min(compact.width, compact.height) * 0.66)
            height: width
            source: "input-keyboard"
            scale: compact.pressed ? 0.88 : 1
            Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
        }
    }

    fullRepresentation: Item {
        Layout.minimumWidth: card.implicitWidth + Kirigami.Units.largeSpacing * 2
        Layout.minimumHeight: card.implicitHeight + Kirigami.Units.largeSpacing * 2
        Layout.preferredWidth: Layout.minimumWidth
        Layout.preferredHeight: Layout.minimumHeight

        RowLayout {
            id: card
            anchors.centerIn: parent
            spacing: Kirigami.Units.largeSpacing

            Item {
                Layout.alignment: Qt.AlignVCenter
                implicitWidth: Kirigami.Units.gridUnit * 3
                implicitHeight: implicitWidth

                Rectangle {
                    anchors.fill: parent
                    radius: width / 2
                    color: Qt.alpha(Kirigami.Theme.highlightColor, 0.14)
                }
                Ripple {}
                Kirigami.Icon {
                    anchors.centerIn: parent
                    width: Kirigami.Units.iconSizes.medium
                    height: width
                    source: "input-keyboard"
                }
            }

            ColumnLayout {
                Layout.alignment: Qt.AlignVCenter
                spacing: Kirigami.Units.smallSpacing

                PlasmaComponents.Label {
                    text: i18n("KEYBOARD")
                    font.pixelSize: Kirigami.Theme.smallFont.pixelSize
                    font.weight: Font.DemiBold
                    font.letterSpacing: 0.6
                    opacity: 0.6
                }
                PlasmaComponents.Label {
                    text: i18n("On-screen keyboard")
                    font.pixelSize: Kirigami.Theme.defaultFont.pixelSize * 1.3
                    font.weight: Font.DemiBold
                }
                PlasmaComponents.Button {
                    icon.name: "input-keyboard"
                    text: i18n("Show keyboard")
                    onClicked: root.showKeyboard()
                }
            }
        }
    }
}
