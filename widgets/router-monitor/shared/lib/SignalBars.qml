/*
 * Four-bar WiFi signal strength indicator driven by a 0..4 level.
 */
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

RowLayout {
    id: root
    property int level: 0       // 0..4
    property string tip: ""     // hover tooltip; falls back to "N/4"
    property color activeColor: Kirigami.Theme.highlightColor
    spacing: 1

    Repeater {
        model: 4
        Rectangle {
            Layout.alignment: Qt.AlignBottom
            width: Kirigami.Units.smallSpacing
            height: Kirigami.Units.smallSpacing * (index + 1.4)
            radius: 1
            color: index < root.level ? root.activeColor
                                       : Qt.alpha(Kirigami.Theme.textColor, 0.18)
        }
    }

    HoverHandler { id: hover }
    QQC2.ToolTip.visible: hover.hovered
    QQC2.ToolTip.text: root.tip !== "" ? root.tip : (root.level + "/4")
    QQC2.ToolTip.delay: 400
}
