import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PlasmaComponents

ColumnLayout {
    visible: page.letters.length > 4
    Layout.fillHeight: true
    Layout.fillWidth: false
    Layout.preferredWidth: Kirigami.Units.gridUnit * 1.4
    spacing: 0
    Repeater {
        model: page.letters
        MouseArea {
            id: letter
            required property var modelData
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.maximumHeight: Kirigami.Units.gridUnit * 1.3
            hoverEnabled: modelData.row >= 0
            enabled: modelData.row >= 0
            onClicked: page.jump(modelData.row)
            Rectangle {
                anchors.centerIn: parent
                width: Math.min(parent.width, parent.height)
                height: width
                radius: width / 2
                color: letter.containsMouse ? launcher.selectedFill : "transparent"
            }
            PlasmaComponents.Label {
                anchors.centerIn: parent
                text: letter.modelData.key
                font.pointSize: Kirigami.Theme.smallFont.pointSize
                font.weight: Font.DemiBold
                opacity: letter.modelData.row < 0 ? 0.18 : letter.containsMouse ? 1 : 0.55
            }
        }
    }
    Item { Layout.fillHeight: true }
}
