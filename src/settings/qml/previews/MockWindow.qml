import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import "../catalog/BorderStyles.js" as BorderStyles
import org.kde.konveyor.settings

Item {
    id: mock

    property string caption
    property var ring
    property var border
    property string state_: "inactive"
    property real cornerRadius: 6
    property real zoom: 1.5
    property real barHeight: Kirigami.Units.gridUnit * 1.2
    property bool bare: false
    property string overridePart
    property var overrideLayers: []
    property int emphasis: -1
    property real rippleRest: 3
    property var eventLayers: []
    property var eventPlay: null
    readonly property bool ringDrawn: overridePart === "focus-ring"
        || (!!ring && ring.enabled === true && (state_ !== "inactive" || !!ring.inactive || ringLayers.length > 0))
    readonly property bool borderDrawn: overridePart === "border" || (!!border && border.enabled === true)
    readonly property real ringWidth: ringDrawn ? ring.width * zoom : 0
    readonly property real borderWidth: borderDrawn ? border.width * zoom : 0
    readonly property var ringLayers: overridePart === "focus-ring" ? overrideLayers : layersOf(ring)
    readonly property var borderLayers: overridePart === "border" ? overrideLayers : layersOf(border)

    function layersOf(part) {
        const name = part ? part[state_ + "-style"] : "";
        return name ? BorderStyles.layersNamed(SettingsStore.values["border-styles"] || [], name) : [];
    }

    function playSeconds() {
        return eventFrame.eventDuration();
    }

    PaintFrame {
        anchors.fill: parent
        thickness: mock.ringWidth
        radius: mock.cornerRadius + mock.borderWidth
        paint: mock.ring && mock.ringLayers.length === 0 ? mock.ring[mock.state_] : null
    }

    PaintFrame {
        anchors.fill: parent
        anchors.margins: mock.ringWidth
        thickness: mock.borderWidth
        radius: mock.cornerRadius
        paint: mock.border && mock.borderLayers.length === 0 ? mock.border[mock.state_] : null
    }

    StyleFrame {
        anchors.fill: body
        visible: mock.borderDrawn && mock.borderLayers.length > 0
        layers: mock.borderLayers
        baseWidth: mock.borderDrawn ? mock.border.width : 0
        radius: mock.cornerRadius / mock.zoom
        zoom: mock.zoom
        rippleRest: mock.rippleRest
        emphasis: mock.overridePart === "border" ? mock.emphasis : -1
    }

    StyleFrame {
        anchors.fill: body
        visible: mock.ringDrawn && mock.ringLayers.length > 0
        layers: mock.ringLayers
        baseWidth: mock.ringDrawn ? mock.ring.width : 0
        inset: mock.borderWidth / mock.zoom
        radius: mock.cornerRadius / mock.zoom
        zoom: mock.zoom
        rippleRest: mock.rippleRest
        emphasis: mock.overridePart === "focus-ring" ? mock.emphasis : -1
    }

    StyleFrame {
        id: eventFrame
        anchors.fill: body
        visible: mock.eventLayers.length > 0
        layers: mock.eventLayers
        baseWidth: 3
        radius: mock.cornerRadius / mock.zoom
        zoom: mock.zoom
        play: mock.eventPlay
    }

    Rectangle {
        id: body
        anchors.fill: parent
        anchors.margins: mock.ringWidth + mock.borderWidth
        radius: mock.cornerRadius
        color: mock.bare ? "transparent" : Kirigami.Theme.backgroundColor
        clip: true

        Rectangle {
            visible: !mock.bare && mock.barHeight > 0
            width: parent.width
            height: mock.barHeight
            color: Qt.alpha(Kirigami.Theme.textColor, mock.state_ === "active" ? 0.14 : 0.07)

            QQC2.Label {
                anchors.centerIn: parent
                visible: mock.caption.length > 0
                text: mock.caption
                font: Kirigami.Theme.smallFont
                opacity: mock.state_ === "active" ? 1 : 0.6
            }
        }

        Column {
            visible: !mock.bare
            x: Kirigami.Units.smallSpacing * 2
            y: mock.barHeight + Kirigami.Units.gridUnit * 0.6
            spacing: 5

            Repeater {
                model: [0.7, 0.5, 0.62, 0.35]

                Rectangle {
                    required property real modelData
                    width: body.width * modelData
                    height: 4
                    radius: 2
                    color: Qt.alpha(Kirigami.Theme.textColor, 0.15)
                }
            }
        }
    }
}
