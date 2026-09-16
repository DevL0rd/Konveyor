import QtQuick
import org.kde.kirigami as Kirigami

Item {
    id: preview

    property var values: ({})
    readonly property var tab: values["tab-indicator"] || ({})
    readonly property int count: 3
    property int showing: 0
    readonly property bool vertical: tab.position === "left" || tab.position === "right" || !tab.position
    readonly property real zoom: 1.5
    readonly property bool visibleMarks: tab.enabled === true

    function paintFor(state) {
        const own = tab[state];
        if (own) {
            return own;
        }
        const ring = values["focus-ring"] || {};
        return ring[state === "inactive" ? "inactive" : (state === "urgent" ? "urgent" : "active")];
    }

    Timer {
        interval: 1400
        repeat: true
        running: preview.visible
        onTriggered: preview.showing = (preview.showing + 1) % preview.count
    }

    MockWindow {
        id: column
        anchors.centerIn: parent
        width: parent.width * 0.62
        height: parent.height * 0.72
        state_: "active"
        caption: "Tab " + (preview.showing + 1) + " of " + preview.count
        ring: preview.values["focus-ring"]
        border: preview.values.border
    }

    Repeater {
        model: preview.visibleMarks ? preview.count : 0

        PaintFrame {
            required property int index
            readonly property real side: preview.vertical ? column.height : column.width
            readonly property real total: side * Math.max(0.05, Math.min(1, preview.tab.length || 0.5))
            readonly property real between: (preview.tab["gaps-between-tabs"] || 0) * preview.zoom
            readonly property real each: (total - between * (preview.count - 1)) / preview.count
            readonly property real along: (side - total) / 2 + index * (each + between)
            readonly property real thick: Math.max(2, (preview.tab.width || 4) * preview.zoom)
            readonly property real offset: (preview.tab.gap || 0) * preview.zoom
            readonly property bool inside: preview.tab["place-within-column"] === true

            filled: true
            radius: Math.min((preview.tab["corner-radius"] || 0) * preview.zoom, Math.min(width, height) / 2)
            paint: preview.paintFor(index === preview.showing ? "active" : "inactive")
            width: preview.vertical ? thick : each
            height: preview.vertical ? each : thick
            x: {
                if (!preview.vertical) {
                    return column.x + along;
                }
                return preview.tab.position === "right" ? (inside ? column.x + column.width - thick - offset : column.x + column.width + offset)
                                                        : (inside ? column.x + offset : column.x - thick - offset);
            }
            y: {
                if (preview.vertical) {
                    return column.y + along;
                }
                return preview.tab.position === "bottom" ? (inside ? column.y + column.height - thick - offset : column.y + column.height + offset)
                                                         : (inside ? column.y + offset : column.y - thick - offset);
            }
        }
    }
}
