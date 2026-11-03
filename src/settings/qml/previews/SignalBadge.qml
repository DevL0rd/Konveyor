import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import "../catalog/Signals.js" as Signals

RowLayout {
    id: badge

    property var layers: []
    property bool showTips: true
    readonly property var sources: Signals.sourcesOf(layers)

    visible: sources.length > 0
    spacing: 2

    Repeater {
        model: badge.sources

        Rectangle {
            id: chip
            required property string modelData
            implicitWidth: Kirigami.Units.gridUnit
            implicitHeight: Kirigami.Units.gridUnit
            radius: 3
            color: Qt.rgba(10 / 255, 12 / 255, 14 / 255, 0.72)

            HoverHandler {
                id: chipHover
                enabled: badge.showTips
            }

            QQC2.ToolTip.text: Signals.badges[chip.modelData].tooltip
            QQC2.ToolTip.visible: chipHover.hovered
            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay

            Row {
                anchors.centerIn: parent
                visible: chip.modelData === "audio"
                spacing: 1

                Repeater {
                    model: [0.35, 0.8, 0.55, 1, 0.45]

                    Rectangle {
                        required property real modelData
                        anchors.verticalCenter: parent.verticalCenter
                        width: 1.5
                        height: Math.round(chip.height * 0.7 * modelData)
                        radius: 0.75
                        color: "#fcfcfc"
                    }
                }
            }

            Kirigami.Icon {
                anchors.centerIn: parent
                visible: chip.modelData !== "audio"
                width: Math.round(chip.width * 0.8)
                height: width
                source: Signals.badges[chip.modelData].icon || ""
                color: "#fcfcfc"
                isMask: true
            }
        }
    }
}
