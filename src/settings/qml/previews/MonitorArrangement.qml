import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import "../components/monitors/MonitorSummary.js" as MonitorSummary
import org.kde.konveyor.settings

Item {
    id: arrangement

    property string highlightProfile
    readonly property var outputs: SettingsStore.live.outputs
    readonly property var bounds: {
        let minX = Infinity, minY = Infinity, maxX = -Infinity, maxY = -Infinity;
        for (const output of outputs) {
            const l = output.logical;
            minX = Math.min(minX, l.x);
            minY = Math.min(minY, l.y);
            maxX = Math.max(maxX, l.x + l.width);
            maxY = Math.max(maxY, l.y + l.height);
        }
        return { x: minX, y: minY, width: maxX - minX, height: maxY - minY };
    }
    readonly property real scale: outputs.length ? Math.min(width / bounds.width, height / bounds.height) : 1

    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - Kirigami.Units.gridUnit * 4
        visible: arrangement.outputs.length === 0
        icon.name: "video-display"
        text: "No monitors to show"
        explanation: "Connected monitors appear here while Konveyor is running."
    }

    Item {
        anchors.centerIn: parent
        width: arrangement.bounds.width * arrangement.scale
        height: arrangement.bounds.height * arrangement.scale
        visible: arrangement.outputs.length > 0

        Repeater {
            model: arrangement.outputs

            Rectangle {
                id: screen
                required property var modelData
                readonly property var logical: modelData.logical
                readonly property string profile: SettingsStore.revision >= 0 ? SettingsStore.profileForOutput(modelData) : ""
                readonly property bool highlighted: arrangement.highlightProfile.length > 0 && profile === arrangement.highlightProfile

                x: (logical.x - arrangement.bounds.x) * arrangement.scale + 3
                y: (logical.y - arrangement.bounds.y) * arrangement.scale + 3
                width: logical.width * arrangement.scale - 6
                height: logical.height * arrangement.scale - 6
                radius: Kirigami.Units.cornerRadius * 2
                color: Qt.darker(Kirigami.Theme.backgroundColor, 1.5)
                border.width: highlighted ? 3 : 2
                border.color: highlighted ? Kirigami.Theme.highlightColor : Qt.alpha(Kirigami.Theme.textColor, 0.35)

                ColumnLayout {
                    anchors.centerIn: parent
                    width: parent.width - Kirigami.Units.largeSpacing * 2
                    spacing: 0

                    QQC2.Label {
                        text: screen.modelData.name
                        font.bold: true
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    QQC2.Label {
                        text: screen.logical.width + " × " + screen.logical.height + " · " + MonitorSummary.ratioLabel(screen.logical.width / screen.logical.height)
                        color: "white"
                        opacity: 0.8
                        font: Kirigami.Theme.smallFont
                        horizontalAlignment: Text.AlignHCenter
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    Rectangle {
                        Layout.alignment: Qt.AlignHCenter
                        Layout.topMargin: Kirigami.Units.smallSpacing
                        implicitWidth: profileLabel.implicitWidth + Kirigami.Units.largeSpacing * 2
                        implicitHeight: profileLabel.implicitHeight + Kirigami.Units.smallSpacing
                        radius: height / 2
                        color: screen.profile.length ? Kirigami.Theme.highlightColor : Qt.alpha("white", 0.2)

                        QQC2.Label {
                            id: profileLabel
                            anchors.centerIn: parent
                            text: screen.profile.length ? "Profile: " + screen.profile : "No profile"
                            color: "white"
                            font: Kirigami.Theme.smallFont
                        }
                    }
                }
            }
        }
    }
}
