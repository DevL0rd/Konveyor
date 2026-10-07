import QtQuick
import org.kde.plasma.configuration

ConfigModel {
    ConfigCategory {
        name: i18n("Appearance")
        icon: "preferences-desktop-color"
        source: "configAppearance.qml"
    }
    ConfigCategory {
        name: i18n("Behavior")
        icon: "preferences-desktop"
        source: "configBehavior.qml"
    }
    ConfigCategory {
        name: i18n("Workspaces")
        icon: "virtual-desktops"
        source: "configWorkspaces.qml"
    }
    ConfigCategory {
        name: i18n("Pinned Apps")
        icon: "window-pin"
        source: "configPins.qml"
    }
}
