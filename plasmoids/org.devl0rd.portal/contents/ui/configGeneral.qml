import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.FormLayout {
    property alias cfg_icon: iconField.text
    property alias cfg_showFriendsBadge: friendsBadge.checked
    property alias cfg_iconSize: iconSizeSpin.value
    property alias cfg_gameCardWidth: cardSpin.value
    property alias cfg_showAppLabels: showLabels.checked
    property alias cfg_showGameTitles: showTitles.checked
    property alias cfg_gamesFriendsOnly: friendsOnly.checked
    property string cfg_defaultCategory
    property string cfg_gamesViewMode
    property string cfg_appViewMode
    property string cfg_defaultSort
    property string cfg_lastAppCategory

    QQC2.TextField {
        id: iconField
        Kirigami.FormData.label: i18n("Panel icon:")
        placeholderText: i18n("icon name")
    }
    QQC2.CheckBox {
        id: friendsBadge
        Kirigami.FormData.label: i18n("Panel:")
        text: i18n("Badge with friends in game")
    }

    Item { Kirigami.FormData.isSection: true }

    RowLayout {
        Kirigami.FormData.label: i18n("App icon size:")
        QQC2.SpinBox { id: iconSizeSpin; from: 32; to: 160; stepSize: 8 }
        QQC2.Label { text: i18n("px"); opacity: 0.6 }
    }
    RowLayout {
        Kirigami.FormData.label: i18n("Game card width:")
        QQC2.SpinBox { id: cardSpin; from: 100; to: 320; stepSize: 10 }
        QQC2.Label { text: i18n("px"); opacity: 0.6 }
    }
    QQC2.CheckBox { id: showLabels; Kirigami.FormData.label: i18n("Show:"); text: i18n("App name labels") }
    QQC2.CheckBox { id: showTitles; text: i18n("Game titles (always, not just on hover)") }
    QQC2.CheckBox { id: friendsOnly; text: i18n("Only games with friends online") }
}
