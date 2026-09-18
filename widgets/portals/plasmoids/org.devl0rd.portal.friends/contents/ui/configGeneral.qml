import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.FormLayout {
    property alias cfg_icon: iconField.text
    property alias cfg_avatarSize: sizeSpin.value
    property alias cfg_showCountBadge: badge.checked
    property alias cfg_panelShrink: panelShrink.checked
    property string cfg_panelDetail
    property alias cfg_showPlayingNow: playingNow.checked
    property alias cfg_hideOffline: hideOffline.checked
    property string cfg_sortMode
    property string cfg_favorites
    property string cfg_currentTab

    QQC2.TextField {
        id: iconField
        Kirigami.FormData.label: i18n("Panel icon:")
        placeholderText: i18n("icon name")
    }
    QQC2.CheckBox {
        id: badge
        Kirigami.FormData.label: i18n("Panel:")
        text: i18n("Show how many friends are online")
    }
    QQC2.CheckBox { id: panelShrink; text: i18n("Shrink to fit the panel") }
    QQC2.ComboBox {
        Kirigami.FormData.label: i18n("Detail level:")
        textRole: "text"
        valueRole: "value"
        model: [
            { text: i18n("Automatic"), value: "auto" },
            { text: i18n("Icon and count"), value: "medium" },
            { text: i18n("Count only"), value: "small" },
            { text: i18n("Icon only"), value: "tiny" }
        ]
        Component.onCompleted: currentIndex = Math.max(0, indexOfValue(cfg_panelDetail))
        onActivated: cfg_panelDetail = currentValue
    }

    Item { Kirigami.FormData.isSection: true }

    RowLayout {
        Kirigami.FormData.label: i18n("Avatar size:")
        QQC2.SpinBox { id: sizeSpin; from: 24; to: 96; stepSize: 4 }
        QQC2.Label { text: i18n("px"); opacity: 0.6 }
    }
    QQC2.CheckBox {
        id: playingNow
        Kirigami.FormData.label: i18n("Show:")
        text: i18n("Who's playing, grouped by game")
    }
    QQC2.CheckBox {
        id: hideOffline
        text: i18n("Hide offline friends")
    }
}
