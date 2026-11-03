import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import "../catalog/BorderStyles.js" as BorderStyles
import "../catalog/Signals.js" as Signals

Rectangle {
    id: tile

    property var layers: []
    property string name
    property string summary: BorderStyles.summary(layers)
    property string usage
    property string tooltip: name + "\n" + summary
    property real previewHeight: 96
    property real baseWidth: 3
    property real rippleRest: 3
    property bool interactive: true
    property bool locked: false
    property bool bands: false
    default property alias actions: actionColumn.data
    readonly property bool tryingOn: hover.hovered || main.activeFocus

    signal activated()

    implicitWidth: Kirigami.Units.gridUnit * 9
    implicitHeight: layout.implicitHeight
    radius: 5
    color: Kirigami.Theme.backgroundColor
    border.width: 1
    border.color: tile.tryingOn ? Kirigami.Theme.highlightColor : Qt.alpha(Kirigami.Theme.textColor, 0.2)

    HoverHandler {
        id: hover
    }

    QQC2.ToolTip.text: [tile.tooltip, Signals.tooltipFor(tile.layers)].filter(text => text.length > 0).join("\n")
    QQC2.ToolTip.visible: hover.hovered && QQC2.ToolTip.text.length > 0
    QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay

    ColumnLayout {
        id: layout
        anchors.fill: parent
        anchors.margins: tile.border.width
        spacing: 0

        QQC2.AbstractButton {
            id: main
            Layout.fillWidth: true
            text: tile.name
            hoverEnabled: false
            padding: 0
            onClicked: {
                if (tile.interactive) {
                    tile.activated();
                }
            }

            contentItem: ColumnLayout {
                spacing: 0

                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: tile.previewHeight
                    clip: true

                    StyleBackdrop {
                        anchors.fill: parent
                        bands: tile.bands
                        topRadius: tile.radius - tile.border.width
                    }

                    MockWindow {
                        anchors.fill: parent
                        anchors.margins: Kirigami.Units.gridUnit - 2
                        zoom: 1
                        barHeight: 10
                        bare: tile.bands
                        state_: "active"
                        ring: ({ enabled: false, width: tile.baseWidth })
                        overridePart: "focus-ring"
                        overrideLayers: tile.layers
                        rippleRest: tile.rippleRest
                    }

                    SignalBadge {
                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.margins: 4
                        showTips: false
                        layers: tile.layers
                    }

                    Rectangle {
                        anchors.top: parent.top
                        anchors.right: parent.right
                        anchors.margins: 4
                        visible: tile.locked
                        width: 24
                        height: 24
                        radius: 3
                        color: Qt.rgba(10 / 255, 12 / 255, 14 / 255, 0.72)

                        Kirigami.Icon {
                            anchors.centerIn: parent
                            width: Kirigami.Units.iconSizes.small
                            height: width
                            source: "object-locked"
                            color: "#fcfcfc"
                            isMask: true
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.margins: Kirigami.Units.smallSpacing + 2
                    spacing: 0

                    QQC2.Label {
                        Layout.fillWidth: true
                        text: tile.name
                        elide: Text.ElideRight
                        font.weight: Font.Medium
                    }

                    QQC2.Label {
                        Layout.fillWidth: true
                        text: tile.summary
                        elide: Text.ElideRight
                        font: Kirigami.Theme.smallFont
                        opacity: 0.75
                    }

                    QQC2.Label {
                        Layout.fillWidth: true
                        visible: tile.usage.length > 0
                        text: tile.usage
                        elide: Text.ElideRight
                        font: Kirigami.Theme.smallFont
                        opacity: 0.75
                    }
                }
            }
        }

        Item {
            Layout.fillHeight: true
        }

        ColumnLayout {
            id: actionColumn
            Layout.fillWidth: true
            Layout.margins: visibleChildren.length > 0 ? Kirigami.Units.smallSpacing : 0
            spacing: 0
        }
    }
}
