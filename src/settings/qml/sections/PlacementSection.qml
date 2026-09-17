import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import "../components"
import "../previews"
import "LayoutKeys.js" as LayoutKeys
import org.kde.konveyor.components
import org.kde.konveyor.settings

ColumnLayout {
    id: root

    property string scopePath: "layout"
    property bool overrideMode: false
    property var excluded: []
    readonly property var values: SettingsStore.revision >= 0 ? SettingsStore.scope(scopePath) : ({})

    spacing: 0

    CardHeader {
        title: "New windows"
    }

    Card {
        ScopedRow {
            scope: root
            key: "new-column-position"
            inheritedSummary: root.values["new-column-position"] === "left" ? "left of focus" : "right of focus"
            wideControl: true
            label: "Where new windows open"
            description: "A new window gets its own column next to the one you're using. Pinned columns always keep their place."
            iconName: "window-new"

            ChoiceCards {
                width: parent.width
                currentValue: root.values["new-column-position"]
                cardHeight: Kirigami.Units.gridUnit * 7
                onChosen: value => LayoutKeys.write(SettingsStore, root.scopePath, "new-column-position", value)
                options: [
                    { value: "left", title: "Left of focus", description: "The column you're on moves right", preview: leftDiagram },
                    { value: "right", title: "Right of focus", description: "The new column appears after it", preview: rightDiagram }
                ]
            }
        }

        ScopedRow {
            scope: root
            key: "new-window-placement"
            inheritedSummary: root.values["new-window-placement"] === "stack" ? "stack in the focused column" : "own column"
            wideControl: true
            label: "Give each new window"
            description: "Stacking fills the focused column up to the most stacked windows per column, then opens a new column. Portrait monitors stack two by default. Window rules can choose per app."
            iconName: "view-split-left-right"

            ChoiceCards {
                width: parent.width
                currentValue: root.values["new-window-placement"]
                cardHeight: Kirigami.Units.gridUnit * 7
                onChosen: value => LayoutKeys.write(SettingsStore, root.scopePath, "new-window-placement", value)
                options: [
                    { value: "column", title: "Its own column", description: "Every window gets a column", preview: placeColumnDiagram },
                    { value: "stack", title: "A row in the focused column", description: "Columns fill up before a new one opens", preview: placeStackDiagram }
                ]
            }
        }

        ScopedRow {
            scope: root
            key: "group-app-windows"
            inheritedSummary: ({ off: "off", beside: "beside", stack: "stacked" })[root.values["group-app-windows"]] || ""
            wideControl: true
            label: "Windows from an app that's already open"
            description: "Keep an app's windows together instead of opening them beside whatever you're focused on. Window rules can choose differently per app."
            iconName: "window-duplicate"

            ChoiceCards {
                width: parent.width
                currentValue: root.values["group-app-windows"]
                cardHeight: Kirigami.Units.gridUnit * 7
                onChosen: value => LayoutKeys.write(SettingsStore, root.scopePath, "group-app-windows", value)
                options: [
                    { value: "off", title: "Don't group", description: "Opens beside the focused column", preview: groupOffDiagram },
                    { value: "beside", title: "Beside its app", description: "New column right of the app's windows", preview: groupBesideDiagram },
                    { value: "stack", title: "Stack in its column", description: "New row under the app's windows", preview: groupStackDiagram }
                ]
            }
        }

        ScopedRow {
            scope: root
            key: "max-rows-per-column"
            label: "Most stacked windows per column"
            description: "When a column is full, a new column opens and the windows are spread evenly, with any extra on the left."
            iconName: "view-split-top-bottom"

            ValueSlider {
                value: root.values["max-rows-per-column"] || 3
                from: 1
                to: 8
                onEdited: value => LayoutKeys.write(SettingsStore, root.scopePath, "max-rows-per-column", Math.round(value))
            }
        }

        ScopedRow {
            scope: root
            key: "float-child-windows"
            visible: !root.overrideMode
            label: "Float an app's extra windows"
            description: "The first window of an app tiles; any more it opens (friends lists, chats, settings) float above the row. Window rules can turn this on or off per app."
            iconName: "window-keep-above"

            ScopedSwitch {
                isOn: root.values["float-child-windows"] === true
                onSwitched: on => LayoutKeys.writeFlag(SettingsStore, root.scopePath, root.overrideMode, "float-child-windows", on)
            }
        }

        ScopedRow {
            scope: root
            key: "default-column-width"
            label: "Starting width"
            description: "How wide a new column is. Window rules can override this per app."
            iconName: "zoom-fit-width"

            SizePicker {
                size: root.values["default-column-width"]
                presets: root.values["preset-column-widths"] || []
                onEdited: size => LayoutKeys.write(SettingsStore, root.scopePath, "default-column-width", size)
            }
        }

        ScopedRow {
            scope: root
            key: "default-column-display"
            inheritedSummary: root.values["default-column-display"] === "tabbed" ? "tabbed" : "stacked"
            wideControl: true
            label: "How a column shows several windows"
            description: "Stacked windows share the column height; tabbed windows fill it and you flip between them."
            iconName: "view-list-details"

            ChoiceCards {
                width: parent.width
                currentValue: root.values["default-column-display"]
                cardHeight: Kirigami.Units.gridUnit * 6.5
                onChosen: value => LayoutKeys.write(SettingsStore, root.scopePath, "default-column-display", value)
                options: [
                    { value: "normal", title: "Stacked", preview: stackedDiagram },
                    { value: "tabbed", title: "Tabbed", preview: tabbedDiagram }
                ]
            }
        }
    }

    CardHeader {
        title: "Scrolling and centering"
    }

    Card {
        ScopedRow {
            scope: root
            key: "center-focused-column"
            inheritedSummary: ({ never: "never", always: "always", "on-overflow": "when it doesn't fit" })[root.values["center-focused-column"]] || ""
            wideControl: true
            label: "Keep the focused column centered"
            description: "Controls where the row scrolls to when you move focus."
            iconName: "align-horizontal-center"

            ChoiceCards {
                width: parent.width
                currentValue: root.values["center-focused-column"]
                cardHeight: Kirigami.Units.gridUnit * 7
                onChosen: value => LayoutKeys.write(SettingsStore, root.scopePath, "center-focused-column", value)
                options: [
                    { value: "never", title: "Never", description: "Scroll just enough to show it", preview: neverDiagram },
                    { value: "always", title: "Always", description: "Focused column sits in the middle", preview: alwaysDiagram },
                    { value: "on-overflow", title: "When it doesn't fit", description: "Center only if it and a neighbor don't fit", preview: overflowDiagram }
                ]
            }
        }

        ScopedRow {
            scope: root
            key: "always-center-single-column"
            label: "Center a lone window"
            description: "When a workspace has only one column, place it in the middle of the screen."
            iconName: "format-justify-center"

            ScopedSwitch {
                isOn: root.values["always-center-single-column"] === true
                onSwitched: on => LayoutKeys.writeFlag(SettingsStore, root.scopePath, root.overrideMode, "always-center-single-column", on)
            }
        }

        ScopedRow {
            scope: root
            key: "remember-window-sizes"
            visible: !root.overrideMode
            label: "Remember window sizes"
            description: "Reopen each app at the width it had last time, and floating windows at their last size."
            iconName: "transform-scale"

            ScopedSwitch {
                isOn: root.values["remember-window-sizes"] === true
                onSwitched: on => LayoutKeys.writeFlag(SettingsStore, root.scopePath, root.overrideMode, "remember-window-sizes", on)
            }
        }

        ScopedRow {
            scope: root
            key: "remember-window-positions"
            visible: !root.overrideMode
            label: "Remember floating window positions"
            description: "Reopen floating windows where they were last placed."
            iconName: "transform-move"

            ScopedSwitch {
                isOn: root.values["remember-window-positions"] === true
                onSwitched: on => LayoutKeys.writeFlag(SettingsStore, root.scopePath, root.overrideMode, "remember-window-positions", on)
            }
        }
    }

    Component {
        id: leftDiagram
        NewColumnDiagram {
            side: "left"
            running: parent && parent.selected === true
        }
    }

    Component {
        id: rightDiagram
        NewColumnDiagram {
            side: "right"
            running: parent && parent.selected === true
        }
    }

    Component {
        id: placeColumnDiagram
        PlacementDiagram {
            stack: false
            rows: Math.max(1, root.values["max-rows-per-column"] || 2)
            running: parent && parent.selected === true
        }
    }

    Component {
        id: placeStackDiagram
        PlacementDiagram {
            stack: true
            rows: Math.max(1, root.values["max-rows-per-column"] || 2)
            running: parent && parent.selected === true
        }
    }

    Component {
        id: groupOffDiagram
        ChoiceDiagram {
            kind: "group-off"
            running: parent && parent.selected === true
        }
    }

    Component {
        id: groupBesideDiagram
        ChoiceDiagram {
            kind: "group-beside"
            running: parent && parent.selected === true
        }
    }

    Component {
        id: groupStackDiagram
        ChoiceDiagram {
            kind: "group-stack"
            running: parent && parent.selected === true
        }
    }

    Component {
        id: stackedDiagram
        ChoiceDiagram {
            kind: "stacked"
            running: parent && parent.selected === true
        }
    }

    Component {
        id: tabbedDiagram
        ChoiceDiagram {
            kind: "tabbed"
            running: parent && parent.selected === true
        }
    }

    Component {
        id: neverDiagram
        ChoiceDiagram {
            kind: "never"
            running: parent && parent.selected === true
        }
    }

    Component {
        id: alwaysDiagram
        ChoiceDiagram {
            kind: "always"
            running: parent && parent.selected === true
        }
    }

    Component {
        id: overflowDiagram
        ChoiceDiagram {
            kind: "on-overflow"
            running: parent && parent.selected === true
        }
    }
}
