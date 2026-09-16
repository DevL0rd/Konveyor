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
    property bool known: false
    property bool busy: false

    readonly property bool inPanel: Plasmoid.formFactor === PlasmaCore.Types.Horizontal || Plasmoid.formFactor === PlasmaCore.Types.Vertical
    readonly property string stateText: !known ? i18n("Checking…") : portrait ? i18n("Portrait") : i18n("Landscape")
    readonly property string actionText: portrait ? i18n("Rotate to landscape") : i18n("Rotate to portrait")

    function toggleRotation() {
        if (busy)
            return
        busy = true
        runner.connectSource(toggleCommand)
    }

    Plasmoid.icon: portrait ? "object-rotate-right" : "object-rotate-left"
    Plasmoid.title: i18n("Screen Rotate")
    preferredRepresentation: inPanel ? compactRepresentation : fullRepresentation
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
                root.known = true
                root.busy = false
            } else {
                connectSource(root.readCommand)
            }
        }
    }

    Component.onCompleted: runner.connectSource(readCommand)

    component ScreenGlyph: Item {
        id: glyph

        property real size: Kirigami.Units.iconSizes.medium
        property color tint: Kirigami.Theme.textColor

        implicitWidth: size
        implicitHeight: size

        Rectangle {
            id: frame
            anchors.centerIn: parent
            width: glyph.size * 0.86
            height: glyph.size * 0.58
            radius: Math.max(2, glyph.size * 0.1)
            color: Qt.alpha(glyph.tint, 0.12)
            border.width: Math.max(1.5, glyph.size * 0.08)
            border.color: glyph.tint
            rotation: root.portrait ? 90 : 0
            opacity: root.known ? 1 : 0.5
            Behavior on rotation { NumberAnimation { duration: 420; easing.type: Easing.OutBack; easing.overshoot: 1.2 } }
            Behavior on border.color { ColorAnimation { duration: 200 } }

            Rectangle {
                anchors.right: parent.right
                anchors.rightMargin: parent.border.width + Math.max(1, glyph.size * 0.05)
                anchors.verticalCenter: parent.verticalCenter
                width: Math.max(2, glyph.size * 0.07)
                height: width
                radius: width / 2
                color: glyph.tint
            }
        }

        SequentialAnimation on opacity {
            running: root.busy
            loops: Animation.Infinite
            alwaysRunToEnd: true
            NumberAnimation { to: 0.45; duration: 380; easing.type: Easing.InOutQuad }
            NumberAnimation { to: 1; duration: 380; easing.type: Easing.InOutQuad }
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
        onClicked: root.toggleRotation()

        Rectangle {
            anchors.fill: parent
            anchors.margins: 1
            radius: Kirigami.Units.cornerRadius
            color: Qt.alpha(compact.containsMouse ? Kirigami.Theme.highlightColor : Kirigami.Theme.textColor,
                            compact.pressed ? 0.28 : compact.containsMouse ? 0.14 : 0)
            Behavior on color { ColorAnimation { duration: 150 } }
        }

        ScreenGlyph {
            anchors.centerIn: parent
            size: Math.round(Math.min(compact.width, compact.height) * 0.72)
            tint: compact.containsMouse ? Kirigami.Theme.highlightColor : Kirigami.Theme.textColor
        }
    }

    fullRepresentation: Item {
        id: full

        Layout.minimumWidth: card.implicitWidth + Kirigami.Units.largeSpacing * 2
        Layout.minimumHeight: card.implicitHeight + Kirigami.Units.largeSpacing * 2
        Layout.preferredWidth: Layout.minimumWidth
        Layout.preferredHeight: Layout.minimumHeight

        RowLayout {
            id: card
            anchors.centerIn: parent
            spacing: Kirigami.Units.largeSpacing

            ScreenGlyph {
                Layout.alignment: Qt.AlignVCenter
                size: Kirigami.Units.gridUnit * 3
                tint: Kirigami.Theme.highlightColor
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                spacing: Kirigami.Units.smallSpacing

                PlasmaComponents.Label {
                    Layout.fillWidth: true
                    text: i18n("SCREEN")
                    font.pixelSize: Kirigami.Theme.smallFont.pixelSize
                    font.weight: Font.DemiBold
                    font.letterSpacing: 0.6
                    opacity: 0.6
                    elide: Text.ElideRight
                }
                PlasmaComponents.Label {
                    Layout.fillWidth: true
                    text: root.stateText
                    font.pixelSize: Kirigami.Theme.defaultFont.pixelSize * 1.3
                    font.weight: Font.DemiBold
                    elide: Text.ElideRight
                }
                PlasmaComponents.Button {
                    icon.name: root.portrait ? "object-rotate-right" : "object-rotate-left"
                    text: root.actionText
                    enabled: !root.busy
                    onClicked: root.toggleRotation()
                }
            }
        }
    }
}
