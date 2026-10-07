import QtQuick
import org.kde.kirigami as Kirigami

ConfigPage {
    id: page

    title: i18n("Behavior")

    ConfigChoice {
        id: groupMode
        Kirigami.FormData.label: i18n("Group windows:")
        page: page
        key: "groupMode"
        model: [i18n("Like Konveyor's group-app-windows"), i18n("Always group an app's neighbouring columns"), i18n("One icon per column")]
    }

    ConfigChoice {
        id: activeClick
        Kirigami.FormData.label: i18n("Clicking the focused app:")
        page: page
        key: "activeClick"
        model: [i18n("Minimizes it"), i18n("Does nothing"), i18n("Switches to its next window")]
    }

    ConfigChoice {
        id: middleClick
        Kirigami.FormData.label: i18n("Middle-click:")
        page: page
        key: "middleClick"
        model: [i18n("Closes the window"), i18n("Opens a new window"), i18n("Does nothing")]
    }

    ConfigCheck {
        id: wheelCyclesTasks
        Kirigami.FormData.label: i18n("Scroll wheel:")
        page: page
        key: "wheelCyclesTasks"
        text: i18n("Scroll over the apps to switch between them")
    }

    Item {
        Kirigami.FormData.isSection: true
    }

    ConfigChoice {
        id: onlyThisScreen
        Kirigami.FormData.label: i18n("Show windows from:")
        page: page
        key: "onlyThisScreen"
        flag: true
        model: [i18n("Every screen"), i18n("This screen only")]
    }

    ConfigCheck {
        id: showFloating
        Kirigami.FormData.label: i18n("Show:")
        page: page
        key: "showFloating"
        text: i18n("Floating windows")
    }

    ConfigCheck {
        id: showTooltips
        page: page
        key: "showTooltips"
        text: i18n("Tooltips")
    }

    ConfigCheck {
        id: showShortcutBadges
        page: page
        key: "showShortcutBadges"
        text: i18n("Shortcut numbers while Meta or Meta+Alt is held")
    }
}
