import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import "../components"
import "../catalog/Kdl.js" as Kdl
import "CornerRule.js" as CornerRule
import org.kde.konveyor.settings

ColumnLayout {
    id: root

    readonly property var rule: SettingsStore.revision >= 0 ? CornerRule.find(SettingsStore) : ({ path: "", radii: [0, 0, 0, 0], clip: false })

    function writeRule(name, args) {
        if (root.rule.path.length > 0) {
            SettingsStore.setValue(root.rule.path + "/" + name, args);
        } else {
            SettingsStore.append("", Kdl.block("window-rule", [Kdl.leaf(name, args)]));
        }
    }

    spacing: 0

    CardHeader {
        title: "Window corners"
    }

    Card {
        SettingRow {
            wideControl: true
            label: "Corner roundness for every window"
            description: "Rounds window corners, including apps that draw square corners themselves. Fullscreen windows always stay square."
            iconName: "draw-rectangle"

            CornerRadiusEditor {
                radii: root.rule.radii
                onEdited: radii => root.writeRule("geometry-corner-radius", radii.every(r => r === radii[0]) ? [Math.round(radii[0])] : radii.map(Math.round))
            }
        }

        SettingRow {
            label: "Cut app content to the rounded shape"
            description: "Clips what the app draws so its corners follow the roundness above."
            iconName: "transform-crop"

            ScopedSwitch {
                isOn: root.rule.clip
                onSwitched: on => root.writeRule("clip-to-geometry", [on])
            }
        }
    }
}
