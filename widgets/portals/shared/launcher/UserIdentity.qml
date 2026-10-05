import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.components as Components
import org.kde.plasma.components as PlasmaComponents

RowLayout {
    spacing: Kirigami.Units.largeSpacing
    Components.Avatar {
        Layout.preferredWidth: Kirigami.Units.iconSizes.medium + Kirigami.Units.smallSpacing
        Layout.preferredHeight: Layout.preferredWidth
        source: launcherData.user.faceIconUrl
        name: launcherData.user.fullName || launcherData.user.loginName
    }
    ColumnLayout {
        spacing: 0
        Layout.fillWidth: true
        PlasmaComponents.Label {
            text: launcherData.user.fullName || launcherData.user.loginName
            font.weight: Font.DemiBold
            elide: Text.ElideRight
            Layout.fillWidth: true
        }
        PlasmaComponents.Label {
            text: launcherData.user.host
            font: Kirigami.Theme.smallFont
            opacity: 0.55
            elide: Text.ElideRight
            Layout.fillWidth: true
        }
    }
}
