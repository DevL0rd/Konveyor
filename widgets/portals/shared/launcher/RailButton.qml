import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PlasmaComponents

    MouseArea {
        id: railItem
        required property var modelData
        readonly property int index: launcher.pageDefs.findIndex(def => def.key === modelData.key)
        readonly property bool current: !launcher.searching && launcher.page === modelData.key
        readonly property int badge: modelData.key === "friends" ? launcherData.friendsInGame : 0
        Layout.fillWidth: true
        Layout.preferredHeight: Kirigami.Units.gridUnit * (launcher.compact ? 2.7 : 3)
        hoverEnabled: true
        property bool hintDismissed: false
        onClicked: {
            hintDismissed = true
            launcher.goToPage(modelData.key)
        }
        onExited: hintDismissed = false

        Rectangle {
            anchors.fill: parent
            radius: Kirigami.Units.cornerRadius * 2
            color: railItem.current ? launcher.selectedFill : railItem.containsMouse ? launcher.hoverFill : "transparent"
            border.width: railItem.current ? 1 : 0
            border.color: launcher.hairline
        }
        Rectangle {
            visible: railItem.badge > 0
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.margins: Kirigami.Units.smallSpacing * 0.6
            width: Math.max(height, badgeLabel.implicitWidth + Kirigami.Units.smallSpacing * 1.5)
            height: badgeLabel.implicitHeight
            radius: height / 2
            color: Qt.alpha(Kirigami.Theme.positiveTextColor, 0.22)
            border.width: 1
            border.color: Qt.alpha(Kirigami.Theme.positiveTextColor, 0.55)
            PlasmaComponents.Label {
                id: badgeLabel
                anchors.centerIn: parent
                text: railItem.badge
                font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.85
                font.weight: Font.DemiBold
            }
        }
        Rectangle {
            visible: launcher.altHeld && railItem.index < 9
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.margins: Kirigami.Units.smallSpacing * 0.6
            width: Math.max(height, altKey.implicitWidth + Kirigami.Units.smallSpacing * 1.5)
            height: altKey.implicitHeight + 2
            radius: Kirigami.Units.cornerRadius
            color: Qt.alpha(launcher.ink, 0.9)
            PlasmaComponents.Label {
                id: altKey
                anchors.centerIn: parent
                text: railItem.index + 1
                color: Kirigami.Theme.backgroundColor
                font.pointSize: Kirigami.Theme.smallFont.pointSize * 0.85
                font.weight: Font.Bold
            }
        }
        Timer {
            id: hintDelay
            interval: 450
            running: railItem.containsMouse
        }
        Rectangle {
            id: railHint
            readonly property bool wanted: railItem.containsMouse && !hintDelay.running && !railItem.hintDismissed
            visible: opacity > 0
            opacity: wanted ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: 120 } }
            x: parent.width + Kirigami.Units.largeSpacing
            anchors.verticalCenter: parent.verticalCenter
            width: hintRow.implicitWidth + Kirigami.Units.largeSpacing * 1.5
            height: hintRow.implicitHeight + Kirigami.Units.smallSpacing * 2
            radius: height / 2
            color: Qt.rgba(0.08, 0.08, 0.09, 0.96)
            border.width: 1
            border.color: launcher.hairline
            RowLayout {
                id: hintRow
                anchors.centerIn: parent
                spacing: Kirigami.Units.smallSpacing * 1.5
                PlasmaComponents.Label {
                    text: railItem.badge > 0 ? i18np("%1 friend in game", "%1 friends in game", railItem.badge) : railItem.modelData.hint
                    color: "white"
                }
                Rectangle {
                    visible: railItem.index < 9
                    implicitWidth: hintKey.implicitWidth + Kirigami.Units.smallSpacing * 1.5
                    implicitHeight: hintKey.implicitHeight + 2
                    radius: Kirigami.Units.cornerRadius
                    color: Qt.alpha("white", 0.12)
                    border.width: 1
                    border.color: Qt.alpha("white", 0.18)
                    PlasmaComponents.Label {
                        id: hintKey
                        anchors.centerIn: parent
                        text: i18n("Alt %1", railItem.index + 1)
                        color: "white"
                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                        opacity: 0.8
                    }
                }
            }
        }
        ColumnLayout {
            anchors.centerIn: parent
            spacing: Kirigami.Units.smallSpacing * 0.75
            Kirigami.Icon {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: Kirigami.Units.iconSizes.smallMedium
                Layout.preferredHeight: Kirigami.Units.iconSizes.smallMedium
                source: railItem.modelData.icon
                color: launcher.ink
                isMask: true
                opacity: railItem.current ? 1 : 0.62
            }
            PlasmaComponents.Label {
                Layout.alignment: Qt.AlignHCenter
                text: railItem.modelData.label
                font.pointSize: Kirigami.Theme.smallFont.pointSize
                font.weight: railItem.current ? Font.DemiBold : Font.Normal
                opacity: railItem.current ? 1 : 0.62
            }
        }
    }
