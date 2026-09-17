import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PlasmaComponents
import org.kde.konveyor.components
import "../lib/Highlight.js" as Highlight

MouseArea {
    id: row

    required property var entry
    property string query
    property bool selected: false
    signal hovered()

    implicitHeight: Math.max(keys.implicitHeight, label.implicitHeight) + Kirigami.Units.smallSpacing * 2.5
    hoverEnabled: true
    onContainsMouseChanged: if (containsMouse) row.hovered()

    Rectangle {
        anchors.fill: parent
        radius: Kirigami.Units.cornerRadius * 1.5
        color: row.selected ? launcher.selectedFill : row.containsMouse ? launcher.hoverFill : "transparent"
        border.width: row.selected ? 1 : 0
        border.color: launcher.selectedLine
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Kirigami.Units.largeSpacing
        anchors.rightMargin: Kirigami.Units.largeSpacing
        spacing: Kirigami.Units.largeSpacing

        Loader {
            active: !!row.entry.id
            Layout.preferredWidth: Kirigami.Units.gridUnit * 1.9
            Layout.preferredHeight: Kirigami.Units.gridUnit * 1.1
            sourceComponent: ActionPreview {
                actionId: row.entry.id
            }
        }
        Kirigami.Icon {
            visible: !row.entry.id
            Layout.preferredWidth: Kirigami.Units.gridUnit * 1.9
            Layout.preferredHeight: Kirigami.Units.iconSizes.small
            source: row.entry.gesture ? (row.entry.gesture.device === "touchscreen" ? "input-touchscreen-symbolic" : "input-touchpad-symbolic") : "input-keyboard-symbolic"
            color: launcher.ink
            isMask: true
            opacity: 0.4
        }

        PlasmaComponents.Label {
            id: label
            Layout.fillWidth: true
            text: row.query === "" ? row.entry.action : Highlight.mark(row.entry.action, row.query, launcher.ink)
            textFormat: row.query === "" ? Text.PlainText : Text.StyledText
            elide: Text.ElideRight
            opacity: row.selected ? 1 : 0.85
        }

        RowLayout {
            id: keys
            spacing: Kirigami.Units.smallSpacing
            PlasmaComponents.Label {
                visible: row.entry.keys.length > 1 && !row.entry.gesture
                text: "+" + (row.entry.keys.length - 1)
                font.pointSize: Kirigami.Theme.smallFont.pointSize
                opacity: 0.45
            }
            KeyCaps {
                visible: !row.entry.gesture
                keyName: row.entry.keys[0]
            }
            FingerCaps {
                visible: !!row.entry.gesture
                fingers: row.entry.gesture ? row.entry.gesture.fingers : 1
                motion: row.entry.gesture ? row.entry.gesture.motion : ""
                device: row.entry.gesture ? row.entry.gesture.device : ""
            }
        }
    }
}
