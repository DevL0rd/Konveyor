import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import ".."

ColumnLayout {
    id: section

    property string rulePath
    readonly property var workspaceNames: kcm.revision >= 0 ? kcm.children("", "workspace").map(entry => String((entry.node.args || [])[0] || "")).filter(name => name.length) : []

    function argOf(name) {
        const node = kcm.revision >= 0 ? kcm.node(rulePath + "/" + name) : ({});
        return node.args && node.args.length ? node.args[0] : undefined;
    }

    spacing: 0

    SettingRow {
        label: "Tiling"
        description: "Leave it to KDE for popups, splash screens and helper windows that shouldn't join the row"
        iconName: "view-split-left-right"

        Segmented {
            readonly property var manage: section.argOf("manage")
            currentValue: manage === undefined ? "default" : (manage ? "tile" : "kde")
            options: [
                { value: "default", label: "Default" },
                { value: "tile", label: "Tile it" },
                { value: "kde", label: "Leave it to KDE" }
            ]
            onChosen: value => value === "default" ? kcm.remove(section.rulePath + "/manage") : kcm.setValue(section.rulePath + "/manage", [value === "tile"])
        }
    }

    RuleTriStateRow {
        path: section.rulePath + "/open-floating"
        label: "Open floating"
        description: "Float above the row instead of becoming a column"
        iconName: "window-keep-above"
    }

    RuleTriStateRow {
        path: section.rulePath + "/open-maximized"
        label: "Open maximized"
        description: "Column fills the width of the screen"
        iconName: "window-maximize"
    }

    RuleTriStateRow {
        path: section.rulePath + "/open-maximized-to-edges"
        label: "Open maximized to the edges"
        description: "Covers gaps and panels' free space too"
        iconName: "zoom-fit-best"
    }

    RuleTriStateRow {
        path: section.rulePath + "/open-fullscreen"
        label: "Open fullscreen"
        iconName: "view-fullscreen"
    }

    RuleTriStateRow {
        path: section.rulePath + "/open-focused"
        label: "Take focus when it opens"
        iconName: "window-new"
    }

    SettingRow {
        label: "Workspace"
        description: section.workspaceNames.length ? "Open on a named workspace" : "Name a workspace on the Workspaces page to pick it here"
        iconName: "virtual-desktops"

        QQC2.ComboBox {
            readonly property var current: section.argOf("open-on-workspace")
            readonly property var options: [{ value: "", label: "Default" }]
                .concat(section.workspaceNames.map(name => ({ value: name, label: name })))
                .concat(current !== undefined && !section.workspaceNames.includes(current) ? [{ value: current, label: current + " (not defined)" }] : [])
            model: options
            textRole: "label"
            valueRole: "value"
            enabled: options.length > 1
            currentIndex: current === undefined ? 0 : Math.max(0, options.findIndex(entry => entry.value === current))
            onActivated: currentValue === "" ? kcm.remove(section.rulePath + "/open-on-workspace") : kcm.setValue(section.rulePath + "/open-on-workspace", [currentValue])
        }
    }

    SettingRow {
        label: "Monitor"
        description: "Open on a specific monitor"
        iconName: "video-display"

        OutputPicker {
            allowNone: true
            noneLabel: "Default"
            value: section.argOf("open-on-output") || ""
            onPicked: name => name.length ? kcm.setValue(section.rulePath + "/open-on-output", [name]) : kcm.remove(section.rulePath + "/open-on-output")
        }
    }

    RuleSizeRow {
        path: section.rulePath + "/default-column-width"
        nodeName: "default-column-width"
        label: "Width"
        description: "How wide its column starts"
        iconName: "distribute-horizontal-x"
    }

    RuleSizeRow {
        path: section.rulePath + "/default-window-height"
        nodeName: "default-window-height"
        presetsKey: "preset-window-heights"
        label: "Height"
        description: "How tall it starts when it shares a column"
        iconName: "distribute-vertical-y"
    }

    SettingRow {
        label: "Column style"
        iconName: "tab-new"

        Segmented {
            readonly property var display: section.argOf("default-column-display")
            currentValue: display === undefined ? "default" : display
            options: [
                { value: "default", label: "Default" },
                { value: "normal", label: "Stacked" },
                { value: "tabbed", label: "Tabs" }
            ]
            onChosen: value => value === "default" ? kcm.remove(section.rulePath + "/default-column-display") : kcm.setValue(section.rulePath + "/default-column-display", [value])
        }
    }

    SettingRow {
        label: "Other windows from this app"
        description: "Where it opens when the app already has a window on the workspace"
        iconName: "window-duplicate"

        Segmented {
            readonly property var mode: section.argOf("group-app-windows")
            currentValue: mode === undefined ? "default" : mode
            options: [
                { value: "default", label: "Default" },
                { value: "off", label: "Don't group" },
                { value: "beside", label: "Beside" },
                { value: "stack", label: "Stack" }
            ]
            onChosen: value => value === "default" ? kcm.remove(section.rulePath + "/group-app-windows") : kcm.setValue(section.rulePath + "/group-app-windows", [value])
        }
    }

    RuleTriStateRow {
        path: section.rulePath + "/float-child-windows"
        label: "Float its extra windows"
        description: "Its first window tiles; any more it opens while one is already open float above the row"
        iconName: "window-keep-above"
    }

    StackLimitRow {
        path: section.rulePath + "/max-rows-per-column"
        label: "Most stacked per column"
        description: "How many of its windows share a column before another column opens"
        iconName: "view-split-top-bottom"
    }

    ColumnPositionRow {
        path: section.rulePath + "/column-position"
        label: "Pin its column"
        description: "Pinned columns stay at one end of the row; new columns open on the other side of them"
        iconName: "pin"
    }

    FloatingPositionRow {
        path: section.rulePath + "/default-floating-position"
        label: "Floating position"
        description: "Where it appears when it floats, measured from a corner or edge of the screen"
        iconName: "transform-move"
    }
}
