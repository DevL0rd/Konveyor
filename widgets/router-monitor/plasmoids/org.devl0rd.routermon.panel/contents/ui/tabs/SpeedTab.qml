import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.plasma.plasmoid
import org.kde.plasma.components as PlasmaComponents
import "../lib"

PopScroll {
    id: speedTab

    component CenteredStat: Item {
        property alias label: centered.label
        property alias value: centered.value
        property alias unit: centered.unit
        property alias color: centered.color
        Layout.fillWidth: true
        Layout.preferredWidth: 1
        implicitHeight: centered.implicitHeight
        PopStat {
            id: centered
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }

    readonly property int plan: Plasmoid.configuration.planDownMbps
    readonly property real gaugeMax: plan > 0 ? plan : 1000
    readonly property real liveMbps: root.live ? root.live.mbps || 0 : 0
    readonly property string phase: root.live ? root.live.phase || "" : ""
    readonly property real shownMbps: root.testing ? liveMbps : Plasmoid.configuration.peakDown
    readonly property var shown: root.speed(shownMbps)

    PopCard {
        title: i18n("Internet speed")
        icon: "speedometer"
        trailing: root.lastResult && root.lastResult.server ? root.lastResult.server : ""

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: Kirigami.Units.gridUnit * 11

            ArcGauge {
                id: gauge
                anchors.centerIn: parent
                width: Math.min(parent.width, parent.height)
                height: width
                value: speedTab.shownMbps
                maxValue: speedTab.gaugeMax
                running: root.testing && speedTab.liveMbps <= 0
                color: speedTab.phase === "upload" ? root.upColor : root.downColor
                tip: i18n("%1 of %2 Mb/s", Math.round(speedTab.shownMbps), speedTab.gaugeMax)
            }

            ColumnLayout {
                anchors.centerIn: parent
                spacing: 0
                PlasmaComponents.Label {
                    Layout.alignment: Qt.AlignHCenter
                    text: root.testing ? (speedTab.phase === "upload" ? i18n("↑ Upload") : speedTab.phase === "ping" || speedTab.phase === "" ? i18n("Ping…") : i18n("↓ Download"))
                                       : i18n("Peak download")
                    font: Kirigami.Theme.smallFont
                    opacity: 0.65
                }
                PlasmaComponents.Label {
                    Layout.alignment: Qt.AlignHCenter
                    text: root.testing && speedTab.liveMbps <= 0 ? "…" : speedTab.shownMbps > 0 ? speedTab.shown.value : "—"
                    font.pixelSize: gauge.width * 0.2
                    font.weight: Font.DemiBold
                    font.features: { "tnum": 1 }
                    color: speedTab.phase === "upload" ? root.upColor : root.downColor
                }
                PlasmaComponents.Label {
                    Layout.alignment: Qt.AlignHCenter
                    text: speedTab.shown.unit
                    opacity: 0.6
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            CenteredStat {
                label: i18n("Upload")
                value: Plasmoid.configuration.peakUp > 0 ? root.speed(Plasmoid.configuration.peakUp).value : "—"
                unit: Plasmoid.configuration.peakUp > 0 ? root.speed(Plasmoid.configuration.peakUp).unit : ""
                color: root.upColor
            }
            CenteredStat {
                label: i18n("Ping")
                value: root.lastResult ? (root.lastResult.ping_ms || 0).toFixed(0) : "—"
                unit: root.lastResult ? "ms" : ""
            }
            CenteredStat {
                label: i18n("Jitter")
                value: root.lastResult ? (root.lastResult.jitter_ms || 0).toFixed(1) : "—"
                unit: root.lastResult ? "ms" : ""
            }
            CenteredStat {
                label: i18n("Loss")
                value: root.lastResult ? (root.lastResult.loss || 0) + "" : "—"
                unit: root.lastResult ? "%" : ""
            }
        }

        PlasmaComponents.Button {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: Kirigami.Units.smallSpacing
            text: root.testing ? i18n("Testing…") : i18n("Run speed test")
            icon.name: root.testing ? "view-refresh" : "media-playback-start"
            enabled: !root.testing
            onClicked: root.runSpeedTest()
        }
        PlasmaComponents.Label {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            text: i18n("The gauge holds the fastest speed seen since the last test")
            font: Kirigami.Theme.smallFont
            opacity: 0.5
            wrapMode: Text.WordWrap
        }
    }

    PopCard {
        visible: root.speedHistory.length > 0
        title: i18n("History")
        icon: "view-history"
        trailing: i18np("%1 test", "%1 tests", root.speedHistory.length)

        Sparkline {
            visible: root.speedHistory.length > 1
            Layout.fillWidth: true
            Layout.preferredHeight: Kirigami.Units.gridUnit * 3
            values: root.speedHistory.slice().reverse().map(result => result.down_mbps || 0)
            values2: root.speedHistory.slice().reverse().map(result => result.up_mbps || 0)
            lineColor: root.downColor
            lineColor2: root.upColor
            gradient: false
            tipText: v => root.speedText(v)
        }

        Repeater {
            model: root.speedHistory

            MouseArea {
                id: historyRow
                required property var modelData
                required property int index
                Layout.fillWidth: true
                implicitHeight: historyLayout.implicitHeight + Kirigami.Units.smallSpacing
                hoverEnabled: true

                RowLayout {
                    id: historyLayout
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    PlasmaComponents.Label {
                        text: historyRow.modelData.ts ? Qt.formatDateTime(new Date(historyRow.modelData.ts * 1000), "ddd hh:mm") : historyRow.modelData.time || ""
                        opacity: 0.65
                        Layout.preferredWidth: Kirigami.Units.gridUnit * 5
                    }
                    PlasmaComponents.Label {
                        text: "↓ " + root.speedText(historyRow.modelData.down_mbps)
                        color: root.downColor
                        font.features: { "tnum": 1 }
                        Layout.fillWidth: true
                        Layout.preferredWidth: 1
                    }
                    PlasmaComponents.Label {
                        text: "↑ " + root.speedText(historyRow.modelData.up_mbps)
                        color: root.upColor
                        font.features: { "tnum": 1 }
                        Layout.fillWidth: true
                        Layout.preferredWidth: 1
                    }
                    PlasmaComponents.Label {
                        text: (historyRow.modelData.ping_ms || 0).toFixed(0) + " ms"
                        opacity: 0.75
                        font.features: { "tnum": 1 }
                    }
                    PlasmaComponents.ToolButton {
                        opacity: historyRow.containsMouse ? 1 : 0
                        icon.name: "edit-delete"
                        display: PlasmaComponents.AbstractButton.IconOnly
                        implicitWidth: Kirigami.Units.iconSizes.small + Kirigami.Units.smallSpacing * 2
                        implicitHeight: implicitWidth
                        text: i18n("Remove")
                        onClicked: root.deleteSpeedResult(historyRow.index)
                    }
                }
            }
        }
    }
}
