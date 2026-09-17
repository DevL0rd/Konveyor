/*
 * A compact stat card: small icon + label on top, large value + unit below.
 * Optional sub-line and value colour. Matches Plasma's card altitude.
 */
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PlasmaComponents

Item {
    id: root

    property string label: ""
    property string value: ""
    property string unit: ""
    property string sub: ""
    property string iconName: ""
    property color valueColor: Kirigami.Theme.textColor
    property alias background: bg.visible

    implicitWidth: Kirigami.Units.gridUnit * 6
    implicitHeight: Kirigami.Units.gridUnit * 4

    Rectangle {
        id: bg
        anchors.fill: parent
        radius: Kirigami.Units.smallSpacing
        color: Kirigami.Theme.backgroundColor
        opacity: 0.4
        border.width: 1
        border.color: Qt.alpha(Kirigami.Theme.textColor, 0.08)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Kirigami.Units.smallSpacing
        spacing: 0

        RowLayout {
            spacing: Kirigami.Units.smallSpacing
            Layout.fillWidth: true
            Kirigami.Icon {
                source: root.iconName
                visible: root.iconName !== ""
                Layout.preferredWidth: Kirigami.Units.iconSizes.small
                Layout.preferredHeight: Kirigami.Units.iconSizes.small
            }
            PlasmaComponents.Label {
                text: root.label
                elide: Text.ElideRight
                opacity: 0.75
                font: Kirigami.Theme.smallFont
                Layout.fillWidth: true
            }
        }

        Item { Layout.fillHeight: true }

        RowLayout {
            spacing: Kirigami.Units.smallSpacing / 2
            Layout.fillWidth: true
            PlasmaComponents.Label {
                text: root.value
                color: root.valueColor
                font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.5
                font.weight: Font.DemiBold
            }
            PlasmaComponents.Label {
                text: root.unit
                opacity: 0.7
                Layout.alignment: Qt.AlignBottom
                bottomPadding: Kirigami.Units.smallSpacing
                visible: root.unit !== ""
            }
        }

        PlasmaComponents.Label {
            text: root.sub
            visible: root.sub !== ""
            opacity: 0.6
            font: Kirigami.Theme.smallFont
            elide: Text.ElideRight
            Layout.fillWidth: true
        }
    }
}
