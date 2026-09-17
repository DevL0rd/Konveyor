import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.konveyor.components
import "Previews.js" as Previews

Item {
    id: preview

    property string actionId
    property bool animated: false
    property bool showGlyph: animated
    property bool after: !animated
    readonly property var spec: Previews.previewFor(actionId)
    readonly property bool back: spec && spec.direction === "back"
    readonly property var baseColumns: [{ width: 0.24 }, { width: 0.3, focused: true }, { width: 0.24 }]

    visible: spec !== null

    Timer {
        interval: 1300
        repeat: true
        running: preview.animated && preview.visible && preview.spec !== null
        onRunningChanged: preview.after = !running
        onTriggered: preview.after = !preview.after
    }

    Loader {
        anchors.fill: parent
        active: preview.spec !== null
        sourceComponent: {
            switch (preview.spec ? preview.spec.kind : "") {
            case "fullscreen": return fullscreenStage;
            case "floating": return floatingStage;
            case "workspace": return workspaceStage;
            case "monitor": return monitorStage;
            default: return rowStage;
            }
        }
    }

    Component {
        id: rowStage
        MiniColumns {
            active: false
            offset: (preview.after ? preview.spec.after : preview.spec.before).offset || 0
            columns: (preview.after ? preview.spec.after : preview.spec.before).columns
        }
    }

    Component {
        id: fullscreenStage
        Item {
            MiniColumns {
                anchors.fill: parent
                active: false
                opacity: preview.after ? 0 : 1
                columns: preview.baseColumns
            }
            Rectangle {
                anchors.fill: parent
                visible: preview.after
                radius: 4
                color: Qt.alpha(Kirigami.Theme.highlightColor, 0.35)
                border.width: 1.5
                border.color: Kirigami.Theme.highlightColor
            }
        }
    }

    Component {
        id: floatingStage
        Item {
            MiniColumns {
                anchors.fill: parent
                active: false
                columns: preview.after ? [{ width: 0.3 }, { width: 0.3 }] : preview.baseColumns
            }
            Rectangle {
                visible: preview.after
                width: parent.width * 0.42
                height: parent.height * 0.6
                anchors.centerIn: parent
                radius: 3
                color: Qt.alpha(Kirigami.Theme.highlightColor, 0.35)
                border.width: 1.5
                border.color: Kirigami.Theme.highlightColor
            }
        }
    }

    Component {
        id: workspaceStage
        Column {
            spacing: 3
            readonly property bool moves: /^move/.test(preview.actionId)
            readonly property bool lower: preview.after !== preview.back
            MiniColumns {
                width: parent.width
                height: (preview.height - 3) / 2
                active: false
                columns: parent.moves && parent.lower ? [{ width: 0.24 }] : [{ width: 0.24 }, { width: 0.3, focused: !parent.lower }]
            }
            MiniColumns {
                width: parent.width
                height: (preview.height - 3) / 2
                active: false
                columns: parent.moves && parent.lower ? [{ width: 0.3, focused: true }, { width: 0.24 }] : [{ width: 0.3, focused: parent.lower }, { width: 0.24 }]
            }
        }
    }

    Component {
        id: monitorStage
        Row {
            spacing: 4
            readonly property bool moves: /move-column/.test(preview.actionId)
            MiniColumns {
                width: (parent.width - 4) / 2
                height: parent.height
                active: false
                columns: parent.moves && preview.after ? [{ width: 0.4 }] : [{ width: 0.4 }, { width: 0.4, focused: !preview.after }]
            }
            MiniColumns {
                width: (parent.width - 4) / 2
                height: parent.height
                active: false
                columns: parent.moves && preview.after ? [{ width: 0.4 }, { width: 0.4, focused: true }] : [{ width: 0.4, focused: preview.after }]
            }
        }
    }

    Kirigami.Icon {
        visible: preview.showGlyph && preview.spec !== null
        source: {
            if (!preview.spec) {
                return "";
            }
            const glyph = preview.spec.glyph;
            if (!preview.back) {
                return glyph;
            }
            return glyph.replace("go-next", "go-previous").replace("go-down", "go-up");
        }
        width: Kirigami.Units.iconSizes.small
        height: width
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 2
        opacity: preview.after ? 1 : 0.3
        Behavior on opacity {
            NumberAnimation { duration: Kirigami.Units.shortDuration }
        }
    }
}
