import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.kquickcontrols as KQuickControls
import "../catalog/BindKeys.js" as BindKeys

ColumnLayout {
    id: editor

    property string keyName
    property string modKey
    property string kind: "keyboard"
    signal edited(string keyName)

    readonly property var parts: BindKeys.split(keyName)

    function syncKind() {
        kind = keyName.length ? BindKeys.kindOf(keyName) : "keyboard";
    }

    function toggleModifier(modifier) {
        const current = parts.modifiers.filter(part => part !== modifier);
        if (current.length === parts.modifiers.length) {
            current.push(modifier);
        }
        edited(BindKeys.join(current, parts.key));
    }

    spacing: Kirigami.Units.largeSpacing

    Segmented {
        currentValue: editor.kind
        options: BindKeys.triggerKinds.map(entry => ({ value: entry.id, label: entry.label, icon: entry.icon }))
        onChosen: value => {
            if (value === editor.kind) {
                return;
            }
            editor.kind = value;
            const pointer = BindKeys.pointerTriggers[value];
            editor.edited(pointer ? BindKeys.join(editor.parts.modifiers.length ? editor.parts.modifiers : ["Mod"], pointer[0].key) : "");
        }
    }

    ColumnLayout {
        visible: editor.kind === "keyboard"
        spacing: Kirigami.Units.smallSpacing

        QQC2.Label {
            text: "Click the button, then press the keys you want to use."
            opacity: 0.7
        }

        KQuickControls.KeySequenceItem {
            id: recorder
            showClearButton: false
            multiKeyShortcutsAllowed: false
            modifierlessAllowed: true
            checkForConflictsAgainst: KQuickControls.ShortcutType.None
            onKeySequenceModified: {
                const name = kcm.keyName(keySequence);
                if (name.length > 0) {
                    editor.edited(name);
                }
            }
        }
    }

    ColumnLayout {
        visible: editor.kind !== "keyboard"
        spacing: Kirigami.Units.largeSpacing

        RowLayout {
            spacing: Kirigami.Units.smallSpacing

            QQC2.Label {
                text: "Hold:"
            }

            Repeater {
                model: ["Mod", "Ctrl", "Shift", "Alt", "Super"]

                QQC2.Button {
                    required property string modelData
                    text: modelData === "Mod" ? "Mod" : modelData === "Super" ? "Meta" : modelData
                    checkable: true
                    checked: editor.parts.modifiers.some(part => part.toLowerCase() === modelData.toLowerCase())
                    onClicked: editor.toggleModifier(editor.parts.modifiers.find(part => part.toLowerCase() === modelData.toLowerCase()) || modelData)
                }
            }
        }

        GridLayout {
            visible: editor.kind === "wheel" || editor.kind === "touchpad"
            columns: 3
            rowSpacing: Kirigami.Units.smallSpacing
            columnSpacing: Kirigami.Units.smallSpacing

            Repeater {
                model: [null, 0, null, 1, "center", 2, null, 3, null]

                Item {
                    id: cell
                    required property var modelData
                    readonly property var trigger: typeof modelData === "number" && BindKeys.pointerTriggers[editor.kind] ? BindKeys.pointerTriggers[editor.kind][modelData] : null
                    implicitWidth: Kirigami.Units.gridUnit * 5
                    implicitHeight: Kirigami.Units.gridUnit * 2.2

                    Kirigami.Icon {
                        anchors.centerIn: parent
                        visible: cell.modelData === "center"
                        source: editor.kind === "wheel" ? "input-mouse" : "input-touchpad"
                        width: Kirigami.Units.iconSizes.medium
                        height: width
                    }

                    QQC2.Button {
                        anchors.fill: parent
                        visible: cell.trigger !== null
                        text: cell.trigger ? cell.trigger.label : ""
                        icon.name: cell.trigger ? cell.trigger.icon : ""
                        checkable: true
                        checked: cell.trigger !== null && editor.parts.key.toLowerCase() === cell.trigger.key.toLowerCase()
                        onClicked: editor.edited(BindKeys.join(editor.parts.modifiers, cell.trigger.key))
                    }
                }
            }
        }

        Flow {
            visible: editor.kind === "mouse"
            Layout.fillWidth: true
            spacing: Kirigami.Units.smallSpacing

            Repeater {
                model: BindKeys.pointerTriggers.mouse

                QQC2.Button {
                    required property var modelData
                    text: modelData.label
                    icon.name: modelData.icon
                    checkable: true
                    checked: editor.parts.key.toLowerCase() === modelData.key.toLowerCase()
                    onClicked: editor.edited(BindKeys.join(editor.parts.modifiers, modelData.key))
                }
            }
        }
    }
}
