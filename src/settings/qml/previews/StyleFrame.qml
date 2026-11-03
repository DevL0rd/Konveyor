import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.konveyor.settings

BorderStyleItem {
    property real radius: 0
    property var play: null

    eventTime: play ? play.time : -1
    eventFrame: play ? play.frame : Qt.rect(0, 0, 0, 0)
    eventSubject: play ? play.subject : Qt.rect(0, 0, 0, 0)
    eventFrom: play ? play.from : Qt.rect(0, 0, 0, 0)
    eventTo: play ? play.to : Qt.rect(0, 0, 0, 0)
    eventFarthest: play ? play.farthest : 0
    eventLength: play ? play.length : 0
    radii: Qt.vector4d(radius, radius, radius, radius)
    reduceMotion: SettingsStore.values["animations/reduce-motion"] === true || SettingsStore.values["animations/enabled"] === false
    theme: ({
        "accent": Kirigami.Theme.highlightColor,
        "focus": Kirigami.Theme.focusColor,
        "hover": Kirigami.Theme.hoverColor,
        "window": Kirigami.Theme.backgroundColor,
        "window-text": Kirigami.Theme.textColor,
        "inactive-text": Kirigami.Theme.disabledTextColor
    })
}
