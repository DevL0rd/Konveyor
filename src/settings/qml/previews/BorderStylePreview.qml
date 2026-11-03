import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.konveyor.settings

Item {
    id: preview

    property var layers: []
    property string caption
    property real baseWidth: 4
    property int emphasis: -1

    MockWindow {
        anchors.centerIn: parent
        height: parent.height - Kirigami.Units.gridUnit * 2
        width: Math.min(parent.width - Kirigami.Units.gridUnit * 3, Kirigami.Units.gridUnit * 18.5)
        zoom: 1
        cornerRadius: 8
        state_: "active"
        caption: preview.caption
        ring: ({ enabled: false, width: preview.baseWidth })
        overridePart: "focus-ring"
        overrideLayers: preview.layers
        emphasis: preview.emphasis
    }

    QQC2.Label {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        visible: SettingsStore.values["animations/reduce-motion"] === true || SettingsStore.values["animations/enabled"] === false
        text: "Reduce motion is on, so this stays still"
        color: "#fcfcfc"
        opacity: 0.75
        font: Kirigami.Theme.smallFont
    }
}
