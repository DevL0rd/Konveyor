import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import "../../catalog/Kdl.js" as Kdl

RowLayout {
    id: field

    property var regex
    property string anyLabel: "Any title"
    property string placeholder: "Window title"
    signal edited(var regex)

    readonly property var parsed: regex === undefined || regex === null ? { mode: "any", text: "" } : Kdl.parseTextMatch(regex)
    property string mode: parsed.mode
    readonly property var modes: [
        { value: "any", label: anyLabel },
        { value: "is", label: "Is exactly" },
        { value: "contains", label: "Contains" },
        { value: "starts", label: "Starts with" },
        { value: "ends", label: "Ends with" },
        { value: "pattern", label: "Pattern" }
    ]

    function commit() {
        if (mode === "any") {
            edited(null);
        } else if (input.text.length > 0) {
            edited(Kdl.textMatch(mode, input.text));
        }
    }

    onParsedChanged: {
        mode = parsed.mode;
        input.text = parsed.text;
    }

    spacing: Kirigami.Units.smallSpacing

    QQC2.ComboBox {
        model: field.modes
        textRole: "label"
        valueRole: "value"
        currentIndex: Math.max(0, field.modes.findIndex(entry => entry.value === field.mode))
        onActivated: {
            field.mode = currentValue;
            field.commit();
            if (field.mode !== "any") {
                input.forceActiveFocus();
            }
        }
    }

    QQC2.TextField {
        id: input
        visible: field.mode !== "any"
        text: field.parsed.text
        placeholderText: field.mode === "pattern" ? "Regular expression" : field.placeholder
        font.family: field.mode === "pattern" ? "monospace" : Kirigami.Theme.defaultFont.family
        Layout.fillWidth: true
        Layout.minimumWidth: Kirigami.Units.gridUnit * 10
        onEditingFinished: field.commit()
    }
}
