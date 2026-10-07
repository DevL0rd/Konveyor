import QtQuick
import org.kde.kirigami as Kirigami

Item {
    id: indicator

    property int count: 0
    property bool active: false
    property bool attention: false
    property bool minimized: false
    property bool vertical: false
    property real length: 24

    readonly property int dot: Math.max(3, Math.round(Kirigami.Units.smallSpacing * 0.9))
    readonly property int shown: Math.min(count, 3)
    readonly property color tint: attention ? Kirigami.Theme.neutralTextColor : active ? Kirigami.Theme.highlightColor : Kirigami.Theme.textColor

    implicitWidth: vertical ? dot : length
    implicitHeight: vertical ? length : dot

    Grid {
        anchors.centerIn: parent
        rows: indicator.vertical ? 3 : 1
        columns: indicator.vertical ? 1 : 3
        spacing: Math.max(2, Math.round(indicator.dot * 0.6))

        Repeater {
            model: indicator.shown

            Rectangle {
                required property int index
                readonly property bool lead: index === 0 && indicator.active
                readonly property real span: lead ? Math.round(indicator.length * 0.42) : indicator.dot

                width: indicator.vertical ? indicator.dot : span
                height: indicator.vertical ? span : indicator.dot
                radius: indicator.dot / 2
                color: indicator.minimized && !indicator.active ? "transparent" : indicator.tint
                border.width: indicator.minimized && !indicator.active ? 1 : 0
                border.color: indicator.tint
                opacity: indicator.active || indicator.attention ? 1 : 0.55

                Behavior on width {
                    NumberAnimation { duration: Kirigami.Units.longDuration; easing.type: Easing.OutCubic }
                }
                Behavior on height {
                    NumberAnimation { duration: Kirigami.Units.longDuration; easing.type: Easing.OutCubic }
                }
                Behavior on color {
                    ColorAnimation { duration: Kirigami.Units.longDuration }
                }
                Behavior on opacity {
                    NumberAnimation { duration: Kirigami.Units.shortDuration }
                }
            }
        }
    }
}
