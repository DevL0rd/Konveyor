import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

QQC2.Popup {
    id: popup

    property var gradient
    signal edited(var changes)

    readonly property var spaces: [
        { value: "srgb", label: "sRGB (classic)" },
        { value: "srgb-linear", label: "Linear sRGB (brighter middle)" },
        { value: "oklab", label: "Oklab (even, natural)" },
        { value: "oklch shorter hue", label: "Oklch, shorter hue path" },
        { value: "oklch longer hue", label: "Oklch, rainbow the long way" },
        { value: "oklch increasing hue", label: "Oklch, hue increasing" },
        { value: "oklch decreasing hue", label: "Oklch, hue decreasing" }
    ]

    padding: Kirigami.Units.largeSpacing

    contentItem: GridLayout {
        columns: 2
        columnSpacing: Kirigami.Units.largeSpacing
        rowSpacing: Kirigami.Units.smallSpacing

        QQC2.Label { text: "From:"; Layout.alignment: Qt.AlignRight }

        ColorField {
            value: popup.gradient.from
            onEdited: css => popup.edited({ from: css })
        }

        QQC2.Label { text: "To:"; Layout.alignment: Qt.AlignRight }

        ColorField {
            value: popup.gradient.to
            onEdited: css => popup.edited({ to: css })
        }

        QQC2.Label { text: "Angle:"; Layout.alignment: Qt.AlignRight }

        AngleDial {
            angle: popup.gradient.angle
            onEdited: value => popup.edited({ angle: value })
        }

        QQC2.Label { text: "Spread across:"; Layout.alignment: Qt.AlignRight }

        QQC2.ComboBox {
            model: [
                { value: "window", label: "Each window" },
                { value: "workspace-view", label: "The whole screen" }
            ]
            textRole: "label"
            valueRole: "value"
            currentIndex: popup.gradient["relative-to"] === "workspace-view" ? 1 : 0
            onActivated: popup.edited({ "relative-to": currentValue })
        }

        QQC2.Label { text: "Blend colors in:"; Layout.alignment: Qt.AlignRight }

        QQC2.ComboBox {
            model: popup.spaces
            textRole: "label"
            valueRole: "value"
            currentIndex: Math.max(0, popup.spaces.findIndex(space => space.value === popup.gradient["in"]))
            onActivated: popup.edited({ "in": currentValue })
        }
    }
}
