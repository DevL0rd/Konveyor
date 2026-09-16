import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.plasma.plasmoid
import org.kde.plasma.components as PlasmaComponents
import "lib"
import "lib/Highlight.js" as Highlight

Rectangle {
    id: row

    required property int index
    required property string time
    required property string date
    required property string app
    required property string unit
    required property string pid
    required property string msg
    required property int prio
    required property bool expanded
    property string query

    readonly property bool wrap: expanded || Plasmoid.configuration.wrapMessages
    readonly property color mark: Kirigami.Theme.highlightColor
    readonly property var lineModel: ({ time: time, app: app, pid: pid, msg: msg })

    implicitHeight: body.implicitHeight + Kirigami.Units.smallSpacing * 2
    color: expanded ? Qt.alpha(Kirigami.Theme.highlightColor, 0.1)
         : mouse.containsMouse ? Qt.alpha(Kirigami.Theme.highlightColor, 0.1)
         : prio <= 3 ? Qt.alpha(Kirigami.Theme.negativeTextColor, 0.06)
         : prio === 4 ? Qt.alpha(Kirigami.Theme.neutralTextColor, 0.045)
         : index % 2 ? Qt.alpha(Kirigami.Theme.textColor, 0.025) : "transparent"

    Rectangle {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.topMargin: 2
        anchors.bottomMargin: 2
        width: 3
        radius: 1.5
        color: root.stripeColor(row.prio)
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onClicked: function(event) {
            if (event.button === Qt.RightButton)
                menu.popup()
            else
                root.toggleExpand(row.index)
        }
    }

    ColumnLayout {
        id: body
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.topMargin: Kirigami.Units.smallSpacing
        anchors.leftMargin: Kirigami.Units.smallSpacing * 3
        anchors.rightMargin: Kirigami.Units.smallSpacing * 2
        spacing: Kirigami.Units.smallSpacing

        RowLayout {
            Layout.fillWidth: true
            spacing: Kirigami.Units.smallSpacing * 2

            PlasmaComponents.Label {
                text: row.time
                font.pointSize: Kirigami.Theme.smallFont.pointSize
                font.features: { "tnum": 1 }
                opacity: 0.5
                Layout.alignment: Qt.AlignTop
            }
            PlasmaComponents.Label {
                visible: Plasmoid.configuration.showApp
                text: Highlight.mark(row.app, row.query, row.mark)
                textFormat: Text.StyledText
                font.pointSize: Kirigami.Theme.smallFont.pointSize
                font.weight: Font.DemiBold
                color: root.accent
                elide: Text.ElideRight
                Layout.preferredWidth: Kirigami.Units.gridUnit * 7
                Layout.maximumWidth: Kirigami.Units.gridUnit * 7
                Layout.alignment: Qt.AlignTop
            }
            PlasmaComponents.Label {
                id: message
                text: row.expanded ? Highlight.mark(row.msg.replace(/\s+$/, ""), row.query, row.mark).replace(/\n/g, "<br>")
                                   : Highlight.mark(row.msg.replace(/\s*\n[\s\S]*$/, " …"), row.query, row.mark)
                textFormat: Text.StyledText
                color: root.prioColor(row.prio)
                font.family: "monospace"
                font.pointSize: Kirigami.Theme.smallFont.pointSize
                wrapMode: row.wrap ? Text.WrapAnywhere : Text.NoWrap
                elide: row.wrap ? Text.ElideNone : Text.ElideRight
                maximumLineCount: row.expanded ? 400 : Plasmoid.configuration.wrapMessages ? 4 : 1
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop
            }
        }

        ColumnLayout {
            visible: row.expanded
            Layout.fillWidth: true
            Layout.bottomMargin: Kirigami.Units.smallSpacing
            spacing: Kirigami.Units.smallSpacing * 2

            Flow {
                Layout.fillWidth: true
                spacing: Kirigami.Units.largeSpacing * 1.5

                PopStat {
                    label: i18n("Level")
                    value: root.priorityNames[row.prio] || row.prio + ""
                    color: root.prioColor(row.prio)
                    scale: 0.95
                }
                PopStat {
                    label: i18n("When")
                    value: row.date + "  " + row.time
                    scale: 0.95
                }
                PopStat {
                    visible: row.pid !== ""
                    label: i18n("PID")
                    value: row.pid
                    scale: 0.95
                }
                PopStat {
                    visible: row.unit !== ""
                    label: i18n("Unit")
                    value: row.unit
                    scale: 0.95
                }
            }

            PopActions {
                showText: row.width > Kirigami.Units.gridUnit * 26
                model: [
                    { text: i18n("Copy line"), icon: "edit-copy", run: () => root.copyText(root.lineText(row.lineModel)) },
                    { text: i18n("Copy message"), icon: "edit-copy", run: () => root.copyText(row.msg) },
                    { text: root.search.toLowerCase() === row.app.toLowerCase() ? i18n("Show all apps") : i18n("Only %1", row.app), icon: "view-filter",
                      run: () => root.search.toLowerCase() === row.app.toLowerCase() ? root.searchRequested("") : root.searchRequested(row.app) },
                    { text: i18n("Mute %1", row.app), icon: "audio-volume-muted", run: () => root.muteApp(row.app) },
                    { text: i18n("Ask Claude"), icon: "help-hint", run: () => root.askClaude(row.time, row.app, row.pid, row.msg, row.prio) }
                ]
            }
        }
    }

    QQC2.Menu {
        id: menu
        QQC2.MenuItem {
            text: i18n("Copy line")
            icon.name: "edit-copy"
            onTriggered: root.copyText(root.lineText(row.lineModel))
        }
        QQC2.MenuItem {
            text: i18n("Copy all")
            icon.name: "edit-copy-all"
            onTriggered: root.copyAll()
        }
        QQC2.MenuSeparator {}
        QQC2.MenuItem {
            text: i18n("Only \"%1\"", row.app)
            icon.name: "view-filter"
            onTriggered: root.searchRequested(row.app)
        }
        QQC2.MenuItem {
            text: i18n("Mute \"%1\"", row.app)
            icon.name: "audio-volume-muted"
            onTriggered: root.muteApp(row.app)
        }
        QQC2.MenuItem {
            text: i18n("Ask Claude")
            icon.name: "help-hint"
            onTriggered: root.askClaude(row.time, row.app, row.pid, row.msg, row.prio)
        }
    }
}
