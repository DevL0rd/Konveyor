import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

Item {
    id: preview

    property var values: ({})
    readonly property var output: kcm.live.outputs.length > 0 ? kcm.live.outputs[0].logical : ({ width: 1920, height: 1080 })
    readonly property real screenWidth: output.width
    readonly property real screenHeight: output.height
    readonly property var struts: values.struts || ({ left: 0, right: 0, top: 0, bottom: 0 })
    readonly property real gaps: values.gaps || 0
    readonly property var ring: values["focus-ring"] || ({ enabled: false, width: 0 })
    readonly property var border: values.border || ({ enabled: false, width: 0 })
    property bool opened: false

    function widthPx(size) {
        if (!size) {
            return screenWidth * 0.4;
        }
        return size.kind === "fixed" ? size.value : size.value * (screenWidth - struts.left - struts.right - gaps) - gaps;
    }

    function paintColor(paint, fallback) {
        if (!paint) {
            return fallback;
        }
        if (paint.gradient) {
            return paint.gradient.from;
        }
        return paint.source === "color" ? paint.color : Kirigami.Theme.highlightColor;
    }

    readonly property var columnWidths: {
        const fresh = widthPx(values["default-column-width"]);
        const base = [screenWidth * 0.3, fresh, screenWidth * 0.45, screenWidth * 0.3];
        if (opened) {
            base.splice(values["new-column-position"] === "left" ? 1 : 2, 0, fresh);
        }
        return base;
    }
    readonly property int focusedIndex: opened ? (values["new-column-position"] === "left" ? 1 : 2) : 1

    Timer {
        interval: 2200
        repeat: true
        running: preview.visible
        onTriggered: preview.opened = !preview.opened
    }

    MonitorFrame {
        id: frame
        anchors.centerIn: parent
        height: Math.min(parent.height, parent.width / aspect)
        aspect: preview.screenWidth / Math.max(1, preview.screenHeight)
        color: preview.values["background-color"] || Qt.darker(Kirigami.Theme.backgroundColor, 1.6)

        readonly property real k: (width - 10) / preview.screenWidth

        Rectangle {
            x: 0
            y: 0
            width: parent.width
            height: parent.height
            color: "transparent"
            border.color: Qt.alpha(Kirigami.Theme.neutralTextColor, 0.6)
            border.width: 0
        }

        Item {
            id: area
            x: preview.struts.left * frame.k
            y: preview.struts.top * frame.k
            width: parent.width - x - preview.struts.right * frame.k
            height: parent.height - y - preview.struts.bottom * frame.k

            readonly property real g: preview.gaps > 0 ? Math.max(1.5, preview.gaps * frame.k) : 0
            readonly property var starts: {
                const result = [];
                let x = g;
                for (const w of preview.columnWidths) {
                    result.push(x);
                    x += w * frame.k + g;
                }
                result.push(x);
                return result;
            }
            readonly property real viewOffset: {
                const mode = preview.values["center-focused-column"];
                const i = preview.focusedIndex;
                const left = starts[i] - g;
                const w = preview.columnWidths[i] * frame.k;
                const total = starts[starts.length - 1];
                if (preview.columnWidths.length <= 1 && preview.values["always-center-single-column"]) {
                    return left - (width - w) / 2 + g;
                }
                const centered = left - (width - w - 2 * g) / 2;
                if (mode === "always") {
                    return centered;
                }
                if (mode === "on-overflow") {
                    const neighbor = i + 1 < preview.columnWidths.length ? preview.columnWidths[i + 1] * frame.k + g : 0;
                    if (w + neighbor + 2 * g > width) {
                        return centered;
                    }
                }
                return Math.max(0, Math.min(left, total - width));
            }

            Repeater {
                model: preview.columnWidths.length

                Rectangle {
                    required property int index
                    readonly property bool focused: index === preview.focusedIndex
                    x: area.starts[index] - area.viewOffset
                    y: area.g
                    width: preview.columnWidths[index] * frame.k
                    height: area.height - 2 * area.g
                    radius: 3
                    color: Qt.alpha(Kirigami.Theme.backgroundColor, focused ? 0.95 : 0.75)
                    border.width: focused && preview.ring.enabled ? Math.max(1.5, preview.ring.width * frame.k * 3)
                                  : (preview.border.enabled ? Math.max(1, preview.border.width * frame.k * 3) : 0)
                    border.color: focused ? preview.paintColor(preview.ring.enabled ? preview.ring.active : preview.border.active, Kirigami.Theme.highlightColor)
                                          : preview.paintColor(preview.border.inactive, Kirigami.Theme.disabledTextColor)

                    Behavior on x {
                        NumberAnimation {
                            duration: Kirigami.Units.longDuration
                            easing.type: Easing.OutCubic
                        }
                    }

                    Behavior on width {
                        NumberAnimation {
                            duration: Kirigami.Units.longDuration
                            easing.type: Easing.OutCubic
                        }
                    }

                    Rectangle {
                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.margins: parent.border.width
                        height: Math.max(3, parent.height * 0.06)
                        color: Qt.alpha(Kirigami.Theme.textColor, 0.12)
                    }
                }
            }
        }
    }

    QQC2.Label {
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        text: preview.opened ? "New window opened" : "Before"
        font: Kirigami.Theme.smallFont
        opacity: 0.7
    }
}
