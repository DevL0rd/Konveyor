import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PlasmaComponents

RowLayout {
    id: header

    property string title
    property string trailing
    property string actionText
    signal actionClicked()

    Layout.fillWidth: true
    Layout.topMargin: Kirigami.Units.smallSpacing
    spacing: Kirigami.Units.smallSpacing

    PlasmaComponents.Label {
        text: header.title
        font.pointSize: Kirigami.Theme.smallFont.pointSize
        font.weight: Font.DemiBold
        font.capitalization: Font.AllUppercase
        font.letterSpacing: 0.6
        opacity: 0.65
    }
    PlasmaComponents.Label {
        visible: header.trailing !== ""
        text: header.trailing
        font: Kirigami.Theme.smallFont
        opacity: 0.45
    }
    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 1
        color: Qt.alpha(Kirigami.Theme.textColor, 0.1)
    }
    PlasmaComponents.ToolButton {
        visible: header.actionText !== ""
        text: header.actionText
        font: Kirigami.Theme.smallFont
        onClicked: header.actionClicked()
    }
}
