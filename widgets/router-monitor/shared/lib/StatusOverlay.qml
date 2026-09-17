/*
 * Covers a widget with a centered message when monitoring is paused or the
 * router is unreachable. Driven by the shared RouterData state, so all widgets
 * show the same status at once. When paused it offers a Resume button.
 */
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PlasmaComponents
import org.kde.plasma.plasma5support as P5Support

Rectangle {
    id: overlay

    property bool online: true
    property bool paused: false
    readonly property string ctl: "$HOME/.local/bin/routermon-ctl"

    z: 100
    visible: paused || !online
    color: Qt.alpha(Kirigami.Theme.backgroundColor, 0.9)
    radius: Kirigami.Units.smallSpacing

    // swallow clicks so the (paused) widget underneath can't be interacted with
    MouseArea { anchors.fill: parent }

    P5Support.DataSource {
        id: ctlRun
        engine: "executable"
        onNewData: function(source, d) { disconnectSource(source) }
    }

    ColumnLayout {
        anchors.centerIn: parent
        width: parent.width - Kirigami.Units.largeSpacing * 2
        spacing: Kirigami.Units.smallSpacing

        Kirigami.Icon {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: Kirigami.Units.iconSizes.large
            Layout.preferredHeight: Kirigami.Units.iconSizes.large
            source: overlay.paused ? "media-playback-pause" : "network-disconnect"
            opacity: 0.8
        }
        PlasmaComponents.Label {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            font.weight: Font.DemiBold
            text: overlay.paused ? i18n("Paused") : i18n("Router not connected")
        }
        QQC2.Button {
            Layout.alignment: Qt.AlignHCenter
            visible: overlay.paused
            icon.name: "media-playback-start"
            text: i18n("Resume")
            onClicked: ctlRun.connectSource(overlay.ctl + " pause off")
        }
    }
}
