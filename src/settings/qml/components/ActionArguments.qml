import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import "../catalog/Actions.js" as Actions
import org.kde.konveyor.settings

ColumnLayout {
    id: editor

    property var action
    property var args: []
    property var props: ({})
    signal edited(var args, var props)

    readonly property string kind: action ? action.argument : "none"
    readonly property string first: args.length ? String(args[0]) : ""
    readonly property var size: {
        const match = /^([+-]?)(\d+(?:\.\d+)?)(%?)$/.exec(first);
        return match ? { mode: match[1] === "+" ? "grow" : match[1] === "-" ? "shrink" : "set", value: Number(match[2]), unit: match[3] === "%" ? "percent" : "pixels" }
                     : { mode: "grow", value: 10, unit: "percent" };
    }
    readonly property var workspaceNames: SettingsStore.revision >= 0 ? SettingsStore.children("", "workspace").map(entry => String((entry.node.args || [])[0] || "")).filter(Boolean) : []
    readonly property bool namedWorkspace: args.length > 0 && typeof args[0] === "string" && !/^\d+$/.test(args[0])

    function emitArgs(list) {
        edited(list, props);
    }

    function writeSize(changes) {
        const next = Object.assign({}, size, changes);
        const sign = next.mode === "grow" ? "+" : next.mode === "shrink" ? "-" : "";
        emitArgs([sign + Math.round(next.value) + (next.unit === "percent" ? "%" : "")]);
    }

    function stripFieldCodes(exec) {
        return exec.replace(/%[a-zA-Z%]/g, "").replace(/\s+/g, " ").trim();
    }

    visible: kind !== "none" || (action !== null && action !== undefined && action.followProperty)
    spacing: Kirigami.Units.largeSpacing

    RowLayout {
        visible: editor.kind === "index"
        spacing: Kirigami.Units.smallSpacing

        QQC2.Label {
            text: "Position"
        }

        QQC2.SpinBox {
            from: 1
            to: 99
            editable: true
            value: Math.max(1, parseInt(editor.first) || 1)
            onValueModified: editor.emitArgs([value])
        }

        QQC2.Label {
            text: "counting from 1 at the start"
            opacity: 0.7
        }
    }

    ColumnLayout {
        visible: editor.kind === "workspace"
        spacing: Kirigami.Units.smallSpacing

        Segmented {
            currentValue: editor.namedWorkspace ? "name" : "number"
            options: [{ value: "number", label: "By number" }, { value: "name", label: "Named workspace" }]
            onChosen: value => editor.emitArgs(value === "name" ? [editor.workspaceNames.length ? editor.workspaceNames[0] : "main"] : [1])
        }

        QQC2.SpinBox {
            visible: !editor.namedWorkspace
            from: 1
            to: 255
            editable: true
            value: Math.max(1, parseInt(editor.first) || 1)
            onValueModified: editor.emitArgs([value])
        }

        QQC2.ComboBox {
            visible: editor.namedWorkspace
            editable: true
            model: editor.workspaceNames
            currentIndex: editor.workspaceNames.indexOf(editor.first)
            editText: editor.first
            implicitWidth: Kirigami.Units.gridUnit * 14
            onActivated: index => editor.emitArgs([editor.workspaceNames[index]])
            onAccepted: editor.emitArgs([editText.trim()])
        }

        QQC2.Label {
            visible: editor.namedWorkspace && editor.workspaceNames.length === 0
            text: "You have no named workspaces yet. Name them on the Workspaces page, or type a name here."
            wrapMode: Text.Wrap
            opacity: 0.7
            Layout.fillWidth: true
        }
    }

    ColumnLayout {
        visible: editor.kind === "size"
        spacing: Kirigami.Units.smallSpacing

        Segmented {
            currentValue: editor.size.mode
            options: [
                { value: "set", label: "Set to", icon: "zoom-fit-width" },
                { value: "grow", label: "Grow by", icon: "zoom-in" },
                { value: "shrink", label: "Shrink by", icon: "zoom-out" }
            ]
            onChosen: value => editor.writeSize({ mode: value })
        }

        RowLayout {
            spacing: Kirigami.Units.smallSpacing

            QQC2.SpinBox {
                from: 1
                to: editor.size.unit === "percent" ? 100 : 16000
                stepSize: editor.size.unit === "percent" ? 5 : 50
                editable: true
                value: editor.size.value
                onValueModified: editor.writeSize({ value: value })
            }

            Segmented {
                currentValue: editor.size.unit
                options: [{ value: "percent", label: "% of screen" }, { value: "pixels", label: "Pixels" }]
                onChosen: value => editor.writeSize({ unit: value, value: value === "percent" ? 10 : 100 })
            }
        }
    }

    RowLayout {
        visible: editor.kind === "monitor"
        spacing: Kirigami.Units.smallSpacing

        QQC2.Label {
            text: "Monitor"
        }

        OutputPicker {
            value: editor.first
            onPicked: name => editor.emitArgs(name.length ? [name] : [])
        }
    }

    Segmented {
        visible: editor.kind === "display"
        currentValue: editor.first === "tabbed" ? "tabbed" : "normal"
        options: [{ value: "normal", label: "Stacked", icon: "view-split-top-bottom" }, { value: "tabbed", label: "Tabs", icon: "tab-new" }]
        onChosen: value => editor.emitArgs([value])
    }

    ColumnLayout {
        visible: editor.kind === "command"
        spacing: Kirigami.Units.smallSpacing
        Layout.fillWidth: true

        RowLayout {
            Layout.fillWidth: true
            spacing: Kirigami.Units.smallSpacing

            QQC2.TextField {
                id: commandField
                Layout.fillWidth: true
                placeholderText: "Command to run, for example konsole"
                text: editor.args.map(String).join(" ")
                onTextEdited: editor.emitArgs(text.length ? [text] : [])
            }

            QQC2.Button {
                text: "Choose application…"
                icon.name: "applications-all"
                onClicked: appChooser.open()
            }
        }

        ApplicationChooser {
            id: appChooser
            onChosen: exec => editor.emitArgs([editor.stripFieldCodes(exec)])
        }
    }

    QQC2.TextField {
        visible: editor.kind === "shell" || editor.kind === "text"
        Layout.fillWidth: true
        placeholderText: editor.kind === "shell" ? "Shell command, for example notify-send hello && sleep 1" : "Workspace name"
        text: editor.first
        onTextEdited: editor.emitArgs(text.length ? [text] : [])
    }

    QQC2.Switch {
        visible: editor.action !== null && editor.action !== undefined && editor.action.followProperty
        text: editor.action && editor.action.id.indexOf("column") >= 0 ? "Follow the column to that workspace" : "Follow the window to that workspace"
        checked: editor.props.focus !== false
        onToggled: editor.edited(editor.args, checked ? {} : { focus: false })
    }
}
