import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import "../catalog/Kdl.js" as Kdl

ColumnLayout {
    id: editor

    property var sizes: []
    property real screenLength: 1920
    property string addLabel: "Add a size"
    signal edited(var sizes)

    function replaced(index, size) {
        const next = editor.sizes.slice();
        next[index] = size;
        return next;
    }

    function fraction(size) {
        return size.kind === "fixed" ? Math.min(1, size.value / editor.screenLength) : Math.min(1, size.value);
    }

    function fromFraction(kind, value) {
        const clamped = Math.max(0.05, Math.min(1, value));
        return kind === "fixed" ? { kind: kind, value: Math.round(clamped * editor.screenLength / 10) * 10 } : { kind: kind, value: Math.round(clamped * 100) / 100 };
    }

    spacing: Kirigami.Units.smallSpacing

    Repeater {
        model: editor.sizes

        RowLayout {
            id: entry

            required property var modelData
            required property int index
            property real dragFraction: -1
            readonly property real shown: dragFraction >= 0 ? dragFraction : editor.fraction(modelData)

            Layout.fillWidth: true
            spacing: Kirigami.Units.smallSpacing

            ColumnLayout {
                spacing: 0

                QQC2.ToolButton {
                    icon.name: "go-up"
                    enabled: entry.index > 0
                    display: QQC2.AbstractButton.IconOnly
                    text: "Move up"
                    implicitHeight: Kirigami.Units.gridUnit
                    implicitWidth: Kirigami.Units.gridUnit * 1.2
                    icon.width: Kirigami.Units.iconSizes.small
                    icon.height: Kirigami.Units.iconSizes.small
                    padding: 0
                    onClicked: {
                        const next = editor.sizes.slice();
                        next.splice(entry.index - 1, 0, next.splice(entry.index, 1)[0]);
                        editor.edited(next);
                    }
                }

                QQC2.ToolButton {
                    icon.name: "go-down"
                    enabled: entry.index < editor.sizes.length - 1
                    display: QQC2.AbstractButton.IconOnly
                    text: "Move down"
                    implicitHeight: Kirigami.Units.gridUnit
                    implicitWidth: Kirigami.Units.gridUnit * 1.2
                    icon.width: Kirigami.Units.iconSizes.small
                    icon.height: Kirigami.Units.iconSizes.small
                    padding: 0
                    onClicked: {
                        const next = editor.sizes.slice();
                        next.splice(entry.index + 1, 0, next.splice(entry.index, 1)[0]);
                        editor.edited(next);
                    }
                }
            }

            Item {
                id: track
                Layout.fillWidth: true
                implicitHeight: Kirigami.Units.gridUnit * 1.8

                Rectangle {
                    anchors.fill: parent
                    radius: Kirigami.Units.cornerRadius
                    color: Qt.alpha(Kirigami.Theme.textColor, 0.06)
                    border.color: Qt.alpha(Kirigami.Theme.textColor, 0.15)
                }

                Rectangle {
                    id: bar
                    width: Math.max(handle.width, track.width * entry.shown)
                    height: parent.height
                    radius: Kirigami.Units.cornerRadius
                    color: Qt.alpha(Kirigami.Theme.highlightColor, 0.3)
                    border.color: Kirigami.Theme.highlightColor

                    Behavior on width {
                        enabled: entry.dragFraction < 0
                        NumberAnimation {
                            duration: Kirigami.Units.shortDuration
                        }
                    }

                    QQC2.Label {
                        anchors.left: parent.left
                        anchors.leftMargin: Kirigami.Units.smallSpacing
                        anchors.verticalCenter: parent.verticalCenter
                        text: Kdl.sizeLabel(editor.fromFraction(entry.modelData.kind, entry.shown))
                        font.bold: true
                    }
                }

                Rectangle {
                    id: handle
                    x: bar.width - width / 2
                    anchors.verticalCenter: parent.verticalCenter
                    width: Kirigami.Units.gridUnit * 0.6
                    height: parent.height + 4
                    radius: width / 2
                    color: Kirigami.Theme.highlightColor

                    MouseArea {
                        anchors.fill: parent
                        anchors.margins: -Kirigami.Units.smallSpacing
                        cursorShape: Qt.SizeHorCursor
                        preventStealing: true
                        onPositionChanged: mouse => {
                            const x = mapToItem(track, mouse.x, mouse.y).x;
                            entry.dragFraction = Math.max(0.05, Math.min(1, x / track.width));
                        }
                        onReleased: {
                            if (entry.dragFraction >= 0) {
                                editor.edited(editor.replaced(entry.index, editor.fromFraction(entry.modelData.kind, entry.dragFraction)));
                            }
                            entry.dragFraction = -1;
                        }
                    }
                }
            }

            Segmented {
                currentValue: entry.modelData.kind
                options: [{ value: "proportion", label: "%" }, { value: "fixed", label: "px" }]
                onChosen: value => editor.edited(editor.replaced(entry.index, editor.fromFraction(value, editor.fraction(entry.modelData))))
            }

            QQC2.ToolButton {
                icon.name: "edit-delete-remove"
                display: QQC2.AbstractButton.IconOnly
                text: "Remove this size"
                enabled: editor.sizes.length > 1
                onClicked: {
                    const next = editor.sizes.slice();
                    next.splice(entry.index, 1);
                    editor.edited(next);
                }
                QQC2.ToolTip.text: text
                QQC2.ToolTip.visible: hovered
            }
        }
    }

    QQC2.Button {
        icon.name: "list-add"
        text: editor.addLabel
        onClicked: editor.edited(editor.sizes.concat([{ kind: "proportion", value: 0.5 }]))
    }
}
