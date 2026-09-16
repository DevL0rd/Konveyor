import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

QQC2.Button {
    id: picker

    property string appId
    property string placeholder: "Choose an app…"
    signal picked(string appId)

    readonly property var running: {
        const seen = new Set();
        const list = [];
        for (const window of kcm.live.windows) {
            if (!window.app_id || seen.has(window.app_id)) {
                continue;
            }
            seen.add(window.app_id);
            list.push({ appId: window.app_id, name: kcm.live.nameFor(window.app_id), detail: window.title, icon: kcm.live.iconFor(window.app_id), group: "Open now" });
        }
        return list;
    }

    function entries(query) {
        const lower = query.toLowerCase();
        const seen = new Set(running.map(entry => entry.appId));
        const installed = kcm.live.applications
            .filter(app => !seen.has(app.appId))
            .map(app => ({ appId: app.appId, name: app.name, detail: app.appId, icon: app.icon, group: "Installed apps" }));
        return running.concat(installed).filter(entry => !lower.length
            || entry.name.toLowerCase().includes(lower) || entry.appId.toLowerCase().includes(lower));
    }

    icon.name: appId.length ? kcm.live.iconFor(appId) : "application-x-executable"
    text: appId.length ? kcm.live.nameFor(appId) + (kcm.live.nameFor(appId) === appId ? "" : "  (" + appId + ")") : placeholder
    onClicked: {
        kcm.live.refresh();
        search.text = "";
        popup.open();
        search.forceActiveFocus();
    }

    QQC2.Popup {
        id: popup

        y: picker.height
        width: Math.max(picker.width, Kirigami.Units.gridUnit * 22)
        height: Kirigami.Units.gridUnit * 20
        padding: Kirigami.Units.smallSpacing

        contentItem: ColumnLayout {
            spacing: Kirigami.Units.smallSpacing

            Kirigami.SearchField {
                id: search
                Layout.fillWidth: true
                placeholderText: "Search apps or type an app id…"
                onAccepted: {
                    if (list.count > 0) {
                        picker.picked(list.model[0].appId);
                    } else if (text.trim().length) {
                        picker.picked(text.trim());
                    }
                    popup.close();
                }
            }

            QQC2.ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true

                ListView {
                    id: list
                    clip: true
                    model: popup.visible ? picker.entries(search.text) : []
                    section.property: "group"
                    section.delegate: Kirigami.ListSectionHeader {
                        required property string section
                        width: ListView.view.width
                        text: section
                    }

                    delegate: QQC2.ItemDelegate {
                        id: entry
                        required property var modelData
                        width: ListView.view.width
                        highlighted: modelData.appId === picker.appId
                        onClicked: {
                            picker.picked(modelData.appId);
                            popup.close();
                        }

                        contentItem: RowLayout {
                            spacing: Kirigami.Units.largeSpacing

                            Kirigami.Icon {
                                source: entry.modelData.icon
                                fallback: "application-x-executable"
                                Layout.preferredWidth: Kirigami.Units.iconSizes.medium
                                Layout.preferredHeight: Kirigami.Units.iconSizes.medium
                            }

                            ColumnLayout {
                                spacing: 0
                                Layout.fillWidth: true

                                QQC2.Label {
                                    text: entry.modelData.name
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }

                                QQC2.Label {
                                    text: entry.modelData.detail
                                    elide: Text.ElideRight
                                    opacity: 0.7
                                    font: Kirigami.Theme.smallFont
                                    Layout.fillWidth: true
                                }
                            }
                        }
                    }
                }
            }

            QQC2.Label {
                visible: list.count === 0
                text: search.text.trim().length ? "Press Enter to use “" + search.text.trim() + "” as the app id" : "No apps found"
                opacity: 0.7
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }
        }
    }
}
