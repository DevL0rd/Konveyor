import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.kcmutils as KCM
import "catalog/Pages.js" as Pages
import "catalog/SettingsIndex.js" as SettingsIndex
import "components"
import "previews"

KCM.SimpleKCM {
    id: hub

    readonly property var layoutValues: kcm.revision >= 0 ? kcm.scope("layout") : ({})
    readonly property var values: kcm.values
    property string query

    function openPage(id) {
        const page = Pages.byId(id);
        while (kcm.depth > 1) {
            kcm.pop();
        }
        kcm.push(page.file);
    }

    function summary(id) {
        const count = (parent, name) => kcm.revision >= 0 ? kcm.children(parent, name).length : 0;
        const plural = (n, word) => n + " " + word + (n === 1 ? "" : "s");
        switch (id) {
        case "layout":
            return Math.round(layoutValues.gaps || 0) + " px gaps · new columns open " + layoutValues["new-column-position"];
        case "look": {
            const ring = layoutValues["focus-ring"] || {};
            return (ring.enabled ? "Focus ring " + Math.round(ring.width) + " px" : "No focus ring") + " · border " + ((layoutValues.border || {}).enabled ? "on" : "off");
        }
        case "motion":
            return values["animations/enabled"] ? "Animations on · " + values["animations/slowdown"] + "× speed" : "Animations off";
        case "mouse":
            return (values["input/focus-follows-mouse"] ? "Focus follows mouse" : "Click to focus") + " · title bar drag " + (values["gestures/titlebar-drag"] === "move-window" ? "moves windows" : "scrolls");
        case "shortcuts":
            return plural(kcm.revision >= 0 ? (kcm.node("binds").children || []).length : 0, "shortcut");
        case "rules":
            return plural(count("", "window-rule"), "rule");
        case "monitors":
            return plural(count("", "monitor-profile"), "profile") + " · " + plural(kcm.live.outputs.length, "monitor") + " connected";
        case "workspaces":
            return plural(count("", "workspace"), "named workspace");
        case "plasma":
            return [values["hide-desktop-widgets"] ? "widgets hide" : "", values["fill-panels-on-maximize"] ? "panels stretch" : "", values["disable-minimize"] ? "no minimize" : ""].filter(Boolean).join(" · ") || "Plasma defaults";
        }
        return "";
    }

    Component.onCompleted: {
        if (kcm.initialPage.length > 0 && Pages.byId(kcm.initialPage)) {
            Qt.callLater(() => openPage(kcm.initialPage));
        }
    }

    topPadding: 0
    leftPadding: 0
    rightPadding: 0

    header: ColumnLayout {
        spacing: 0

        StatusMessages {
            Layout.fillWidth: true
        }

        HeroBanner {
            Layout.fillWidth: true
        }
    }

    ColumnLayout {
        width: hub.width
        spacing: Kirigami.Units.largeSpacing

        Kirigami.SearchField {
            id: search
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: Kirigami.Units.largeSpacing * 2
            Layout.preferredWidth: Math.min(hub.width - Kirigami.Units.gridUnit * 2, Kirigami.Units.gridUnit * 30)
            placeholderText: "Search every setting…"
            onTextChanged: hub.query = text
        }

        SearchResults {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: Math.min(hub.width - Kirigami.Units.gridUnit * 2, Kirigami.Units.gridUnit * 46)
            visible: hub.query.length > 0
            results: SettingsIndex.search(hub.query)
            onOpened: id => hub.openPage(id)
        }

        GridLayout {
            id: grid
            visible: hub.query.length === 0
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: Math.min(hub.width - Kirigami.Units.gridUnit * 2, Kirigami.Units.gridUnit * 60)
            Layout.bottomMargin: Kirigami.Units.gridUnit
            columns: Math.max(1, Math.floor(width / (Kirigami.Units.gridUnit * 17)))
            columnSpacing: Kirigami.Units.largeSpacing
            rowSpacing: Kirigami.Units.largeSpacing

            Repeater {
                model: Pages.pages

                CategoryTile {
                    required property var modelData
                    Layout.fillWidth: true
                    title: modelData.title
                    iconName: modelData.icon
                    description: modelData.description
                    summary: hub.summary(modelData.id)
                    onClicked: hub.openPage(modelData.id)
                }
            }
        }
    }
}
