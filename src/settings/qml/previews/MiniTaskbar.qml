import QtQuick
import org.kde.kirigami as Kirigami

Item {
    id: bar

    property var settings: ({})
    property var overrides: ({})
    property string badges: ""
    readonly property string shownBadges: overrides.badges !== undefined ? overrides.badges : badges
    property bool cycling: true
    property int focusIndex: 1
    property real maximumThickness: Number.POSITIVE_INFINITY
    property int clicks: 0
    readonly property bool clickDemo: overrides.activeClick !== undefined
    readonly property int shownFocus: !clickDemo ? focusIndex : overrides.activeClick === 2 && clicks % 2 === 1 ? 2 : 1
    readonly property int clickedAt: overrides.activeClick === 2 && clicks > 0 && clicks % 2 === 0 ? 2 : 1
    readonly property int dimmed: clickDemo && overrides.activeClick === 0 && clicks % 2 === 1 ? 1 : -1

    readonly property var look: {
        const result = {};
        for (const key of ["showApps", "showWorkspaces", "autoIconSize", "iconSize", "iconSpacing", "buttonPadding", "highlightStyle",
                 "indicatorStyle", "indicatorEdge", "showSeparator", "groupMode", "workspacesAfterTasks", "pillContent"]) {
            result[key] = bar.overrides[key] !== undefined ? bar.overrides[key] : bar.settings[key];
        }
        return result;
    }
    readonly property real thickness: Math.min(height, width / 7, maximumThickness)
    readonly property real iconSize: look.autoIconSize === false ? thickness * Math.min(0.9, (look.iconSize || 32) / 48) : thickness * 0.6
    readonly property real padding: Math.min(thickness * 0.2, (look.buttonPadding === undefined ? 3 : look.buttonPadding) * thickness / 48)
    readonly property real spacing: (look.iconSpacing === undefined ? 2 : look.iconSpacing) * thickness / 48
    readonly property bool showApps: look.showApps !== false || look.showWorkspaces === false
    readonly property bool showWorkspaces: look.showWorkspaces !== false
    readonly property bool stripAfter: look.workspacesAfterTasks === true
    readonly property bool opposite: look.indicatorEdge === 1
    readonly property bool grouped: look.groupMode !== 2
    readonly property var entries: grouped
        ? [{ icons: ["system-file-manager"], count: 1 }, { icons: ["internet-web-browser"], count: 2 },
           { icons: ["utilities-terminal", "accessories-text-editor"], capsule: true }, { icons: ["multimedia-player"], count: 1 }]
        : [{ icons: ["system-file-manager"], count: 1 }, { icons: ["internet-web-browser"], count: 1 }, { icons: ["internet-web-browser"], count: 1 },
           { icons: ["utilities-terminal", "accessories-text-editor"], capsule: true }, { icons: ["multimedia-player"], count: 1 }]
    readonly property var firsts: entries.map((entry, index) => entries.slice(0, index).reduce((sum, each) => sum + each.icons.length, 0))
    readonly property int iconCount: entries.reduce((sum, entry) => sum + entry.icons.length, 0)

    Timer {
        interval: 1400
        repeat: true
        running: (bar.cycling || bar.clickDemo) && bar.visible
        onTriggered: {
            if (bar.clickDemo)
                ++bar.clicks
            else
                bar.focusIndex = (bar.focusIndex + 1) % bar.iconCount
        }
    }

    Rectangle {
        id: panel
        anchors.centerIn: parent
        width: Math.min(parent.width, content.implicitWidth + bar.thickness * 0.4)
        height: bar.thickness
        radius: Kirigami.Units.cornerRadius
        color: Qt.alpha(Kirigami.Theme.textColor, 0.08)
        border.width: 1
        border.color: Qt.alpha(Kirigami.Theme.textColor, 0.12)

        Row {
            id: content
            anchors.centerIn: parent
            spacing: bar.thickness * 0.12
            layoutDirection: bar.stripAfter ? Qt.RightToLeft : Qt.LeftToRight

            Rectangle {
                id: strip
                visible: bar.showWorkspaces
                anchors.verticalCenter: parent.verticalCenter
                width: pills.implicitWidth + bar.thickness * 0.16
                height: bar.thickness * 0.66
                radius: height / 2
                color: Qt.alpha(Kirigami.Theme.textColor, 0.07)

                Row {
                    id: pills
                    anchors.centerIn: parent
                    spacing: bar.thickness * 0.06

                    Repeater {
                        model: 2

                        MiniPill {
                            required property int index
                            size: bar.thickness
                            current: index === 0
                            mode: bar.look.pillContent || 0
                            number: index + 1
                            columns: index === 0 ? 4 : 0
                            badge: bar.shownBadges === "workspaces" ? String(index + 1) : ""
                        }
                    }
                }
            }

            Rectangle {
                visible: bar.showWorkspaces && bar.showApps && bar.look.showSeparator !== false
                anchors.verticalCenter: parent.verticalCenter
                width: 1
                height: bar.thickness * 0.5
                color: Qt.alpha(Kirigami.Theme.textColor, 0.25)
            }

            Row {
                visible: bar.showApps
                anchors.verticalCenter: parent.verticalCenter
                spacing: bar.spacing
                layoutDirection: Qt.LeftToRight

                Repeater {
                    model: bar.entries

                    MiniTask {
                        required property var modelData
                        required property int index
                        strip: bar
                        icons: modelData.icons
                        capsule: !!modelData.capsule
                        count: modelData.count || 1
                        focusedIcon: bar.shownFocus - bar.firsts[index]
                        dimmedIcon: bar.dimmed - bar.firsts[index]
                        clickedIcon: bar.clickDemo ? bar.clickedAt - bar.firsts[index] : -1
                        clickSerial: bar.clicks
                        badge: bar.shownBadges === "columns" ? String(index + 1) : ""
                    }
                }
            }
        }
    }
}
