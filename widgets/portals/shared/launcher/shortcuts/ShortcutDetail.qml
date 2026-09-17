import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PlasmaComponents
import org.kde.konveyor.components

ColumnLayout {
    id: detail

    property var entry: null
    property string section
    property bool animated: false
    readonly property var gesture: entry && entry.gesture ? entry.gesture : null

    spacing: Kirigami.Units.largeSpacing

    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: width * 0.62
        radius: Kirigami.Units.cornerRadius * 2
        color: launcher.well
        border.width: 1
        border.color: launcher.hairline

        GestureAnimation {
            id: gesturePreview
            anchors.fill: parent
            anchors.margins: Kirigami.Units.largeSpacing
            visible: detail.gesture !== null
            device: detail.gesture ? detail.gesture.device : "touchpad"
            gesture: detail.gesture ? (detail.gesture.motion === "tap" ? "tap-" + detail.gesture.action : detail.gesture.motion.replace("swipe-", "")) : "horizontal"
            fingers: detail.gesture ? detail.gesture.fingers : 3
            natural: detail.gesture ? detail.gesture.natural : true
            animated: detail.animated
            showCaption: false
        }
        ActionPreview {
            id: preview
            anchors.fill: parent
            anchors.margins: Kirigami.Units.largeSpacing
            actionId: detail.entry && detail.entry.id ? detail.entry.id : ""
            animated: detail.animated
        }
        Kirigami.Icon {
            visible: !preview.visible && !gesturePreview.visible
            anchors.centerIn: parent
            width: Kirigami.Units.iconSizes.huge
            height: width
            source: "input-keyboard-symbolic"
            color: launcher.ink
            isMask: true
            opacity: 0.25
        }
    }

    PlasmaComponents.Label {
        Layout.fillWidth: true
        visible: detail.entry !== null
        text: detail.section.toUpperCase()
        font.pointSize: Kirigami.Theme.smallFont.pointSize
        font.weight: Font.DemiBold
        font.letterSpacing: 0.8
        opacity: 0.5
    }
    Kirigami.Heading {
        Layout.fillWidth: true
        level: 3
        text: detail.entry ? detail.entry.action : i18n("Pick a shortcut")
        wrapMode: Text.Wrap
    }
    FingerCaps {
        visible: detail.gesture !== null
        fingers: detail.gesture ? detail.gesture.fingers : 1
        motion: detail.gesture ? detail.gesture.motion : ""
        device: detail.gesture ? detail.gesture.device : ""
    }
    Repeater {
        model: detail.entry && !detail.gesture ? detail.entry.keys : []
        KeyCaps {
            required property string modelData
            keyName: modelData
        }
    }
    PlasmaComponents.Label {
        Layout.fillWidth: true
        visible: detail.entry !== null && !preview.visible
        text: detail.gesture ? i18n("A Konveyor gesture. Change it in Settings → Touch & Gestures.") : i18n("A KDE shortcut. Change it in System Settings → Shortcuts.")
        wrapMode: Text.Wrap
        opacity: 0.55
    }
    Item {
        Layout.fillHeight: true
    }
}
