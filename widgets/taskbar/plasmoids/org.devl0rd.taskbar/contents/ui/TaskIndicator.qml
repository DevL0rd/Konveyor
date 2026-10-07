import QtQuick
import org.kde.kirigami as Kirigami

Item {
    id: indicator

    property TaskLook look
    property int count: 0
    property bool active: false
    property bool attention: false
    property bool minimized: false
    property real length: 24

    readonly property bool vertical: look.vertical
    readonly property bool bar: look.indicatorStyle === 1
    readonly property int dot: Math.max(3, Math.round(Kirigami.Units.smallSpacing * 0.9))
    readonly property int shown: bar ? Math.min(count, 1) : Math.min(count, 3)
    readonly property color tint: attention ? Kirigami.Theme.neutralTextColor : active ? Kirigami.Theme.highlightColor : Kirigami.Theme.textColor

    visible: look.indicatorStyle !== 2
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
                readonly property bool lead: index === 0 && (indicator.active || indicator.bar)
                readonly property real span: !lead ? indicator.dot
                    : Math.round(indicator.length * (indicator.bar ? (indicator.active ? 0.5 : 0.28) : 0.42))
                readonly property bool hollow: indicator.minimized && !indicator.active

                width: indicator.vertical ? indicator.dot : span
                height: indicator.vertical ? span : indicator.dot
                radius: indicator.dot / 2
                color: hollow ? "transparent" : indicator.tint
                border.width: hollow ? 1 : 0
                border.color: indicator.tint
                opacity: indicator.active || indicator.attention ? 1 : 0.55

                Behavior on width {
                    NumberAnimation { duration: indicator.look.longDuration; easing.type: Easing.OutCubic }
                }
                Behavior on height {
                    NumberAnimation { duration: indicator.look.longDuration; easing.type: Easing.OutCubic }
                }
                Behavior on color {
                    ColorAnimation { duration: indicator.look.longDuration }
                }
                Behavior on opacity {
                    NumberAnimation { duration: indicator.look.shortDuration }
                }
            }
        }
    }
}
