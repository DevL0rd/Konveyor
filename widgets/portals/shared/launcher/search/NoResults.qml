import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PlasmaComponents

ColumnLayout {
    visible: page.totalResults === 0 && !launcherData.runner.querying && page.term !== "" && !(launcherData.packagesBusy && page.showPackages)
    Layout.fillWidth: true
    Layout.topMargin: Kirigami.Units.gridUnit * 4
    spacing: Kirigami.Units.largeSpacing
    Kirigami.Icon {
        Layout.alignment: Qt.AlignHCenter
        Layout.preferredWidth: Kirigami.Units.iconSizes.huge
        Layout.preferredHeight: Layout.preferredWidth
        source: "edit-find-symbolic"
        color: launcher.ink
        isMask: true
        opacity: 0.35
    }
    PlasmaComponents.Label {
        Layout.alignment: Qt.AlignHCenter
        text: i18n("Nothing found for “%1”", page.term)
        font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.3
        font.weight: Font.DemiBold
    }
    PlasmaComponents.Label {
        Layout.alignment: Qt.AlignHCenter
        text: i18n("Narrow it down with a prefix")
        opacity: 0.55
    }
    RowLayout {
        Layout.alignment: Qt.AlignHCenter
        spacing: Kirigami.Units.smallSpacing
        Repeater {
            model: [
                { prefix: "g ", label: i18n("Games") },
                { prefix: "a ", label: i18n("Apps") },
                { prefix: "f ", label: i18n("Files") },
                { prefix: "s ", label: i18n("Packages") },
                { prefix: "@", label: i18n("Friends") },
                { prefix: "=", label: i18n("Math") },
                { prefix: ">", label: i18n("Command") }
            ]
            MouseArea {
                id: chip
                required property var modelData
                implicitWidth: chipRow.implicitWidth + Kirigami.Units.largeSpacing * 2
                implicitHeight: Kirigami.Units.gridUnit * 1.9
                hoverEnabled: true
                onClicked: launcher.setQuery(modelData.prefix + page.term)
                Rectangle {
                    anchors.fill: parent
                    radius: height / 2
                    color: chip.containsMouse ? launcher.selectedFill : launcher.well
                    border.width: 1
                    border.color: launcher.hairline
                }
                RowLayout {
                    id: chipRow
                    anchors.centerIn: parent
                    spacing: Kirigami.Units.smallSpacing
                    PlasmaComponents.Label {
                        text: chip.modelData.prefix.trim()
                        font.family: "monospace"
                        font.weight: Font.DemiBold
                    }
                    PlasmaComponents.Label {
                        text: chip.modelData.label
                        opacity: 0.7
                    }
                }
            }
        }
    }
}
