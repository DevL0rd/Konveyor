import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import "../components"
import "../previews"
import "../catalog/Actions.js" as Actions
import "../catalog/BindKeys.js" as BindKeys
import org.kde.konveyor.settings

SettingsPage {
    id: page

    title: "Shortcuts"

    readonly property string modKey: SettingsStore.values["input/mod-key"] || "Super"
    readonly property var binds: SettingsStore.revision >= 0 ? (SettingsStore.node("binds").children || []) : []
    readonly property var conflicts: BindKeys.conflicts(binds.map(bind => bind.name), modKey)
    property string query
    property string categoryFilter: "all"

    function actionOf(bind) {
        return bind.children && bind.children.length ? bind.children[0] : null;
    }

    function categoryOf(bind) {
        const node = actionOf(bind);
        const action = node ? Actions.byId(node.name) : null;
        return action ? action.category : "apps";
    }

    function matches(bind) {
        if (categoryFilter !== "all" && categoryOf(bind) !== categoryFilter) {
            return false;
        }
        const words = query.toLowerCase().split(/\s+/).filter(Boolean);
        const title = bind.props && typeof bind.props["hotkey-overlay-title"] === "string" ? bind.props["hotkey-overlay-title"] : "";
        const haystack = (Actions.describe(actionOf(bind)) + " " + bind.name + " " + title).toLowerCase();
        return words.every(word => haystack.includes(word));
    }

    function countIn(category) {
        return binds.filter(bind => category === "all" || categoryOf(bind) === category).length;
    }

    CardHeader {
        title: "Mod key"
    }

    Card {
        SettingRow {
            label: "Key that “Mod” stands for"
            description: "Shortcuts written with Mod use this key, so switching it here moves all of them at once."
            iconName: "input-keyboard"
            resetPaths: ["input/mod-key"]
            wideControl: true

            ColumnLayout {
                width: parent.width
                spacing: Kirigami.Units.largeSpacing

                ModKeyPreview {
                    Layout.fillWidth: true
                    Layout.preferredHeight: Kirigami.Units.gridUnit * 7
                    modKey: page.modKey
                    shortcutCount: page.binds.length
                }

                ModKeyPicker {
                    Layout.alignment: Qt.AlignHCenter
                    value: page.modKey
                    onPicked: name => SettingsStore.setValue("input/mod-key", [name])
                }
            }
        }
    }

    ColumnLayout {
        Layout.fillWidth: true
        Layout.maximumWidth: Kirigami.Units.gridUnit * 46
        Layout.alignment: Qt.AlignHCenter
        Layout.topMargin: Kirigami.Units.largeSpacing * 2
        Layout.leftMargin: Kirigami.Units.largeSpacing
        Layout.rightMargin: Kirigami.Units.largeSpacing
        spacing: Kirigami.Units.largeSpacing

        RowLayout {
            Layout.fillWidth: true
            spacing: Kirigami.Units.smallSpacing

            Kirigami.SearchField {
                Layout.fillWidth: true
                placeholderText: "Search shortcuts by action or key…"
                onTextChanged: page.query = text
            }

            QQC2.Button {
                icon.name: "list-add"
                text: "Add shortcut"
                onClicked: editor.openFor(null)
            }

            QQC2.ToolButton {
                icon.name: "edit-reset"
                display: QQC2.AbstractButton.IconOnly
                text: "Restore default shortcuts"
                enabled: SettingsStore.revision >= 0 && !SettingsStore.isDefault("binds")
                onClicked: restoreDialog.open()
                QQC2.ToolTip.text: text
                QQC2.ToolTip.visible: hovered
                QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
            }
        }

        Flow {
            Layout.fillWidth: true
            spacing: Kirigami.Units.smallSpacing

            Repeater {
                model: [{ id: "all", label: "All", icon: "view-list-icons" }].concat(Actions.categories)

                QQC2.Button {
                    required property var modelData
                    readonly property int count: page.countIn(modelData.id)
                    visible: count > 0 || modelData.id === "all"
                    text: modelData.label + "  " + count
                    icon.name: modelData.icon
                    checkable: true
                    checked: page.categoryFilter === modelData.id
                    flat: !checked
                    onClicked: page.categoryFilter = modelData.id
                }
            }
        }
    }

    Repeater {
        model: Actions.categories

        ColumnLayout {
            id: group

            required property var modelData
            readonly property var entries: page.binds.filter(bind => page.categoryOf(bind) === modelData.id && page.matches(bind))

            Layout.fillWidth: true
            visible: entries.length > 0
            spacing: 0

            CardHeader {
                title: group.modelData.label + " · " + group.entries.length
            }

            Card {
                Repeater {
                    model: group.entries

                    ShortcutRow {
                        required property var modelData
                        bind: modelData
                        modKey: page.modKey
                        conflicting: page.conflicts[modelData.name] === true
                        onEditRequested: editor.openFor(modelData)
                        onRemoveRequested: SettingsStore.remove("binds/" + modelData.name)
                    }
                }
            }
        }
    }

    Kirigami.PlaceholderMessage {
        Layout.fillWidth: true
        Layout.margins: Kirigami.Units.gridUnit * 2
        visible: page.binds.length > 0 && !page.binds.some(bind => page.matches(bind))
        icon.name: "search"
        text: "No shortcuts match"
        explanation: "Try another word, or pick a different category."
    }

    Kirigami.PlaceholderMessage {
        Layout.fillWidth: true
        Layout.margins: Kirigami.Units.gridUnit * 2
        visible: page.binds.length === 0
        icon.name: "preferences-desktop-keyboard-shortcut"
        text: "No shortcuts yet"
        explanation: "Add one, or restore the default set."
    }

    BindEditorDialog {
        id: editor
        modKey: page.modKey
        binds: page.binds
    }

    Kirigami.PromptDialog {
        id: restoreDialog
        parent: QQC2.Overlay.overlay
        title: "Restore default shortcuts?"
        subtitle: "Every shortcut goes back to the Konveyor defaults. Shortcuts you added are removed. Nothing is saved until you press Apply."
        standardButtons: Kirigami.Dialog.Ok | Kirigami.Dialog.Cancel
        onAccepted: SettingsStore.resetToDefault("binds")
    }
}
