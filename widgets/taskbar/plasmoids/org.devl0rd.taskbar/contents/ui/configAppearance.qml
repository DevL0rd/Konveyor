import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

ConfigPage {
    id: page

    title: i18n("Appearance")

    RowLayout {
        Kirigami.FormData.label: i18n("Icon size:")

        ConfigCheck {
            id: autoIconSize
            page: page
            key: "autoIconSize"
            text: i18n("Follow the panel")
        }

        ConfigNumber {
            id: iconSize
            page: page
            key: "iconSize"
            enabled: !autoIconSize.checked
            from: 12
            to: 128
        }
    }

    ConfigNumber {
        id: iconSpacing
        Kirigami.FormData.label: i18n("Space between icons:")
        page: page
        key: "iconSpacing"
        from: 0
        to: 32
    }

    ConfigNumber {
        id: buttonPadding
        Kirigami.FormData.label: i18n("Padding around each icon:")
        page: page
        key: "buttonPadding"
        from: 0
        to: 24
    }

    Item {
        Kirigami.FormData.isSection: true
    }

    ConfigChoice {
        id: highlightStyle
        Kirigami.FormData.label: i18n("Focused app:")
        page: page
        key: "highlightStyle"
        model: [i18n("Filled"), i18n("Outline"), i18n("No highlight")]
    }

    ConfigChoice {
        id: indicatorStyle
        Kirigami.FormData.label: i18n("Window indicators:")
        page: page
        key: "indicatorStyle"
        model: [i18n("A dot for each window"), i18n("A bar"), i18n("None")]
    }

    ConfigChoice {
        id: indicatorEdge
        Kirigami.FormData.label: i18n("Indicator position:")
        page: page
        key: "indicatorEdge"
        enabled: indicatorStyle.currentIndex !== 2
        model: [i18n("Next to the screen edge"), i18n("On the opposite side")]
    }

    Item {
        Kirigami.FormData.isSection: true
    }

    ConfigCheck {
        id: showSeparator
        Kirigami.FormData.label: i18n("Show:")
        page: page
        key: "showSeparator"
        text: i18n("A separator between the workspaces and the apps")
    }

    ConfigCheck {
        id: animations
        page: page
        key: "animations"
        text: i18n("Animations")
    }

    ConfigCheck {
        id: attentionPulse
        page: page
        key: "attentionPulse"
        enabled: animations.checked
        text: i18n("A pulse on apps that want attention")
    }
}
