import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import "../components"
import org.kde.konveyor.settings

ColumnLayout {
    id: section

    readonly property var pins: TaskbarSettings.values.launchers || []

    function desktopId(url) {
        return String(url).replace(/^applications:/, "").replace(/\.desktop$/, "").replace(/^file:\/\/.*\//, "");
    }

    function urlFor(appId) {
        const id = String(appId || "").trim();
        return !id ? "" : /^[a-z]+:/.test(id) ? id : "applications:" + (id.endsWith(".desktop") ? id : id + ".desktop");
    }

    function setPins(next) {
        TaskbarSettings.values.launchers = next;
    }

    function add(appId) {
        const url = urlFor(appId);
        if (url && pins.indexOf(url) < 0) {
            setPins(pins.concat([url]));
        }
    }

    function move(index, delta) {
        const next = pins.slice();
        const target = index + delta;
        if (target < 0 || target >= next.length) {
            return;
        }
        next.splice(target, 0, next.splice(index, 1)[0]);
        setPins(next);
    }

    Layout.fillWidth: true
    spacing: 0

    CardHeader {
        title: "Pinned apps"
    }

    Card {
        TaskbarSwitchRow {
            key: "placePinnedLaunches"
            label: "Open pinned apps in pin order"
            description: "A pinned app's first window opens next to its pinned neighbours, so the columns keep the order of your pins."
            iconName: "window-pin"
        }

        SettingRow {
            label: "Pins"
            description: section.pins.length ? "Use the arrows to reorder them. Right-clicking an app on the taskbar pins it too." : "Nothing pinned yet. Add an app below, or right-click one on the taskbar."
            iconName: "view-list-icons"
            taskbarKeys: ["launchers"]
            wideControl: true

            ColumnLayout {
                width: parent.width
                spacing: 0

                Repeater {
                    id: rows
                    model: section.pins

                    RowLayout {
                        id: pin
                        required property string modelData
                        required property int index
                        readonly property string appId: section.desktopId(modelData)
                        Layout.fillWidth: true

                        Kirigami.Icon {
                            source: SettingsStore.live.iconFor(pin.appId)
                            fallback: "application-x-executable"
                            Layout.preferredWidth: Kirigami.Units.iconSizes.smallMedium
                            Layout.preferredHeight: Kirigami.Units.iconSizes.smallMedium
                        }

                        QQC2.Label {
                            text: SettingsStore.live.nameFor(pin.appId)
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }

                        QQC2.ToolButton {
                            icon.name: "go-up"
                            text: "Move up"
                            display: QQC2.AbstractButton.IconOnly
                            enabled: pin.index > 0
                            onClicked: section.move(pin.index, -1)
                        }

                        QQC2.ToolButton {
                            icon.name: "go-down"
                            text: "Move down"
                            display: QQC2.AbstractButton.IconOnly
                            enabled: pin.index < section.pins.length - 1
                            onClicked: section.move(pin.index, 1)
                        }

                        QQC2.ToolButton {
                            icon.name: "list-remove"
                            text: "Unpin"
                            display: QQC2.AbstractButton.IconOnly
                            onClicked: section.setPins(section.pins.filter((each, index) => index !== pin.index))
                        }
                    }
                }

                AppPicker {
                    id: picker
                    placeholder: "Pin an app…"
                    Layout.topMargin: Kirigami.Units.smallSpacing
                    onPicked: appId => {
                        section.add(appId);
                        picker.appId = "";
                    }
                }
            }
        }
    }
}
