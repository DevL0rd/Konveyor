import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import "../catalog/Actions.js" as Actions
import "../catalog/BindKeys.js" as BindKeys
import org.kde.konveyor.components
import org.kde.konveyor.settings

Kirigami.Dialog {
    id: dialog

    property string modKey
    property var binds: []
    property string originalKey
    property string keyName
    property var actionNode: null
    property bool repeat: true
    property int cooldown: 0
    property string cheatsheetTitle
    property bool hideFromCheatsheet: false

    readonly property var action: actionNode ? Actions.byId(actionNode.name) : null
    readonly property var clash: keyName.length ? binds.find(bind => bind.name !== originalKey
        && BindKeys.signature(bind.name, modKey) === BindKeys.signature(keyName, modKey)) || null : null
    readonly property string problem: {
        if (!keyName.length || !BindKeys.split(keyName).key.length) {
            return "Choose what triggers the shortcut.";
        }
        if (!action) {
            return "Choose what the shortcut does.";
        }
        const args = actionNode.args || [];
        if (action.argument !== "none" && (args.length === 0 || String(args[0]).trim().length === 0)) {
            return "This action needs a value.";
        }
        return "";
    }

    function openFor(bind) {
        originalKey = bind ? bind.name : "";
        keyName = originalKey;
        const node = bind && bind.children && bind.children.length ? bind.children[0] : null;
        actionNode = node ? { name: node.name, args: node.args || [], props: node.props || {} } : null;
        const props = bind ? bind.props || {} : {};
        repeat = props.repeat !== false;
        cooldown = props["cooldown-ms"] || 0;
        hideFromCheatsheet = props.hasOwnProperty("hotkey-overlay-title") && props["hotkey-overlay-title"] === null;
        cheatsheetTitle = typeof props["hotkey-overlay-title"] === "string" ? props["hotkey-overlay-title"] : "";
        options.expanded = bind !== null && (!repeat || cooldown > 0 || hideFromCheatsheet || cheatsheetTitle.length > 0);
        trigger.syncKind();
        picker.expanded = actionNode === null;
        open();
    }

    function save() {
        const props = {};
        if (!repeat) {
            props.repeat = false;
        }
        if (cooldown > 0) {
            props["cooldown-ms"] = cooldown;
        }
        if (hideFromCheatsheet) {
            props["hotkey-overlay-title"] = null;
        } else if (cheatsheetTitle.trim().length) {
            props["hotkey-overlay-title"] = cheatsheetTitle.trim();
        }
        const args = (actionNode.args || []).map(value => typeof value === "string" ? value.trim() : value);
        const node = { name: keyName, args: [], props: props, children: [{ name: actionNode.name, args: args, props: actionNode.props || {} }] };
        if (originalKey.length && originalKey !== keyName) {
            SettingsStore.remove("binds/" + originalKey);
        }
        if (clash) {
            SettingsStore.remove("binds/" + clash.name);
        }
        if (SettingsStore.setNode("binds/" + keyName, node)) {
            close();
        }
    }

    parent: QQC2.Overlay.overlay
    title: originalKey.length ? "Edit shortcut" : "Add shortcut"
    preferredWidth: Kirigami.Units.gridUnit * 38
    maximumHeight: Kirigami.Units.gridUnit * 40
    padding: Kirigami.Units.largeSpacing * 2

    customFooterActions: [
        Kirigami.Action {
            text: dialog.clash ? "Replace and save" : "Save"
            icon.name: "dialog-ok-apply"
            enabled: dialog.problem.length === 0
            onTriggered: dialog.save()
        },
        Kirigami.Action {
            text: "Cancel"
            icon.name: "dialog-cancel"
            onTriggered: dialog.close()
        }
    ]

    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: summary.implicitHeight + Kirigami.Units.largeSpacing * 3
            radius: Kirigami.Units.cornerRadius * 2
            color: Qt.alpha(Kirigami.Theme.highlightColor, 0.1)

            ColumnLayout {
                id: summary
                anchors.centerIn: parent
                width: parent.width - Kirigami.Units.largeSpacing * 2
                spacing: Kirigami.Units.smallSpacing

                KeyCaps {
                    Layout.alignment: Qt.AlignHCenter
                    visible: dialog.keyName.length > 0
                    keyName: dialog.keyName
                    modKey: dialog.modKey
                    scale: 1.3
                }

                QQC2.Label {
                    Layout.fillWidth: true
                    Layout.topMargin: Kirigami.Units.smallSpacing
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                    text: dialog.problem.length ? dialog.problem : Actions.describe(dialog.actionNode)
                    opacity: dialog.problem.length ? 0.7 : 1
                }
            }
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: dialog.clash !== null
            type: Kirigami.MessageType.Warning
            text: dialog.clash ? "This trigger already runs “" + Actions.describe(dialog.clash.children ? dialog.clash.children[0] : null) + "”. Saving replaces it." : ""
        }

        Kirigami.Heading {
            level: 4
            text: "Trigger"
        }

        TriggerEditor {
            id: trigger
            Layout.fillWidth: true
            keyName: dialog.keyName
            modKey: dialog.modKey
            onEdited: name => dialog.keyName = name
        }

        Kirigami.Separator {
            Layout.fillWidth: true
        }

        Kirigami.Heading {
            level: 4
            text: "Action"
        }

        ActionPicker {
            id: picker
            Layout.fillWidth: true
            current: dialog.actionNode ? dialog.actionNode.name : ""
            onPicked: actionId => dialog.actionNode = { name: actionId, args: [], props: {} }
        }

        ActionArguments {
            Layout.fillWidth: true
            action: dialog.action
            args: dialog.actionNode ? dialog.actionNode.args || [] : []
            props: dialog.actionNode ? dialog.actionNode.props || {} : {}
            onEdited: (args, props) => dialog.actionNode = { name: dialog.actionNode.name, args: args, props: props }
        }

        Kirigami.Separator {
            Layout.fillWidth: true
        }

        BindOptions {
            id: options
            Layout.fillWidth: true
            repeat: dialog.repeat
            cooldown: dialog.cooldown
            cheatsheetTitle: dialog.cheatsheetTitle
            hideFromCheatsheet: dialog.hideFromCheatsheet
            placeholderTitle: dialog.action ? dialog.action.label : ""
            onRepeatEdited: value => dialog.repeat = value
            onCooldownEdited: value => dialog.cooldown = value
            onTitleEdited: value => dialog.cheatsheetTitle = value
            onHiddenEdited: value => dialog.hideFromCheatsheet = value
        }
    }
}
