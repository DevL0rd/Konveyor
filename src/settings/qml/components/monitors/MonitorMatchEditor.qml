import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import ".."
import "../../catalog/Kdl.js" as Kdl
import "MonitorSummary.js" as MonitorSummary
import org.kde.konveyor.settings

ColumnLayout {
    id: editor

    property string path
    property var node: ({ props: {} })
    property string heading
    signal removeRequested

    readonly property var props: node && node.props ? node.props : ({})
    readonly property var nameParsed: props.name === undefined ? { mode: "any", text: "" } : Kdl.parseTextMatch(props.name)
    property string nameMode: nameParsed.mode === "any" ? "any" : (nameParsed.mode === "is" ? "monitor" : "pattern")

    function write(changes) {
        const next = Object.assign({}, props, changes);
        for (const key of Object.keys(next)) {
            if (next[key] === null || next[key] === undefined) {
                delete next[key];
            }
        }
        SettingsStore.setNode(path, { name: "match", args: [], props: next });
    }

    onNameParsedChanged: nameMode = nameParsed.mode === "any" ? "any" : (nameParsed.mode === "is" ? "monitor" : "pattern")

    spacing: 0

    RowLayout {
        Layout.fillWidth: true
        Layout.leftMargin: Kirigami.Units.largeSpacing
        Layout.rightMargin: Kirigami.Units.smallSpacing
        Layout.topMargin: Kirigami.Units.smallSpacing

        ColumnLayout {
            spacing: 0
            Layout.fillWidth: true

            QQC2.Label {
                text: editor.heading
                font.bold: true
            }

            QQC2.Label {
                text: Kdl.titleCase(MonitorSummary.describeMatch(editor.node))
                opacity: 0.7
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }
        }

        QQC2.ToolButton {
            icon.name: "edit-delete-remove"
            text: "Remove"
            onClicked: editor.removeRequested()
        }
    }

    SettingRow {
        label: "Monitor name"
        iconName: "monitor"
        wideControl: true

        RowLayout {
            width: parent.width
            spacing: Kirigami.Units.smallSpacing

            Segmented {
                currentValue: editor.nameMode
                options: [
                    { value: "any", label: "Any" },
                    { value: "monitor", label: "This monitor" },
                    { value: "pattern", label: "Pattern" }
                ]
                onChosen: value => {
                    editor.nameMode = value;
                    if (value === "any") {
                        editor.write({ name: null });
                    }
                }
            }

            OutputPicker {
                visible: editor.nameMode === "monitor"
                value: editor.nameParsed.mode === "is" ? editor.nameParsed.text : ""
                onPicked: name => name.length ? editor.write({ name: Kdl.textMatch("is", name) }) : undefined
            }

            QQC2.TextField {
                visible: editor.nameMode === "pattern"
                text: editor.props.name || ""
                placeholderText: "Regular expression, e.g. ^DP-"
                font.family: "monospace"
                Layout.fillWidth: true
                onEditingFinished: editor.write({ name: text.length ? text : null })
            }

            Item {
                visible: editor.nameMode !== "pattern"
                Layout.fillWidth: true
            }
        }
    }

    BoundRow {
        label: "Wider than"
        description: "Shape of the screen, width divided by height"
        iconName: "zoom-fit-width"
        aspect: true
        fallback: 2
        bound: editor.props["aspect-ratio-above"]
        onChanged: value => editor.write({ "aspect-ratio-above": value })
    }

    BoundRow {
        label: "Narrower than"
        iconName: "zoom-fit-height"
        aspect: true
        fallback: 2
        bound: editor.props["aspect-ratio-below"]
        onChanged: value => editor.write({ "aspect-ratio-below": value })
    }

    BoundRow {
        label: "More pixels wide than"
        iconName: "distribute-horizontal-x"
        fallback: 2560
        bound: editor.props["width-above"]
        onChanged: value => editor.write({ "width-above": value })
    }

    BoundRow {
        label: "Fewer pixels wide than"
        iconName: "distribute-horizontal-x"
        fallback: 2560
        bound: editor.props["width-below"]
        onChanged: value => editor.write({ "width-below": value })
    }

    BoundRow {
        label: "More pixels tall than"
        iconName: "distribute-vertical-y"
        fallback: 1440
        bound: editor.props["height-above"]
        onChanged: value => editor.write({ "height-above": value })
    }

    BoundRow {
        label: "Fewer pixels tall than"
        iconName: "distribute-vertical-y"
        fallback: 1440
        bound: editor.props["height-below"]
        onChanged: value => editor.write({ "height-below": value })
    }
}
