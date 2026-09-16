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

    contentItem: Kirigami.FormLayout {
        QQC2.ComboBox {
            Kirigami.FormData.label: "Spread across:"
            model: [
                { value: "window", label: "Each window" },
                { value: "workspace-view", label: "The whole screen" }
            ]
            textRole: "label"
            valueRole: "value"
            currentIndex: popup.gradient["relative-to"] === "workspace-view" ? 1 : 0
            onActivated: popup.edited({ "relative-to": currentValue })
        }

        QQC2.ComboBox {
            Kirigami.FormData.label: "Blend colors in:"
            model: popup.spaces
            textRole: "label"
            valueRole: "value"
            currentIndex: Math.max(0, popup.spaces.findIndex(space => space.value === popup.gradient["in"]))
            onActivated: popup.edited({ "in": currentValue })
        }
    }
}
