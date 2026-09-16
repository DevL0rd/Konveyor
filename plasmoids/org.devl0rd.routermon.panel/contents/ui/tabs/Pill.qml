import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PlasmaComponents

Rectangle {
    id: pill

    property alias text: pillLabel.text
    property color tint: Kirigami.Theme.textColor
    property real strength: 0.1

    implicitWidth: pillLabel.implicitWidth + Kirigami.Units.smallSpacing * 3
    implicitHeight: pillLabel.implicitHeight + Kirigami.Units.smallSpacing
    radius: height / 2
    color: Qt.alpha(tint, strength)

    PlasmaComponents.Label {
        id: pillLabel
        anchors.centerIn: parent
        font.pointSize: Kirigami.Theme.smallFont.pointSize
        font.weight: Font.DemiBold
        color: pill.tint === Kirigami.Theme.textColor ? Kirigami.Theme.textColor : pill.tint
    }
}
