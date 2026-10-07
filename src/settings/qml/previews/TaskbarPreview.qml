import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.konveyor.components

Item {
    id: preview

    property var settings: ({})
    property int phase: 0

    readonly property var phases: settings.showShortcutBadges === false ? [""] : ["", "Mod", "Mod+Alt"]
    readonly property string keys: phases[phase % phases.length]

    Timer {
        interval: 1800
        repeat: true
        running: preview.visible && preview.phases.length > 1
        onTriggered: preview.phase = (preview.phase + 1) % preview.phases.length
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Kirigami.Units.largeSpacing

        MiniTaskbar {
            Layout.fillWidth: true
            Layout.fillHeight: true
            maximumThickness: Kirigami.Units.gridUnit * 3.5
            settings: preview.settings
            badges: preview.keys === "Mod" ? "workspaces" : preview.keys === "Mod+Alt" ? "columns" : ""
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredHeight: Kirigami.Units.gridUnit * 1.6
            spacing: Kirigami.Units.smallSpacing

            KeyCaps {
                visible: preview.keys.length > 0
                keyName: preview.keys
            }

            QQC2.Label {
                text: preview.keys === "Mod" ? "held: the keys that switch workspaces"
                    : preview.keys === "Mod+Alt" ? "held: the keys that jump to each app"
                    : "Shared columns sit together in a capsule, one icon per window"
                opacity: 0.7
            }
        }
    }
}
