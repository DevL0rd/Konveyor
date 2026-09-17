import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import "../previews"

GridLayout {
    id: editor

    property var struts: ({ left: 0, right: 0, top: 0, bottom: 0 })
    property real screenWidth: 1920
    property real screenHeight: 1080
    signal edited(var struts)

    function changed(side, value) {
        const next = Object.assign({}, editor.struts);
        next[side] = value;
        editor.edited(next);
    }

    columns: 3
    rowSpacing: Kirigami.Units.smallSpacing
    columnSpacing: Kirigami.Units.smallSpacing

    component Zone: Rectangle {
        color: Qt.alpha(Kirigami.Theme.neutralTextColor, 0.35)
        border.color: Kirigami.Theme.neutralTextColor
        border.width: 1
    }

    component EdgeSpin: QQC2.SpinBox {
        required property string side
        property real current
        signal picked(string side, int value)
        from: -2000
        to: 2000
        stepSize: 8
        editable: true
        value: Math.round(current)
        textFromValue: (number, locale) => number + " px"
        valueFromText: (text, locale) => parseInt(text)
        onValueModified: picked(side, value)
        Accessible.name: side + " edge space"
    }

    Item {
        implicitWidth: 1
    }

    EdgeSpin {
        side: "top"
        current: editor.struts.top || 0
        onPicked: (side, value) => editor.changed(side, value)
        Layout.alignment: Qt.AlignHCenter
    }

    Item {
        implicitWidth: 1
    }

    EdgeSpin {
        side: "left"
        current: editor.struts.left || 0
        onPicked: (side, value) => editor.changed(side, value)
        Layout.alignment: Qt.AlignVCenter
    }

    MonitorFrame {
        id: frame
        Layout.alignment: Qt.AlignCenter
        height: Kirigami.Units.gridUnit * 8
        Layout.preferredHeight: height
        Layout.preferredWidth: width
        aspect: editor.screenWidth / Math.max(1, editor.screenHeight)

        readonly property real scaleX: (width - 10) / editor.screenWidth
        readonly property real scaleY: (height - 10) / editor.screenHeight

        Zone {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: Math.max(0, editor.struts.left * frame.scaleX)
            visible: width > 0
        }

        Zone {
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: Math.max(0, editor.struts.right * frame.scaleX)
            visible: width > 0
        }

        Zone {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: Math.max(0, editor.struts.top * frame.scaleY)
            visible: height > 0
        }

        Zone {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: Math.max(0, editor.struts.bottom * frame.scaleY)
            visible: height > 0
        }

        Row {
            x: Math.max(0, editor.struts.left * frame.scaleX) + 4
            y: Math.max(0, editor.struts.top * frame.scaleY) + 4
            width: parent.width - x - Math.max(0, editor.struts.right * frame.scaleX) - 4
            height: parent.height - y - Math.max(0, editor.struts.bottom * frame.scaleY) - 4
            spacing: 4

            Repeater {
                model: 3

                Rectangle {
                    required property int index
                    width: (parent.width - 8) / 3
                    height: parent.height
                    radius: 2
                    color: index === 1 ? Qt.alpha(Kirigami.Theme.highlightColor, 0.35) : Qt.alpha(Kirigami.Theme.textColor, 0.18)
                    border.color: index === 1 ? Kirigami.Theme.highlightColor : "transparent"
                }
            }
        }
    }

    EdgeSpin {
        side: "right"
        current: editor.struts.right || 0
        onPicked: (side, value) => editor.changed(side, value)
        Layout.alignment: Qt.AlignVCenter
    }

    Item {
        implicitWidth: 1
    }

    EdgeSpin {
        side: "bottom"
        current: editor.struts.bottom || 0
        onPicked: (side, value) => editor.changed(side, value)
        Layout.alignment: Qt.AlignHCenter
    }

    Item {
        implicitWidth: 1
    }
}
