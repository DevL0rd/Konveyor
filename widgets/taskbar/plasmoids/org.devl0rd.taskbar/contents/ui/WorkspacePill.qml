import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.plasmoid

PlasmaCore.ToolTipArea {
    id: pill

    required property var workspace
    property real size: 32
    property bool vertical: false
    property string badge
    property bool showBadge: false
    property int contentMode: 0
    property bool animate: true

    readonly property var columns: workspace.columns || []
    readonly property bool current: !!workspace.is_active
    readonly property bool urgent: !!workspace.is_urgent
    readonly property string name: workspace.name || ""
    readonly property bool dots: contentMode === 0
    readonly property string label: contentMode === 0 ? name : contentMode === 1 && name ? name : String(workspace.idx || "")
    readonly property int shortDuration: animate ? Kirigami.Units.shortDuration : 0
    readonly property int longDuration: animate ? Kirigami.Units.longDuration : 0
    readonly property int maxDots: 5
    readonly property int dot: Math.max(4, Math.round(size * 0.14))
    readonly property real thickness: Math.round(size * 0.52)
    readonly property color ink: current ? Kirigami.Theme.highlightedTextColor : Kirigami.Theme.textColor
    readonly property int activeColumn: columns.findIndex(column => column.indexOf(workspace.active_window_id) >= 0)

    signal picked()

    implicitWidth: vertical ? thickness : Math.max(thickness, content.implicitWidth + thickness * 0.6)
    implicitHeight: vertical ? Math.max(thickness, content.implicitHeight + thickness * 0.6) : thickness
    mainText: name || i18n("Workspace %1", workspace.idx)
    subText: columns.length === 0 ? i18n("Empty") : i18np("%1 column", "%1 columns", columns.length)
    location: Plasmoid.location

    Behavior on implicitWidth {
        NumberAnimation { duration: pill.longDuration; easing.type: Easing.OutCubic }
    }
    Behavior on implicitHeight {
        NumberAnimation { duration: pill.longDuration; easing.type: Easing.OutCubic }
    }

    Rectangle {
        anchors.fill: parent
        radius: Math.min(width, height) / 2
        color: pill.current ? Kirigami.Theme.highlightColor : Kirigami.Theme.textColor
        opacity: pill.current ? 1 : area.containsMouse ? 0.16 : 0

        Behavior on opacity {
            NumberAnimation { duration: pill.shortDuration; easing.type: Easing.OutCubic }
        }
        Behavior on color {
            ColorAnimation { duration: pill.longDuration }
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: Math.min(width, height) / 2
        color: "transparent"
        border.width: 2
        border.color: Kirigami.Theme.neutralTextColor
        visible: pill.urgent
    }

    Grid {
        id: content
        anchors.centerIn: parent
        rows: pill.vertical ? pill.maxDots + 3 : 1
        columns: pill.vertical ? 1 : pill.maxDots + 3
        spacing: Math.max(2, Math.round(pill.dot * 0.55))
        horizontalItemAlignment: Grid.AlignHCenter
        verticalItemAlignment: Grid.AlignVCenter

        Text {
            visible: pill.label.length > 0 && (!pill.vertical || pill.label.length <= 2)
            text: pill.label
            color: pill.ink
            font.pixelSize: Math.round(pill.thickness * 0.5)
            font.weight: pill.current ? Font.DemiBold : Font.Normal
            elide: Text.ElideRight
            width: Math.min(implicitWidth, pill.size * 2.5)
        }

        Rectangle {
            visible: pill.dots && pill.columns.length === 0
            width: pill.dot
            height: pill.dot
            radius: pill.dot / 2
            color: "transparent"
            border.width: 1
            border.color: pill.ink
            opacity: 0.6
        }

        Repeater {
            model: pill.dots ? Math.min(pill.columns.length, pill.maxDots) : 0

            Rectangle {
                required property int index
                readonly property bool focused: index === pill.activeColumn
                readonly property real span: focused && pill.current ? pill.dot * 2.2 : pill.dot

                width: pill.vertical ? pill.dot : span
                height: pill.vertical ? span : pill.dot
                radius: pill.dot / 2
                color: pill.ink
                opacity: focused ? 1 : pill.current ? 0.75 : 0.5

                Behavior on width {
                    NumberAnimation { duration: pill.longDuration; easing.type: Easing.OutCubic }
                }
                Behavior on height {
                    NumberAnimation { duration: pill.longDuration; easing.type: Easing.OutCubic }
                }
            }
        }

        Text {
            visible: pill.dots && pill.columns.length > pill.maxDots
            text: "+" + (pill.columns.length - pill.maxDots)
            color: pill.ink
            font.pixelSize: Math.round(pill.thickness * 0.42)
        }
    }

    ShortcutBadge {
        label: pill.badge
        shown: pill.showBadge
        animate: pill.animate
        anchors.horizontalCenter: parent.right
        anchors.verticalCenter: parent.top
    }

    MouseArea {
        id: area
        anchors.fill: parent
        hoverEnabled: true
        onClicked: pill.picked()
    }
}
