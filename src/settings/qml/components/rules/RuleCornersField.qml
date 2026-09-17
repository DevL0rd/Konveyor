import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import ".."

RowLayout {
    id: field

    property var radii
    signal edited(var values)

    spacing: Kirigami.Units.largeSpacing

    Segmented {
        Layout.alignment: Qt.AlignTop
        currentValue: field.radii ? "custom" : "default"
        options: [
            { value: "default", label: "Default" },
            { value: "custom", label: "Custom" }
        ]
        onChosen: value => field.edited(value === "default" ? null : [12, 12, 12, 12])
    }

    CornerRadiusEditor {
        visible: field.radii !== null
        radii: field.radii || [12, 12, 12, 12]
        onEdited: values => field.edited(values)
    }

    Item {
        Layout.fillWidth: true
    }
}
