import QtQuick
import QtQuick.Layouts
import "../components"
import org.kde.konveyor.settings

SettingsPage {
    id: page

    title: "Overview"

    readonly property var overview: SettingsStore.values["overview"] || ({})

    CardHeader {
        title: "Konveyor overview"
    }

    Card {
        SwitchRow {
            label: "Use Konveyor's overview instead of KDE's"
            description: "Open every workspace on every monitor at once, with the real windows at their real places. Pan in both directions, drag windows between workspaces, reorder workspaces by their top strip, and pick a window with the arrow keys and Enter. Panels stay visible and empty workspaces show as blank cards. Turning this off gives toggle-overview, the pinch and the hot corners back to KDE's overview."
            iconName: "view-grid"
            resetPaths: ["overview/enabled"]
            isOn: page.overview.enabled === true
            onSwitched: on => SettingsStore.setFlag("overview/enabled", on)
        }

        SliderRow {
            label: "Smallest window scale"
            description: "How small a window may get when the workspace stack is tall, as a fraction of the scale it would have with one workspace. Raise it to keep windows readable, lower it to fit more workspaces at once."
            resetPaths: ["overview/minimum-scale"]
            enabled: page.overview.enabled === true
            from: 0.1
            to: 1.0
            stepSize: 0.05
            decimals: 2
            lowLabel: "Small"
            highLabel: "Large"
            value: page.overview["minimum-scale"] || 0
            onEdited: value => SettingsStore.setValue("overview/minimum-scale", [Math.round(value * 100) / 100])
        }

        SliderRow {
            label: "Space between workspaces"
            description: "The gap between workspace cards, as a fraction of the screen height. A little space separates the cards; none makes a continuous stack."
            resetPaths: ["overview/card-gap"]
            enabled: page.overview.enabled === true
            from: 0
            to: 0.5
            stepSize: 0.01
            decimals: 2
            lowLabel: "Tight"
            highLabel: "Loose"
            value: page.overview["card-gap"] || 0
            onEdited: value => SettingsStore.setValue("overview/card-gap", [Math.round(value * 100) / 100])
        }

        SettingRow {
            label: "Offscreen indicators"
            description: "Show chevrons along screen edges when content extends beyond the view, or hide indicators completely."
            resetPaths: ["overview/offscreen-indicators"]
            enabled: page.overview.enabled === true

            Segmented {
                options: [
                    { value: "chevrons", label: "Chevrons" },
                    { value: "off", label: "Off" }
                ]
                currentValue: page.overview["offscreen-indicators"] || "chevrons"
                onChosen: value => SettingsStore.setValue("overview/offscreen-indicators", [value])
            }
        }

        SwitchRow {
            label: "Allow Kontrol Panel in overview"
            description: "Allow opening the Kontrol Panel on top of the overview, or block it while the overview is open."
            iconName: "configure"
            resetPaths: ["overview/allow-kontrol-panel"]
            enabled: page.overview.enabled === true
            isOn: page.overview["allow-kontrol-panel"] === true
            onSwitched: on => SettingsStore.setFlag("overview/allow-kontrol-panel", on)
        }

        SliderRow {
            label: "Scroll speed"
            description: "How fast scrolling moves through workspaces and panned columns in the overview."
            resetPaths: ["overview/scroll-speed"]
            enabled: page.overview.enabled === true
            from: 0.1
            to: 2.0
            stepSize: 0.05
            decimals: 2
            lowLabel: "Slow"
            highLabel: "Fast"
            value: page.overview["scroll-speed"] !== undefined ? page.overview["scroll-speed"] : 0.5
            onEdited: value => SettingsStore.setValue("overview/scroll-speed", [Math.round(value * 100) / 100])
        }
    }
}
