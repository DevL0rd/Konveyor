import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import "../components/monitors/MonitorSummary.js" as MonitorSummary
import org.kde.konveyor.components

Item {
    id: glance

    property var node: ({})
    readonly property bool portrait: MonitorSummary.isPortrait(node)
    readonly property real columnWidth: MonitorSummary.columnWidth(node) || (portrait ? 1 : 0.4)
    readonly property var rows: MonitorSummary.layoutChild(node, "max-rows-per-column")
    readonly property var placement: MonitorSummary.layoutChild(node, "new-window-placement")
    readonly property int stack: placement && placement.args[0] === "stack" ? Math.max(1, Number(rows ? rows.args[0] : 3)) : 1

    Layout.preferredWidth: Kirigami.Units.gridUnit * (portrait ? 3 : 5.5)
    Layout.preferredHeight: Kirigami.Units.gridUnit * (portrait ? 5 : 3.2)

    MiniColumns {
        anchors.fill: parent
        active: false
        columns: {
            const width = Math.min(1, glance.columnWidth) - (glance.columnWidth >= 1 ? 0.02 : 0);
            const count = Math.max(1, Math.ceil(1 / Math.max(width, 0.1)));
            const result = [];
            for (let i = 0; i < count; ++i) {
                result.push({ width: width, focused: i === 0, stack: glance.stack });
            }
            return result;
        }
    }
}
