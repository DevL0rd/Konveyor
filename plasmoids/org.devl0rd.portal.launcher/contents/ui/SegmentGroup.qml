import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Rectangle {
    default property alias content: groupRow.data
    implicitWidth: groupRow.implicitWidth + Kirigami.Units.smallSpacing
    implicitHeight: groupRow.implicitHeight + Kirigami.Units.smallSpacing
    radius: height / 2
    color: launcher.well
    border.width: 1
    border.color: launcher.hairline
    RowLayout {
        id: groupRow
        anchors.centerIn: parent
        spacing: 0
    }
}
