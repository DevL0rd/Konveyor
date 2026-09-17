import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.konveyor.settings

QQC2.ComboBox {
    id: picker

    property string value
    property bool allowNone: false
    property string noneLabel: "Any monitor"
    signal picked(string name)

    readonly property var entries: {
        const list = allowNone ? [{ name: "", label: noneLabel }] : [];
        const seen = new Set();
        for (const output of SettingsStore.live.outputs) {
            const logical = output.logical || {};
            seen.add(output.name);
            list.push({ name: output.name, label: output.name + " · " + (output.description || "") + " · " + logical.width + "×" + logical.height });
        }
        if (SettingsStore.revision >= 0) {
            for (const entry of SettingsStore.children("", "output")) {
                const name = String((entry.node.args || [])[0] || "");
                if (name.length && !seen.has(name)) {
                    seen.add(name);
                    list.push({ name: name, label: name + " · not connected" });
                }
            }
        }
        if (value.length && !seen.has(value)) {
            list.push({ name: value, label: value + " · not connected" });
        }
        return list;
    }

    model: entries
    textRole: "label"
    valueRole: "name"
    editable: true
    currentIndex: Math.max(0, entries.findIndex(entry => entry.name === value))
    implicitWidth: Kirigami.Units.gridUnit * 16
    onActivated: index => picked(entries[index].name)
    onAccepted: {
        const typed = editText.split(" · ")[0].trim();
        const match = entries.find(entry => entry.label === editText || entry.name === typed);
        picked(match ? match.name : typed);
    }

    QQC2.ToolTip.text: "Pick a connected monitor, or type the name of one that isn't plugged in"
    QQC2.ToolTip.visible: hovered
    QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
}
