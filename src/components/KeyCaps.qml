import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

RowLayout {
    id: caps

    property string keyName
    property string modKey: "Super"

    readonly property var symbols: ({
        "left": "←", "right": "→", "up": "↑", "down": "↓",
        "return": "Enter", "page_up": "PgUp", "page_down": "PgDn", "prior": "PgUp", "next": "PgDn",
        "bracketleft": "[", "bracketright": "]", "comma": ",", "period": ".", "minus": "−", "equal": "=",
        "slash": "/", "backslash": "\\", "semicolon": ";", "apostrophe": "'", "grave": "`", "space": "Space",
        "escape": "Esc", "backspace": "⌫", "delete": "Del", "tab": "Tab", "print": "PrtSc",
        "wheelscrollup": "Wheel ↑", "wheelscrolldown": "Wheel ↓", "wheelscrollleft": "Wheel ←", "wheelscrollright": "Wheel →",
        "touchpadscrollup": "Swipe ↑", "touchpadscrolldown": "Swipe ↓", "touchpadscrollleft": "Swipe ←", "touchpadscrollright": "Swipe →",
        "mouseleft": "Left click", "mouseright": "Right click", "mousemiddle": "Middle click", "mouseback": "Back button", "mouseforward": "Forward button"
    })

    function label(part) {
        const lower = part.toLowerCase();
        if (lower === "mod") {
            return caps.modKey === "Super" ? "Meta" : caps.modKey;
        }
        if (lower === "super" || lower === "win") {
            return "Meta";
        }
        return symbols[lower] || (part.length === 1 ? part.toUpperCase() : part);
    }

    spacing: Kirigami.Units.smallSpacing / 2

    Repeater {
        model: caps.keyName.length ? caps.keyName.split("+") : []

        Rectangle {
            required property string modelData
            readonly property bool isMod: modelData.toLowerCase() === "mod"

            implicitWidth: Math.max(implicitHeight, keyLabel.implicitWidth + Kirigami.Units.largeSpacing)
            implicitHeight: keyLabel.implicitHeight + Kirigami.Units.smallSpacing * 2
            radius: Kirigami.Units.cornerRadius
            color: isMod ? Qt.alpha(Kirigami.Theme.highlightColor, 0.2) : Qt.alpha(Kirigami.Theme.textColor, 0.07)
            border.color: isMod ? Kirigami.Theme.highlightColor : Qt.alpha(Kirigami.Theme.textColor, 0.25)

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: 1
                height: 2
                radius: 1
                color: Qt.alpha(Kirigami.Theme.textColor, 0.15)
            }

            QQC2.Label {
                id: keyLabel
                anchors.centerIn: parent
                text: caps.label(parent.modelData)
                font.pointSize: text.length === 1 ? Kirigami.Theme.defaultFont.pointSize : Kirigami.Theme.smallFont.pointSize
                font.weight: Font.DemiBold
            }
        }
    }
}
