import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import Qt5Compat.GraphicalEffects
import org.kde.kirigami as Kirigami
import org.kde.plasma.plasmoid
import org.kde.plasma.components as PlasmaComponents
import "lib/Highlight.js" as Highlight

Rectangle {
    id: row

    required property string steamid
    required property bool fav

    readonly property var f: root.friendsById[steamid] || ({})
    readonly property bool dim: !f.ingame && f.state === 0
    readonly property color presence: root.stateColor(f)
    readonly property string query: root.searchText.trim()
    readonly property int avatarSize: Plasmoid.configuration.avatarSize
    readonly property bool hot: rowHover.hovered

    height: Math.max(Kirigami.Units.gridUnit * 2.4, avatarSize + Kirigami.Units.smallSpacing * 3)
    radius: Kirigami.Units.cornerRadius * 2
    color: hot ? Qt.alpha(Kirigami.Theme.highlightColor, 0.14) : f.ingame ? Qt.alpha(root.cInGame, 0.06) : "transparent"
    border.width: hot ? 1 : 0
    border.color: Qt.alpha(Kirigami.Theme.highlightColor, 0.35)

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Kirigami.Units.smallSpacing * 1.5
        anchors.rightMargin: Kirigami.Units.smallSpacing * 1.5
        spacing: Kirigami.Units.largeSpacing

        Item {
            Layout.preferredWidth: row.avatarSize
            Layout.preferredHeight: row.avatarSize

            Rectangle {
                id: frame
                anchors.fill: parent
                radius: Kirigami.Units.cornerRadius * 1.5
                color: Qt.alpha(Kirigami.Theme.textColor, 0.08)
                border.width: 2
                border.color: row.presence
                opacity: row.dim ? 0.55 : 1
            }
            Image {
                id: avatar
                anchors.fill: parent
                anchors.margins: 3
                source: row.f.avatar || ""
                sourceSize.width: row.avatarSize * 2
                sourceSize.height: row.avatarSize * 2
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
                cache: true
                visible: false
            }
            Rectangle {
                id: avatarMask
                anchors.fill: avatar
                radius: Kirigami.Units.cornerRadius
                visible: false
            }
            OpacityMask {
                anchors.fill: avatar
                source: avatar
                maskSource: avatarMask
                opacity: row.dim ? 0.45 : 1
            }
            Kirigami.Icon {
                anchors.fill: parent
                anchors.margins: 4
                visible: avatar.status !== Image.Ready
                source: "im-user"
                opacity: row.dim ? 0.45 : 0.8
            }
            Rectangle {
                width: Math.round(row.avatarSize * 0.3)
                height: width
                radius: width / 2
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: -2
                color: row.presence
                border.width: 2
                border.color: Kirigami.Theme.backgroundColor
                visible: !row.dim
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 0

            RowLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing / 2
                Kirigami.Icon {
                    visible: row.fav
                    source: "starred-symbolic"
                    color: "#f0b400"
                    Layout.preferredWidth: Kirigami.Units.iconSizes.small
                    Layout.preferredHeight: Kirigami.Units.iconSizes.small
                }
                PlasmaComponents.Label {
                    Layout.fillWidth: true
                    text: row.query !== "" ? Highlight.mark(row.f.name || "", row.query, Kirigami.Theme.highlightColor) : row.f.name || ""
                    textFormat: row.query !== "" ? Text.StyledText : Text.PlainText
                    elide: Text.ElideRight
                    font.weight: Font.DemiBold
                    opacity: row.dim ? 0.6 : 1
                }
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing
                Kirigami.Icon {
                    visible: !!row.f.ingame
                    source: "input-gamepad"
                    color: root.cInGame
                    Layout.preferredWidth: Kirigami.Units.iconSizes.small * 0.85
                    Layout.preferredHeight: Layout.preferredWidth
                }
                PlasmaComponents.Label {
                    Layout.fillWidth: true
                    text: row.query !== "" && row.f.ingame ? Highlight.mark(root.stateText(row.f), row.query, Kirigami.Theme.highlightColor) : root.stateText(row.f)
                    textFormat: row.query !== "" && row.f.ingame ? Text.StyledText : Text.PlainText
                    elide: Text.ElideRight
                    font: Kirigami.Theme.smallFont
                    color: row.dim ? Kirigami.Theme.textColor : row.presence
                    opacity: row.dim ? 0.5 : 0.95
                }
            }
        }

        Item {
            Layout.preferredWidth: Math.max(side.implicitWidth, root.rowActionButtonWidth * (row.f.join ? 4 : 3))
            Layout.fillHeight: true

            RowLayout {
                id: side
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                spacing: Kirigami.Units.smallSpacing
                opacity: row.hot ? 0 : 1

                Rectangle {
                    visible: !!(row.f.ingame && row.f.capsule) && capsuleImage.status !== Image.Error
                    Layout.preferredHeight: Math.round(row.avatarSize * 0.7)
                    Layout.preferredWidth: Math.round(Layout.preferredHeight * 184 / 69)
                    radius: Kirigami.Units.cornerRadius
                    color: Qt.alpha(Kirigami.Theme.textColor, 0.08)
                    clip: true
                    Image {
                        id: capsuleImage
                        anchors.fill: parent
                        source: row.f.ingame ? row.f.capsule || "" : ""
                        fillMode: Image.PreserveAspectCrop
                        asynchronous: true
                        cache: true
                    }
                }
                ColumnLayout {
                    readonly property string flag: root.flagEmoji(row.f.country)
                    readonly property string ago: root.lastOnlineText(row.f)
                    visible: !row.f.ingame && (flag !== "" || ago !== "")
                    spacing: 0
                    PlasmaComponents.Label {
                        Layout.alignment: Qt.AlignRight
                        visible: parent.flag !== ""
                        text: parent.flag
                        font.family: "Noto Color Emoji"
                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                    }
                    PlasmaComponents.Label {
                        Layout.alignment: Qt.AlignRight
                        visible: parent.ago !== ""
                        text: parent.ago
                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                        font.features: { "tnum": 1 }
                        opacity: 0.5
                    }
                }
            }

            Loader {
                id: actions
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                active: row.hot
                sourceComponent: actionsComponent
            }
        }
    }

    Component {
        id: actionsComponent
        RowLayout {
            spacing: 0
            PlasmaComponents.ToolButton {
                icon.name: "mail-message"
                display: PlasmaComponents.AbstractButton.IconOnly
                text: i18n("Open Chat")
                onClicked: root.openChat(row.f)
                QQC2.ToolTip.visible: hovered
                QQC2.ToolTip.text: text
            }
            PlasmaComponents.ToolButton {
                visible: !!row.f.join
                icon.name: "media-playback-start"
                icon.color: root.cInGame
                display: PlasmaComponents.AbstractButton.IconOnly
                text: i18n("Join Game")
                onClicked: root.steamRun(row.f.join)
                QQC2.ToolTip.visible: hovered
                QQC2.ToolTip.text: text
            }
            PlasmaComponents.ToolButton {
                icon.name: row.fav ? "starred-symbolic" : "non-starred-symbolic"
                icon.color: row.fav ? "#f0b400" : Kirigami.Theme.textColor
                display: PlasmaComponents.AbstractButton.IconOnly
                text: row.fav ? i18n("Remove from Favourites") : i18n("Add to Favourites")
                onClicked: root.toggleFavorite(row.steamid)
                QQC2.ToolTip.visible: hovered
                QQC2.ToolTip.text: text
            }
            PlasmaComponents.ToolButton {
                icon.name: "overflow-menu"
                display: PlasmaComponents.AbstractButton.IconOnly
                text: i18n("More")
                onClicked: root.menuRequested(row.f)
                QQC2.ToolTip.visible: hovered
                QQC2.ToolTip.text: text
            }
        }
    }

    HoverHandler { id: rowHover }
    TapHandler {
        acceptedButtons: Qt.LeftButton
        onTapped: root.openChat(row.f)
    }
    TapHandler {
        acceptedButtons: Qt.RightButton
        onTapped: root.menuRequested(row.f)
    }
}
