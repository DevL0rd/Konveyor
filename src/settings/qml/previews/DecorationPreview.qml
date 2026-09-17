import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

RowLayout {
    id: preview

    property var values: ({})
    property real cornerRadius: 6

    spacing: Kirigami.Units.gridUnit

    Repeater {
        model: [
            { state: "active", caption: "Focused" },
            { state: "inactive", caption: "Other window" },
            { state: "urgent", caption: "Needs attention" }
        ]

        MockWindow {
            required property var modelData
            Layout.fillWidth: true
            Layout.fillHeight: true
            state_: modelData.state
            caption: modelData.caption
            ring: preview.values["focus-ring"]
            border: preview.values.border
            cornerRadius: preview.cornerRadius
        }
    }
}
