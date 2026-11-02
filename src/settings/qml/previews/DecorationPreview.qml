import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.konveyor.settings
import "../catalog/BorderStyles.js" as BorderStyles
import "../catalog/EventEffects.js" as EventEffects

Item {
    id: preview

    property var values: ({})
    property real cornerRadius: 6
    property var tryOn: null
    readonly property bool eventTrying: tryOn !== null && tryOn.event !== undefined
    property var eventPlays: null
    readonly property string targetPart: BorderStyles.targetPart(values)
    readonly property string captionText: !tryOn ? "" : tryOn.caption || "Trying: " + tryOn.label + " on the " + BorderStyles.partLabel(targetPart)

    function updatePlays() {
        const rects = [];
        for (let index = 0; index < mocks.count; ++index) {
            const mock = mocks.itemAt(index);
            if (!mock) {
                eventPlays = null;
                return;
            }
            rects.push(Qt.rect(mock.x / mock.zoom, mock.y / mock.zoom, mock.width / mock.zoom, mock.height / mock.zoom));
        }
        eventPlays = tryOn && eventTrying ? EventEffects.play(rects, 0, EventEffects.crosses(tryOn.layers) ? 2 : -1, tryOn.reach, stageClock.time, tryOn.duration) : null;
    }

    function measurePlay() {
        const mock = mocks.itemAt(0);
        if (eventTrying && mock) {
            stageClock.duration = Math.max(0.1, mock.playSeconds());
        }
        stageClock.restart();
    }

    onTryOnChanged: {
        updatePlays();
        Qt.callLater(measurePlay);
    }
    onWidthChanged: updatePlays()

    EventPlayClock {
        id: stageClock
        playing: preview.eventTrying
        still: SettingsStore.values["animations/reduce-motion"] === true || SettingsStore.values["animations/enabled"] === false
        onTimeChanged: preview.updatePlays()
    }

    RowLayout {
        anchors.fill: parent
        spacing: Kirigami.Units.gridUnit

        Repeater {
            id: mocks
            model: [
                { state: "active", caption: "Focused" },
                { state: "inactive", caption: "Inactive window" },
                { state: "urgent", caption: "Needs attention" }
            ]

            MockWindow {
                required property var modelData
                required property int index
                readonly property bool trying: preview.tryOn !== null && !preview.eventTrying && modelData.state === "active"
                Layout.fillWidth: true
                Layout.fillHeight: true
                state_: modelData.state
                caption: modelData.caption
                ring: preview.values["focus-ring"]
                border: preview.values.border
                cornerRadius: preview.cornerRadius
                overridePart: trying ? preview.targetPart : ""
                overrideLayers: trying ? preview.tryOn.layers : []
                eventLayers: preview.eventTrying && EventEffects.reached(index, 0, preview.tryOn.reach, preview.tryOn.layers) ? preview.tryOn.layers : []
                eventPlay: preview.eventTrying && preview.eventPlays ? preview.eventPlays[index] : null
            }
        }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        visible: preview.captionText.length > 0
        width: captionLabel.implicitWidth + Kirigami.Units.smallSpacing * 4
        height: captionLabel.implicitHeight + Kirigami.Units.smallSpacing
        radius: 3
        color: Qt.rgba(10 / 255, 12 / 255, 14 / 255, 0.72)

        QQC2.Label {
            id: captionLabel
            anchors.centerIn: parent
            text: preview.captionText
            color: "#fcfcfc"
            font: Kirigami.Theme.smallFont
        }
    }
}
