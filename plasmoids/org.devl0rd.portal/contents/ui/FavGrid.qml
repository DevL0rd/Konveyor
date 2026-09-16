import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PlasmaComponents
import org.kde.plasma.plasmoid
import "lib/Highlight.js" as Highlight
import org.kde.plasma.plasma5support as P5Support

Item {
    id: root
    property var favorites: []
    property string searchText: ""
    property string viewMode: "grid"
    property bool sectionMode: false
    signal launched()
    signal removeFav(string resource)

    readonly property int iconSize: Plasmoid.configuration.iconSize

    readonly property int cellW: iconSize + Kirigami.Units.gridUnit * 2
    readonly property int rowHeight: iconSize + (Plasmoid.configuration.showAppLabels ? Kirigami.Units.gridUnit * 2.4 : Kirigami.Units.smallSpacing * 3)
    readonly property int listRowHeight: Math.round(iconSize * 0.8) + Kirigami.Units.smallSpacing * 2
    readonly property int maxCols: Math.max(1, Math.floor(width / cellW))
    readonly property int contentHeightHint: {
        var n = items.length
        if (n === 0) return 0
        if (viewMode === "list") return n * listRowHeight
        var cols = Math.max(1, Math.min(n, maxCols))
        return Math.ceil(n / cols) * rowHeight
    }

    readonly property var items: {
        var q = root.searchText.trim().toLowerCase()
        return (root.favorites || []).filter(function(f) {
            return q === "" || (f.name || "").toLowerCase().indexOf(q) >= 0
        })
    }

    P5Support.DataSource {
        id: runner
        engine: "executable"
        onNewData: function(source, d) { disconnectSource(source) }
    }
    function activateFirst() {
        if (items.length === 0) return false
        activate(items[0])
        return true
    }
    function activate(f) { if (f && f.launch) { runner.connectSource(f.launch); root.launched() } }

    GridView {
        id: grid
        visible: root.viewMode === "grid"
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        width: cols * cellWidth
        clip: true
        interactive: !root.sectionMode
        model: root.viewMode === "grid" ? root.items : []
        readonly property int cols: Math.max(1, Math.min(count, root.maxCols))
        cellWidth: root.cellW
        cellHeight: root.rowHeight
        boundsBehavior: Flickable.StopAtBounds
        QQC2.ScrollBar.vertical: QQC2.ScrollBar { visible: !root.sectionMode }

        delegate: Item {
            width: grid.cellWidth
            height: grid.cellHeight
            Rectangle {
                anchors.fill: parent
                anchors.margins: Kirigami.Units.smallSpacing / 2
                radius: Kirigami.Units.cornerRadius * 2
                color: cellMa.containsMouse ? Qt.alpha(Kirigami.Theme.highlightColor, 0.14) : "transparent"
                border.width: cellMa.containsMouse ? 1 : 0
                border.color: Qt.alpha(Kirigami.Theme.highlightColor, 0.35)
                Behavior on color { ColorAnimation { duration: 120 } }
            }
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Kirigami.Units.smallSpacing
                spacing: Kirigami.Units.smallSpacing / 2
                Kirigami.Icon {
                    source: modelData.icon || "application-x-executable"
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: root.iconSize
                    Layout.preferredHeight: root.iconSize
                }
                PlasmaComponents.Label {
                    visible: Plasmoid.configuration.showAppLabels
                    text: root.searchText.trim() !== "" ? Highlight.mark(modelData.name || "", root.searchText.trim(), Kirigami.Theme.highlightColor) : modelData.name || ""
                    textFormat: root.searchText.trim() !== "" ? Text.StyledText : Text.PlainText
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideRight
                    maximumLineCount: 2
                    wrapMode: Text.Wrap
                    font.pointSize: Kirigami.Theme.smallFont.pointSize
                    opacity: 0.9
                }
            }
            MouseArea {
                id: cellMa
                cursorShape: Qt.PointingHandCursor
                anchors.fill: parent
                hoverEnabled: true
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                onClicked: function(m) { if (m.button === Qt.RightButton) { favMenu.fav = modelData; favMenu.popup() } }
                onDoubleClicked: function(m) { if (m.button === Qt.LeftButton) root.activate(modelData) }
            }
        }
    }

    ListView {
        id: list
        visible: root.viewMode === "list"
        anchors.fill: parent
        clip: true
        interactive: !root.sectionMode
        model: root.viewMode === "list" ? root.items : []
        boundsBehavior: Flickable.StopAtBounds
        QQC2.ScrollBar.vertical: QQC2.ScrollBar { visible: !root.sectionMode }

        delegate: Rectangle {
            width: list.width
            height: root.listRowHeight
            radius: Kirigami.Units.cornerRadius * 2
            color: rowMa.containsMouse ? Qt.alpha(Kirigami.Theme.highlightColor, 0.14) : "transparent"
            border.width: rowMa.containsMouse ? 1 : 0
            border.color: Qt.alpha(Kirigami.Theme.highlightColor, 0.35)
            Behavior on color { ColorAnimation { duration: 120 } }
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Kirigami.Units.smallSpacing
                anchors.rightMargin: Kirigami.Units.smallSpacing
                spacing: Kirigami.Units.largeSpacing
                Kirigami.Icon {
                    source: modelData.icon || "application-x-executable"
                    Layout.preferredWidth: root.iconSize * 0.8
                    Layout.preferredHeight: root.iconSize * 0.8
                }
                PlasmaComponents.Label {
                    Layout.fillWidth: true
                    text: root.searchText.trim() !== "" ? Highlight.mark(modelData.name || "", root.searchText.trim(), Kirigami.Theme.highlightColor) : modelData.name || ""
                    textFormat: root.searchText.trim() !== "" ? Text.StyledText : Text.PlainText
                    elide: Text.ElideRight
                }
            }
            MouseArea {
                id: rowMa
                anchors.fill: parent
                hoverEnabled: true
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                onClicked: function(m) { if (m.button === Qt.RightButton) { favMenu.fav = modelData; favMenu.popup() } }
                onDoubleClicked: function(m) { if (m.button === Qt.LeftButton) root.activate(modelData) }
            }
        }
    }

    QQC2.Menu {
        id: favMenu
        property var fav: null
        QQC2.MenuItem { text: i18n("Launch"); icon.name: "media-playback-start"; enabled: favMenu.fav && favMenu.fav.launch; onTriggered: root.activate(favMenu.fav) }
        QQC2.MenuSeparator {}
        QQC2.MenuItem {
            text: i18n("Remove from Favourites"); icon.name: "list-remove"
            onTriggered: if (favMenu.fav) root.removeFav(favMenu.fav.resource)
        }
    }

    PlasmaComponents.Label {
        anchors.centerIn: parent
        visible: !root.sectionMode && root.items.length === 0
        text: root.searchText !== "" ? i18n("No matches") : i18n("No favourites yet")
        opacity: 0.5
    }
}
