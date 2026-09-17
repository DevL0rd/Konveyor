import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import "../components/monitors/MonitorSummary.js" as MonitorSummary
import org.kde.konveyor.components

SceneAnimation {
    id: glance

    property var node: ({})
    readonly property bool portrait: MonitorSummary.isPortrait(node)
    readonly property real columnWidth: MonitorSummary.columnWidth(node) || (portrait ? 1 : 0.4)
    readonly property var rows: MonitorSummary.layoutChild(node, "max-rows-per-column")
    readonly property var placement: MonitorSummary.layoutChild(node, "new-window-placement")
    readonly property int stack: placement && placement.args[0] === "stack" ? Math.max(1, Number(rows ? rows.args[0] : 3)) : 1

    Layout.preferredWidth: Kirigami.Units.gridUnit * (portrait ? 3 : 5.5)
    Layout.preferredHeight: Kirigami.Units.gridUnit * (portrait ? 5 : 3.2)
    aspect: portrait ? 10 / 16 : 16 / 10
    animated: false
    scene: {
        const width = Math.min(0.93, glance.columnWidth - 0.035);
        const count = Math.max(1, Math.ceil(1 / Math.max(width + 0.035, 0.1)));
        const list = [];
        for (let i = 0; i < count; ++i) {
            list.push({ keys: Array.from({ length: glance.stack }, (_, row) => i + "-" + row), w: width, focus: i === 0 ? "0-0" : "" });
        }
        return build([columns(list)]);
    }
}
