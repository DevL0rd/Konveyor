import QtQuick
import org.kde.kirigami as Kirigami

Item {
    id: mini

    property var columns: []
    property real offset: 0
    property real gap: 3
    property bool active: true
    property color accent: Kirigami.Theme.highlightColor

    clip: true

    Rectangle {
        anchors.fill: parent
        radius: 4
        color: Qt.alpha(Kirigami.Theme.textColor, 0.06)
        border.color: Qt.alpha(Kirigami.Theme.textColor, 0.25)
    }

    Item {
        id: strip
        anchors.fill: parent
        anchors.margins: mini.gap

        Repeater {
            model: mini.columns

            Rectangle {
                required property var modelData
                required property int index
                readonly property real start: {
                    let x = 0;
                    for (let i = 0; i < index; ++i) {
                        x += mini.columns[i].width * strip.width + mini.gap;
                    }
                    return x;
                }
                x: start + mini.offset * strip.width
                width: Math.max(2, modelData.width * strip.width)
                height: strip.height
                radius: 2
                opacity: modelData.ghost ? 0.5 : 1
                color: modelData.focused ? Qt.alpha(mini.accent, 0.35) : Qt.alpha(Kirigami.Theme.textColor, 0.18)
                border.width: modelData.focused ? 1.5 : 0
                border.color: mini.accent

                Repeater {
                    model: Math.max(0, (parent.modelData.stack || 1) - 1)

                    Rectangle {
                        required property int index
                        x: 0
                        width: parent.width
                        y: parent.height * (index + 1) / (parent.modelData.stack || 1) - 1
                        height: mini.gap
                        color: Qt.alpha(Kirigami.Theme.backgroundColor, 0.9)
                    }
                }

                Column {
                    visible: (parent.modelData.tabs || 0) > 0
                    x: -3
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 2

                    Repeater {
                        model: parent.parent.modelData.tabs || 0

                        Rectangle {
                            required property int index
                            width: 2
                            height: 6
                            radius: 1
                            color: index === 0 ? mini.accent : Qt.alpha(Kirigami.Theme.textColor, 0.45)
                        }
                    }
                }

                Behavior on x {
                    enabled: mini.active
                    NumberAnimation {
                        duration: Kirigami.Units.longDuration
                        easing.type: Easing.OutCubic
                    }
                }
            }
        }
    }
}
