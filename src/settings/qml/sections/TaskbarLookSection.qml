import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import "../components"
import org.kde.konveyor.settings

ColumnLayout {
    Layout.fillWidth: true
    spacing: 0

    CardHeader {
        title: "Icons"
    }

    Card {
        TaskbarChoiceRow {
            key: "autoIconSize"
            label: "Icon size"
            description: "Follow the panel's thickness, or keep the icons one size."
            iconName: "zoom-fit-best"
            options: [
                { value: true, title: "Follow the panel", description: "A little over half its thickness" },
                { value: false, title: "Fixed size", description: TaskbarSettings.values.iconSize + " px, set below" }
            ]
        }

        TaskbarSliderRow {
            key: "iconSize"
            label: "Fixed icon size"
            iconName: "transform-scale"
            enabled: TaskbarSettings.values.autoIconSize === false
            from: 12
            to: 96
        }

        TaskbarSliderRow {
            key: "iconSpacing"
            label: "Space between icons"
            iconName: "distribute-horizontal-x"
            from: 0
            to: 24
        }

        TaskbarSliderRow {
            key: "buttonPadding"
            label: "Padding around each icon"
            description: "Room for the hover and focus highlight."
            iconName: "distribute-horizontal-margin"
            from: 0
            to: 16
        }
    }

    CardHeader {
        title: "Focus and indicators"
    }

    Card {
        TaskbarChoiceRow {
            key: "highlightStyle"
            label: "Focused app"
            iconName: "window-active"
            options: [
                { value: 0, title: "Filled", description: "A tinted tile behind the icon" },
                { value: 1, title: "Outline", description: "A ring around the icon" },
                { value: 2, title: "No highlight", description: "Only the indicator shows it" }
            ]
        }

        TaskbarChoiceRow {
            key: "indicatorStyle"
            label: "Window indicators"
            iconName: "view-list-details"
            options: [
                { value: 0, title: "Dots", description: "One for each window" },
                { value: 1, title: "Bar", description: "A line, longer on the focused app" },
                { value: 2, title: "None", description: "Nothing under the icons" }
            ]
        }

        TaskbarChoiceRow {
            key: "indicatorEdge"
            label: "Indicator position"
            iconName: "align-vertical-bottom"
            enabled: TaskbarSettings.values.indicatorStyle !== 2
            options: [
                { value: 0, title: "Screen edge", description: "Next to the edge the panel sits on" },
                { value: 1, title: "Opposite side", description: "Facing the windows" }
            ]
        }

        TaskbarSwitchRow {
            key: "attentionPulse"
            label: "Pulse apps that want attention"
            iconName: "notifications"
            enabled: TaskbarSettings.values.animations !== false
        }

        TaskbarSwitchRow {
            key: "animations"
            label: "Animations"
            description: "Icons slide, grow and bounce as windows open, close and move."
            iconName: "preferences-desktop-effects"
        }
    }
}
