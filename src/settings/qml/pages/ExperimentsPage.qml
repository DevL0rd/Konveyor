import QtQuick
import QtQuick.Layouts
import "../components"
import org.kde.konveyor.settings

SettingsPage {
    title: "Experiments"

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
