import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import "lib/Format.js" as Fmt

RowLayout {
    id: bars
    property var values: []
    property string prefix
    property int highlight: -1
    property bool groupMatched: false
    Layout.fillWidth: true
    Layout.preferredHeight: Kirigami.Units.gridUnit * 2.2
    spacing: 3
    Repeater {
        model: bars.values.length
        Item {
            id: coreBar
            required property int index
            readonly property real value: bars.values[index] || 0
            readonly property bool marked: bars.highlight === index + 1 || (bars.groupMatched && bars.highlight < 0)
            Layout.fillWidth: true
            Layout.fillHeight: true
            Rectangle {
                anchors.fill: parent
                radius: 2
                color: Qt.alpha(Kirigami.Theme.textColor, 0.08)
                border.width: coreBar.marked ? 1.5 : 0
                border.color: Kirigami.Theme.highlightColor
            }
            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: Math.max(2, parent.height * Math.min(1, coreBar.value / 100))
                radius: 2
                color: Fmt.grad(coreBar.value)
                Behavior on height { NumberAnimation { duration: 350; easing.type: Easing.OutCubic } }
                Behavior on color { ColorAnimation { duration: 350 } }
            }
            HoverHandler { id: coreHover }
            QQC2.ToolTip.visible: coreHover.hovered
            QQC2.ToolTip.text: i18n("%1 %2 · %3%", bars.prefix, index + 1, Math.round(coreBar.value))
            QQC2.ToolTip.delay: 200
        }
    }
}
