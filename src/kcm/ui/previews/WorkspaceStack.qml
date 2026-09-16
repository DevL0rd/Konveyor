import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

Item {
    id: stack

    property var names: []
    property var outputs: []
    property bool emptyAbove: false
    property bool backAndForth: false
    property int current: 0

    readonly property var entries: {
        const list = [];
        if (emptyAbove) {
            list.push({ label: "Empty", output: "", empty: true });
        }
        names.forEach((name, i) => list.push({ label: name, output: outputs[i] || "", empty: false }));
        list.push({ label: "Workspace", output: "", empty: false });
        list.push({ label: "New workspace appears here", output: "", empty: true });
        return list;
    }

    Timer {
        interval: 1400
        repeat: true
        running: stack.visible
        onTriggered: stack.current = (stack.current + 1) % Math.max(1, stack.entries.length - 1)
    }

    MonitorFrame {
        id: frame
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        height: parent.height
        aspect: 16 / 10

        ListView {
            id: list
            anchors.fill: parent
            anchors.margins: Kirigami.Units.smallSpacing
            spacing: Kirigami.Units.smallSpacing
            interactive: false
            model: stack.entries
            currentIndex: stack.current
            highlightMoveDuration: Kirigami.Units.longDuration
            preferredHighlightBegin: height / 2 - Kirigami.Units.gridUnit * 1.5
            preferredHighlightEnd: height / 2 + Kirigami.Units.gridUnit * 1.5
            highlightRangeMode: ListView.StrictlyEnforceRange

            delegate: Rectangle {
                id: slot
                required property var modelData
                required property int index
                readonly property bool isCurrent: ListView.isCurrentItem

                width: list.width
                height: Kirigami.Units.gridUnit * 3
                radius: Kirigami.Units.cornerRadius
                color: modelData.empty ? "transparent" : Qt.alpha(Kirigami.Theme.backgroundColor, isCurrent ? 1 : 0.6)
                border.color: isCurrent ? Kirigami.Theme.highlightColor : Qt.alpha(Kirigami.Theme.textColor, 0.25)
                border.width: isCurrent ? 2 : 1
                opacity: modelData.empty ? 0.6 : 1

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: Kirigami.Units.smallSpacing
                    spacing: Kirigami.Units.smallSpacing

                    Repeater {
                        model: slot.modelData.empty ? 0 : 3

                        Rectangle {
                            Layout.fillHeight: true
                            Layout.preferredWidth: slot.height * 0.9
                            radius: 2
                            color: Qt.alpha(Kirigami.Theme.textColor, 0.08)
                        }
                    }

                    QQC2.Label {
                        text: slot.modelData.label
                        font: Kirigami.Theme.smallFont
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                        horizontalAlignment: slot.modelData.empty ? Text.AlignHCenter : Text.AlignLeft
                    }

                    Rectangle {
                        visible: slot.modelData.output.length > 0
                        implicitWidth: badge.implicitWidth + Kirigami.Units.smallSpacing * 2
                        implicitHeight: badge.implicitHeight + 2
                        radius: height / 2
                        color: Qt.alpha(Kirigami.Theme.highlightColor, 0.2)

                        QQC2.Label {
                            id: badge
                            anchors.centerIn: parent
                            text: slot.modelData.output
                            font: Kirigami.Theme.smallFont
                        }
                    }
                }
            }
        }
    }

    ColumnLayout {
        anchors.left: frame.right
        anchors.leftMargin: Kirigami.Units.gridUnit
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        spacing: Kirigami.Units.smallSpacing

        Kirigami.Heading {
            level: 4
            text: "Workspaces stack vertically"
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }

        QQC2.Label {
            text: "Each monitor has its own stack. Named workspaces stay put; unnamed ones come and go as you fill them."
            wrapMode: Text.Wrap
            opacity: 0.75
            Layout.fillWidth: true
        }

        QQC2.Label {
            visible: stack.backAndForth
            text: "Pressing a workspace's shortcut again takes you back."
            wrapMode: Text.Wrap
            color: Kirigami.Theme.highlightColor
            Layout.fillWidth: true
        }
    }
}
