import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

Item {
    id: mock

    property string caption
    property var ring
    property var border
    property string state_: "inactive"
    property real cornerRadius: 6
    property real zoom: 1.5
    readonly property real ringWidth: ring && ring.enabled && state_ !== "inactive" ? ring.width * zoom : 0
    readonly property real borderWidth: border && border.enabled ? border.width * zoom : 0

    PaintFrame {
        anchors.fill: parent
        thickness: mock.ringWidth
        radius: mock.cornerRadius + mock.borderWidth
        paint: mock.ring ? mock.ring[mock.state_ === "urgent" ? "urgent" : "active"] : null
    }

    PaintFrame {
        anchors.fill: parent
        anchors.margins: mock.ringWidth
        thickness: mock.borderWidth
        radius: mock.cornerRadius
        paint: mock.border ? mock.border[mock.state_] : null
    }

    Rectangle {
        id: body
        anchors.fill: parent
        anchors.margins: mock.ringWidth + mock.borderWidth
        radius: mock.cornerRadius
        color: Kirigami.Theme.backgroundColor
        clip: true

        Rectangle {
            width: parent.width
            height: Kirigami.Units.gridUnit * 1.2
            color: Qt.alpha(Kirigami.Theme.textColor, mock.state_ === "active" ? 0.14 : 0.07)

            QQC2.Label {
                anchors.centerIn: parent
                text: mock.caption
                font: Kirigami.Theme.smallFont
                opacity: mock.state_ === "active" ? 1 : 0.6
            }
        }

        Column {
            x: Kirigami.Units.smallSpacing * 2
            y: Kirigami.Units.gridUnit * 1.8
            spacing: 5

            Repeater {
                model: [0.7, 0.5, 0.62, 0.35]

                Rectangle {
                    required property real modelData
                    width: body.width * modelData
                    height: 4
                    radius: 2
                    color: Qt.alpha(Kirigami.Theme.textColor, 0.15)
                }
            }
        }
    }
}
