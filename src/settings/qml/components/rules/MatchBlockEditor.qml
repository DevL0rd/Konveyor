import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import ".."
import "../../catalog/Kdl.js" as Kdl
import org.kde.konveyor.settings

ColumnLayout {
    id: editor

    property string path
    property var node: ({ props: {} })
    property string heading
    signal removeRequested

    readonly property var props: node && node.props ? node.props : ({})
    readonly property var appParsed: props["app-id"] === undefined ? { mode: "any", text: "" } : Kdl.parseTextMatch(props["app-id"])
    property string appMode: appParsed.mode === "any" ? "any" : (appParsed.mode === "is" ? "app" : "pattern")
    readonly property var profileNames: SettingsStore.revision >= 0 ? SettingsStore.children("", "monitor-profile").map(entry => String((entry.node.args || [])[0] || "")).filter(name => name.length) : []
    readonly property var states: [
        { key: "is-focused", label: "Focused" },
        { key: "is-active", label: "Active on its workspace" },
        { key: "is-active-in-column", label: "Active in its column" },
        { key: "is-floating", label: "Floating" },
        { key: "is-urgent", label: "Urgent" },
        { key: "at-startup", label: "Opened at startup" }
    ]

    function write(changes) {
        const next = Object.assign({}, props, changes);
        for (const key of Object.keys(next)) {
            if (next[key] === null || next[key] === undefined) {
                delete next[key];
            }
        }
        SettingsStore.setNode(path, { name: node.name || "match", args: [], props: next });
    }

    onAppParsedChanged: appMode = appParsed.mode === "any" ? "any" : (appParsed.mode === "is" ? "app" : "pattern")

    spacing: 0

    RowLayout {
        Layout.fillWidth: true
        Layout.leftMargin: Kirigami.Units.largeSpacing
        Layout.rightMargin: Kirigami.Units.smallSpacing
        Layout.topMargin: Kirigami.Units.smallSpacing

        QQC2.Label {
            text: editor.heading
            font.bold: true
            Layout.fillWidth: true
        }

        QQC2.ToolButton {
            icon.name: "edit-delete-remove"
            text: "Remove"
            onClicked: editor.removeRequested()
        }
    }

    SettingRow {
        label: "App"
        iconName: "application-x-executable"
        wideControl: true

        RowLayout {
            width: parent.width
            spacing: Kirigami.Units.smallSpacing

            Segmented {
                currentValue: editor.appMode
                options: [
                    { value: "any", label: "Any app" },
                    { value: "app", label: "This app" },
                    { value: "pattern", label: "Pattern" }
                ]
                onChosen: value => {
                    editor.appMode = value;
                    if (value === "any") {
                        editor.write({ "app-id": null });
                    }
                }
            }

            AppPicker {
                visible: editor.appMode === "app"
                appId: editor.appParsed.mode === "is" ? editor.appParsed.text : ""
                onPicked: appId => editor.write({ "app-id": Kdl.textMatch("is", appId) })
            }

            QQC2.TextField {
                visible: editor.appMode === "pattern"
                text: editor.props["app-id"] || ""
                placeholderText: "Regular expression, e.g. ^org\\.kde\\."
                font.family: "monospace"
                Layout.fillWidth: true
                onEditingFinished: editor.write({ "app-id": text.length ? text : null })
            }

            Item {
                visible: editor.appMode !== "pattern"
                Layout.fillWidth: true
            }
        }
    }

    SettingRow {
        label: "Title"
        iconName: "texture"
        wideControl: true

        TextMatchField {
            width: parent.width
            regex: editor.props.title
            onEdited: regex => editor.write({ title: regex })
        }
    }

    SettingRow {
        label: "Monitor profile"
        description: "Only while the window is on a monitor using this profile"
        iconName: "video-display"

        QQC2.ComboBox {
            readonly property string customValue: "__pattern__"
            readonly property var parsed: editor.props["monitor-profile"] === undefined ? null : Kdl.parseTextMatch(editor.props["monitor-profile"])
            readonly property bool isCustom: parsed !== null && !(parsed.mode === "is" && editor.profileNames.includes(parsed.text))
            readonly property var options: [{ value: "", label: "Any monitor" }]
                .concat(editor.profileNames.map(name => ({ value: name, label: name })))
                .concat(isCustom ? [{ value: customValue, label: "Custom pattern" }] : [])
            model: options
            textRole: "label"
            valueRole: "value"
            enabled: editor.profileNames.length > 0 || parsed !== null
            currentIndex: !parsed ? 0 : Math.max(0, options.findIndex(entry => isCustom ? entry.value === customValue : entry.value === parsed.text))
            onActivated: {
                if (currentValue === "") {
                    editor.write({ "monitor-profile": null });
                } else if (currentValue !== customValue) {
                    editor.write({ "monitor-profile": Kdl.textMatch("is", currentValue) });
                }
            }
        }
    }

    SettingRow {
        label: "Window state"
        description: "Conditions on state are checked again whenever the window changes"
        iconName: "view-filter"
        wideControl: true

        GridLayout {
            width: parent.width
            columns: width > Kirigami.Units.gridUnit * 36 ? 2 : 1
            columnSpacing: Kirigami.Units.gridUnit
            rowSpacing: Kirigami.Units.smallSpacing

            Repeater {
                model: editor.states

                RowLayout {
                    id: stateRow
                    required property var modelData
                    Layout.fillWidth: true

                    QQC2.Label {
                        text: stateRow.modelData.label
                        Layout.fillWidth: true
                    }

                    TriState {
                        unsetLabel: "Any"
                        triState: editor.props[stateRow.modelData.key] === undefined ? null : editor.props[stateRow.modelData.key]
                        onStateChosen: state => {
                            const change = {};
                            change[stateRow.modelData.key] = state;
                            editor.write(change);
                        }
                    }
                }
            }
        }
    }
}
