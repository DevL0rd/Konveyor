import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PlasmaComponents

RowLayout {
    visible: !launcher.compact
    Layout.fillWidth: true
    spacing: Kirigami.Units.largeSpacing * 2
    opacity: launcher.contentProgress

    Repeater {
        model: launcher.searching ? [
            { key: "↵", text: i18n("Open") },
            { key: "Alt ↵", text: i18n("Actions") },
            { key: "Tab", text: i18n("Next group") },
            { key: "Esc", text: i18n("Clear") }
        ] : [
            { key: "↵", text: i18n("Open") },
            { key: "Alt ↵", text: i18n("Actions") },
            { key: "Ctrl P", text: i18n("Pin") },
            { key: "Ctrl ⇧ P", text: i18n("Pin to sidebar") },
            { key: "Tab", text: i18n("Next group") },
            { key: "Ctrl Tab", text: i18n("Next page") },
            { key: "Alt 1–" + launcher.pageDefs.length, text: i18n("Go to page") },
            { key: "Esc", text: i18n("Close") }
        ]
        delegate: RowLayout {
            required property var modelData
            spacing: Kirigami.Units.smallSpacing
            Rectangle {
                implicitWidth: keyLabel.implicitWidth + Kirigami.Units.largeSpacing
                implicitHeight: keyLabel.implicitHeight + Kirigami.Units.smallSpacing * 0.5
                radius: Kirigami.Units.cornerRadius
                color: launcher.well
                border.width: 1
                border.color: launcher.hairline
                PlasmaComponents.Label {
                    id: keyLabel
                    anchors.centerIn: parent
                    text: modelData.key
                    font.pointSize: Kirigami.Theme.smallFont.pointSize
                    font.weight: Font.DemiBold
                    opacity: 0.8
                }
            }
            PlasmaComponents.Label {
                text: modelData.text
                font.pointSize: Kirigami.Theme.smallFont.pointSize
                opacity: 0.55
            }
        }
    }
    Item { Layout.fillWidth: true }
    PlasmaComponents.Label {
        text: launcher.searching ? i18n("Prefixes: g games · a apps · f files · s packages · @ friends · = math · > command") : i18n("Type anywhere to search")
        font.pointSize: Kirigami.Theme.smallFont.pointSize
        opacity: 0.45
    }
}
