import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

Item {
    id: dial

    property real angle
    signal edited(real angle)

    implicitWidth: Kirigami.Units.gridUnit * 2.2
    implicitHeight: implicitWidth

    Accessible.role: Accessible.Dial
    Accessible.name: "Gradient angle " + Math.round(angle) + " degrees"

    Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: Qt.alpha(Kirigami.Theme.textColor, 0.06)
        border.color: Qt.alpha(Kirigami.Theme.textColor, 0.3)
    }

    Rectangle {
        width: 2
        height: dial.height / 2 - 4
        color: Kirigami.Theme.highlightColor
        antialiasing: true
        x: dial.width / 2 - 1
        y: 4
        transform: Rotation {
            origin.x: 1
            origin.y: dial.height / 2 - 4
            angle: dial.angle
        }
    }

    QQC2.Label {
        anchors.centerIn: parent
        text: Math.round(dial.angle) + "°"
        font: Kirigami.Theme.smallFont
    }

    MouseArea {
        anchors.fill: parent
        function update(mouse) {
            const dx = mouse.x - width / 2;
            const dy = mouse.y - height / 2;
            let degrees = Math.atan2(dx, -dy) * 180 / Math.PI;
            degrees = Math.round((degrees + 360) % 360 / 15) * 15 % 360;
            dial.angle = degrees;
        }
        onPressed: mouse => update(mouse)
        onPositionChanged: mouse => update(mouse)
        onReleased: dial.edited(dial.angle)
    }
}
