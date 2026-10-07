import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import "TaskOrder.js" as TaskOrder

ConfigPage {
    id: page

    readonly property var pins: page.cfg_launchers || []

    function setPins(next) {
        page.cfg_launchers = next
    }

    function addPin(text) {
        const url = TaskOrder.pinUrl(text)
        if (url && pins.indexOf(url) < 0)
            setPins(pins.concat([url]))
    }

    title: i18n("Pinned Apps")

    ConfigCheck {
        id: placePinnedLaunches
        Kirigami.FormData.label: i18n("New windows:")
        page: page
        key: "placePinnedLaunches"
        text: i18n("Open a pinned app's first window in pin order")
    }

    Item {
        Kirigami.FormData.isSection: true
    }

    ColumnLayout {
        id: pinList
        Kirigami.FormData.label: i18n("Pinned apps:")
        Layout.fillWidth: true
        Layout.minimumWidth: Kirigami.Units.gridUnit * 18
        spacing: 0

        QQC2.Label {
            visible: page.pins.length === 0
            text: i18n("Nothing pinned yet. Right-click an app on the taskbar, or add one below.")
            opacity: 0.7
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }

        Repeater {
            id: rows
            model: page.pins

            RowLayout {
                id: row
                required property string modelData
                required property int index
                Layout.fillWidth: true

                Kirigami.Icon {
                    source: TaskOrder.pinLabel(row.modelData)
                    fallback: "application-x-executable"
                    Layout.preferredWidth: Kirigami.Units.iconSizes.smallMedium
                    Layout.preferredHeight: Kirigami.Units.iconSizes.smallMedium
                }

                QQC2.Label {
                    text: TaskOrder.pinLabel(row.modelData)
                    elide: Text.ElideMiddle
                    Layout.fillWidth: true
                }

                QQC2.ToolButton {
                    icon.name: "go-up"
                    text: i18n("Move up")
                    display: QQC2.AbstractButton.IconOnly
                    enabled: row.index > 0
                    onClicked: page.setPins(TaskOrder.movedPin(page.pins, row.index, -1))
                }

                QQC2.ToolButton {
                    icon.name: "go-down"
                    text: i18n("Move down")
                    display: QQC2.AbstractButton.IconOnly
                    enabled: row.index < page.pins.length - 1
                    onClicked: page.setPins(TaskOrder.movedPin(page.pins, row.index, 1))
                }

                QQC2.ToolButton {
                    icon.name: "list-remove"
                    text: i18n("Unpin")
                    display: QQC2.AbstractButton.IconOnly
                    onClicked: page.setPins(page.pins.filter((pin, index) => index !== row.index))
                }
            }
        }
    }

    Loader {
        id: search
        Kirigami.FormData.label: i18n("Add:")
        Layout.fillWidth: true
        source: "PinSearch.qml"
        Connections {
            target: search.item
            ignoreUnknownSignals: true
            function onPicked(url) {
                page.addPin(url)
            }
        }
    }

    RowLayout {
        Kirigami.FormData.label: search.status === Loader.Ready ? i18n("Or by desktop file:") : i18n("Add by desktop file:")
        Layout.fillWidth: true

        QQC2.TextField {
            id: desktopId
            Layout.fillWidth: true
            placeholderText: i18n("org.kde.dolphin.desktop")
            onAccepted: addButton.clicked()
        }

        QQC2.Button {
            id: addButton
            icon.name: "list-add"
            text: i18n("Pin")
            enabled: desktopId.text.trim().length > 0
            onClicked: {
                page.addPin(desktopId.text)
                desktopId.text = ""
            }
        }
    }
}
