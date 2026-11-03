import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import "../components"
import "../previews"
import "../catalog/BorderStyles.js" as BorderStyles
import org.kde.konveyor.settings

ColumnLayout {
    id: choices

    property string layerPath
    property var layerValues: null
    property string kind
    property real baseWidth: 3

    function variant(key, value) {
        const layer = Object.assign({}, layerValues);
        layer.width = layer.width > 8 ? 6 : layer.width;
        layer[key] = value;
        return [layer];
    }

    spacing: 0

    component Scene: Item {
        id: scene

        property string key
        property bool bands: false

        clip: true

        StyleBackdrop {
            anchors.fill: parent
            bands: scene.bands
        }

        MockWindow {
            anchors.fill: parent
            anchors.margins: Kirigami.Units.gridUnit
            zoom: 1
            barHeight: 0
            bare: scene.bands
            state_: "active"
            cornerRadius: 4
            ring: ({ enabled: false, width: choices.baseWidth })
            overridePart: "focus-ring"
            overrideLayers: choices.layerValues && scene.parent ? choices.variant(scene.key, scene.parent.choice.value) : []
        }
    }

    Component {
        id: directionScene
        Scene {
            key: "direction"
        }
    }

    Component {
        id: blendScene
        Scene {
            key: "blend"
            bands: true
        }
    }

    SettingRow {
        visible: choices.kind === "particles"
        Layout.fillWidth: true
        label: "Direction"
        description: "Which way the sparks fly"
        resetPaths: [choices.layerPath + "/direction"]
        wideControl: true

        ChoiceCards {
            width: parent.width
            cardHeight: Kirigami.Units.gridUnit * 7
            currentValue: choices.layerValues ? choices.layerValues.direction : "out"
            options: BorderStyles.directions.map(entry => ({ value: entry.value, title: entry.label, preview: directionScene }))
            onChosen: value => SettingsStore.setValue(choices.layerPath + "/direction", [value])
        }
    }

    SettingRow {
        Layout.fillWidth: true
        label: "Blend"
        description: "How this layer mixes with what's under it. The tiles show it over dark and bright."
        resetPaths: [choices.layerPath + "/blend"]
        wideControl: true

        ChoiceCards {
            width: parent.width
            cardHeight: Kirigami.Units.gridUnit * 9.5
            currentValue: choices.layerValues ? choices.layerValues.blend : "normal"
            options: BorderStyles.blends.map(entry => ({ value: entry.value, title: entry.label, description: entry.caption, preview: blendScene }))
            onChosen: value => SettingsStore.setValue(choices.layerPath + "/blend", [value])
        }
    }
}
