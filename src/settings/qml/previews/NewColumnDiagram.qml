import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.konveyor.components

Item {
    id: diagram

    property string side: "left"
    property bool running: true
    property bool opened: false

    Timer {
        interval: 1300
        repeat: true
        running: diagram.running
        onTriggered: diagram.opened = !diagram.opened
        onRunningChanged: if (!running) diagram.opened = true
    }

    MiniColumns {
        anchors.fill: parent
        active: false
        columns: {
            const old = [{ width: 0.2 }, { width: 0.26, focused: !diagram.opened }, { width: 0.2 }];
            if (!diagram.opened) {
                return old;
            }
            old[1] = { width: 0.26 };
            const fresh = { width: 0.18, focused: true };
            old.splice(diagram.side === "left" ? 1 : 2, 0, fresh);
            return old;
        }
    }

    Kirigami.Icon {
        source: diagram.side === "left" ? "go-previous" : "go-next"
        width: Kirigami.Units.iconSizes.small
        height: width
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 2
        opacity: diagram.opened ? 1 : 0.3

        Behavior on opacity {
            NumberAnimation {
                duration: Kirigami.Units.shortDuration
            }
        }
    }
}
