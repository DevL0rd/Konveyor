import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import "../components"
import "../previews"
import org.kde.konveyor.settings

SettingsPage {
    id: page

    readonly property bool animationsOn: SettingsStore.values["animations/enabled"] === true
    readonly property real slowdown: SettingsStore.values["animations/slowdown"] || 1

    readonly property var animations: [
        { name: "horizontal-view-movement", title: "Scrolling the row", description: "When focus moves to a column that's off screen and the view slides over.", icon: "go-next-view" },
        { name: "window-movement", title: "Moving windows", description: "When a window changes place, like moving a column or pulling a window into one.", icon: "transform-move" },
        { name: "window-resize", title: "Resizing windows", description: "When a column changes width or a window changes height.", icon: "transform-scale" },
        { name: "window-open", title: "Opening windows", description: "When a new window appears in the row.", icon: "window-new" },
        { name: "workspace-switch", title: "Switching workspaces", description: "When the view slides up or down to another workspace.", icon: "virtual-desktops" }
    ]

    title: "Motion"
    preview: MotionOverview {
        animationsOn: page.animationsOn
        slowdown: page.slowdown
    }

    CardHeader {
        title: "Everything"
    }

    Card {
        SwitchRow {
            label: "Animate windows"
            description: "Turn this off to make every movement instant."
            iconName: "preferences-desktop-effects"
            resetPaths: ["animations/on", "animations/off"]
            isOn: page.animationsOn
            onSwitched: on => SettingsStore.setToggle("animations", on)
        }

        SliderRow {
            enabled: page.animationsOn
            label: "Animation speed"
            description: "Stretches or shortens every animation at once. 1 is normal, 2 takes twice as long."
            iconName: "speedometer"
            resetPaths: ["animations/slowdown"]
            from: 0.1
            to: 5
            stepSize: 0.1
            decimals: 1
            unit: "×"
            lowLabel: "Faster"
            highLabel: "Slower"
            value: page.slowdown
            onEdited: value => SettingsStore.setValue("animations/slowdown", [Math.round(value * 10) / 10])
        }
    }

    CardHeader {
        title: "Each animation"
    }

    Repeater {
        model: page.animations

        ColumnLayout {
            required property var modelData
            Layout.fillWidth: true
            Layout.bottomMargin: Kirigami.Units.largeSpacing * 2
            spacing: 0
            enabled: page.animationsOn

            AnimationCard {
                name: modelData.name
                title: modelData.title
                description: modelData.description
                iconName: modelData.icon
            }
        }
    }
}
