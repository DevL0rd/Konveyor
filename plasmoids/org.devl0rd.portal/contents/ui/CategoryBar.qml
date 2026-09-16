import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PlasmaComponents

ListView {
    id: bar

    property var categories: []
    property string current
    signal picked(string label)

    implicitHeight: Kirigami.Units.gridUnit * 1.6
    orientation: ListView.Horizontal
    spacing: Kirigami.Units.smallSpacing
    clip: true
    boundsBehavior: Flickable.StopAtBounds
    model: categories
    currentIndex: categories.findIndex(c => c.label === current)
    highlightFollowsCurrentItem: false
    onCurrentIndexChanged: if (currentIndex >= 0) positionViewAtIndex(currentIndex, ListView.Contain)

    delegate: Rectangle {
        id: pill
        required property var modelData
        required property int index
        readonly property bool selected: modelData.label === bar.current

        height: bar.height
        width: label.implicitWidth + Kirigami.Units.largeSpacing * 2
        radius: height / 2
        color: selected ? Qt.alpha(Kirigami.Theme.highlightColor, 0.24)
             : hover.hovered ? Qt.alpha(Kirigami.Theme.textColor, 0.1) : Qt.alpha(Kirigami.Theme.textColor, 0.05)
        border.width: 1
        border.color: selected ? Qt.alpha(Kirigami.Theme.highlightColor, 0.5) : "transparent"
        Behavior on color { ColorAnimation { duration: 120 } }

        PlasmaComponents.Label {
            id: label
            anchors.centerIn: parent
            text: pill.modelData.allApps ? i18n("All") : pill.modelData.label
            font.pointSize: Kirigami.Theme.smallFont.pointSize
            font.weight: pill.selected ? Font.DemiBold : Font.Normal
            opacity: pill.selected ? 1 : 0.8
        }
        HoverHandler { id: hover; cursorShape: Qt.PointingHandCursor }
        TapHandler { onTapped: bar.picked(pill.modelData.label) }
    }
}
