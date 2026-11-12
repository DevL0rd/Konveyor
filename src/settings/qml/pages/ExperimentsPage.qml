import QtQuick
import QtQuick.Layouts
import "../components"
import org.kde.konveyor.settings

SettingsPage {
    id: page

    title: "Experiments"

    readonly property var peek: SettingsStore.values["experiments/peek-desktop"] || ({})

    CardHeader {
        title: "Desktop peek"
    }

    Card {
        SwitchRow {
            label: "Peek at the desktop while holding Meta"
            description: "Hold Meta on its own to fade every other window on that screen while the focused one stays. Move the pointer off the focused window and it slides to the other side, shrinking to half the screen if it is wider, so files can be dragged to and from the desktop. Let go of Meta, or finish the drop, and everything goes back. A quick tap still opens the launcher."
            iconName: "user-desktop"
            resetPaths: ["experiments/peek-desktop"]
            isOn: page.peek.enabled === true
            onSwitched: on => SettingsStore.setToggle("experiments/peek-desktop", on)
        }

        SliderRow {
            label: "Hold before peeking"
            description: "How long Meta has to be held on its own before the other windows fade, so Meta shortcuts and a quick tap for the launcher stay as they are."
            resetPaths: ["experiments/peek-desktop/delay-ms"]
            enabled: page.peek.enabled === true
            from: 100
            to: 1500
            stepSize: 10
            unit: "ms"
            lowLabel: "Quick"
            highLabel: "Patient"
            value: page.peek["delay-ms"] || 0
            onEdited: value => SettingsStore.setValue("experiments/peek-desktop/delay-ms", [Math.round(value)])
        }
    }

    CardHeader {
        title: "Fullscreen focus changes"
    }

    Card {
        SwitchRow {
            label: "Prevent minimizing on focus change"
            description: "Intercept native Wayland and XWayland minimize requests while an app is fullscreen. XWayland apps may stop drawing until they are focused again. KWin and user-initiated minimize actions still work."
            iconName: "window-minimize"
            resetPaths: ["experiments/prevent-fullscreen-minimize"]
            isOn: SettingsStore.values["experiments/prevent-fullscreen-minimize"]
            onSwitched: on => SettingsStore.setFlag("experiments/prevent-fullscreen-minimize", on)
        }

        SwitchRow {
            label: "Prevent leaving fullscreen on focus change"
            description: "Ignore native Wayland and XWayland requests to leave fullscreen while the app is unfocused. Requests made while focused still work."
            iconName: "view-fullscreen"
            resetPaths: ["experiments/prevent-fullscreen-exit"]
            isOn: SettingsStore.values["experiments/prevent-fullscreen-exit"]
            onSwitched: on => SettingsStore.setFlag("experiments/prevent-fullscreen-exit", on)
        }
    }
}
