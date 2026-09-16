import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

ColumnLayout {
    id: preview

    property string rulePath
    readonly property var result: kcm.revision >= 0 && kcm.live.windows ? kcm.checkRule(kcm.node(rulePath)) : ({})
    readonly property var windows: result.windows || []

    spacing: Kirigami.Units.smallSpacing

    RowLayout {
        Layout.fillWidth: true

        Kirigami.Icon {
            source: preview.windows.length ? "checkmark" : "window"
            Layout.preferredWidth: Kirigami.Units.iconSizes.smallMedium
            Layout.preferredHeight: Kirigami.Units.iconSizes.smallMedium
        }

        QQC2.Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            text: !kcm.live.running ? "Konveyor isn't running, so open windows can't be checked."
                : result.error ? "Can't check this rule yet: " + result.error
                : preview.windows.length === 0 ? "No open window matches this rule right now."
                : preview.windows.length === 1 ? "1 open window matches right now" : preview.windows.length + " open windows match right now"
        }

        QQC2.ToolButton {
            icon.name: "view-refresh"
            text: "Refresh"
            display: QQC2.AbstractButton.IconOnly
            onClicked: kcm.live.refresh()
            QQC2.ToolTip.text: "Check open windows again"
            QQC2.ToolTip.visible: hovered
        }
    }

    Flow {
        Layout.fillWidth: true
        spacing: Kirigami.Units.smallSpacing
        visible: preview.windows.length > 0

        Repeater {
            model: preview.windows

            Rectangle {
                id: chip
                required property var modelData
                width: Math.min(chipRow.implicitWidth + Kirigami.Units.largeSpacing * 2, Kirigami.Units.gridUnit * 20)
                height: chipRow.implicitHeight + Kirigami.Units.smallSpacing * 2
                radius: height / 2
                color: Qt.alpha(Kirigami.Theme.highlightColor, 0.15)
                border.color: Qt.alpha(Kirigami.Theme.highlightColor, 0.5)

                RowLayout {
                    id: chipRow
                    anchors.centerIn: parent
                    width: parent.width - Kirigami.Units.largeSpacing * 2
                    spacing: Kirigami.Units.smallSpacing

                    Kirigami.Icon {
                        source: kcm.live.iconFor(chip.modelData.app_id || "")
                        fallback: "application-x-executable"
                        Layout.preferredWidth: Kirigami.Units.iconSizes.small
                        Layout.preferredHeight: Kirigami.Units.iconSizes.small
                    }

                    QQC2.Label {
                        text: chip.modelData.title || chip.modelData.app_id
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }
            }
        }
    }
}
