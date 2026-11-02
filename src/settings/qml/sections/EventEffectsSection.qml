import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import "../components"
import "../catalog/BorderStyles.js" as BorderStyles
import "../catalog/EventEffects.js" as EventEffects
import org.kde.konveyor.settings

ColumnLayout {
    id: root

    property string scopePath: "layout"
    property bool overrideMode: false
    property var excluded: []
    readonly property var values: SettingsStore.revision >= 0 ? SettingsStore.scope(scopePath) : ({})
    readonly property bool customized: SettingsStore.revision >= 0 && SettingsStore.has(EventEffects.blockPath(scopePath))
    property var hoverTry: null
    property var replay: null
    property string choosingEvent
    readonly property var tryOn: hoverTry || replay

    function playOf(view, reach, prefix) {
        return {
            label: view.style,
            caption: prefix + view.style + " " + view.info.phrase,
            layers: view.layers,
            reach: reach,
            duration: view.value.duration || 0,
            event: view.event
        };
    }

    function tried(view, reach, on) {
        if (on && view.active) {
            hoverTry = playOf(view, reach, "Trying: ");
        } else if (hoverTry && hoverTry.event === view.event) {
            hoverTry = null;
        }
    }

    function replayed(view) {
        replay = view.active ? playOf(view, view.value.reach, "") : null;
        replayEnd.restart();
    }

    spacing: 0
    visible: !excluded.includes("event-effects")

    Timer {
        id: replayEnd
        interval: 3000
        onTriggered: root.replay = null
    }

    StyleChooser {
        id: chooser
        onChosen: name => {
            const view = rows.itemAt(EventEffects.events.findIndex(info => info.value === root.choosingEvent));
            view.update({ style: name });
            root.replayed(view);
        }
    }

    CardHeader {
        title: "Event effects"
    }

    Card {
        QQC2.Label {
            text: "Play any style when something happens, on one window or across several. " + (root.overrideMode ? "" : "Hover one to try it above.")
            wrapMode: Text.Wrap
            opacity: 0.75
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.largeSpacing
            Layout.bottomMargin: 0
        }

        ScopedRow {
            visible: root.overrideMode
            scope: root
            key: "event-effects"
            label: "Event effects here"
            description: "Use different effects just for this one"
            iconName: "preferences-desktop-effects"
            inheritedSummary: EventEffects.summary(root.values["event-effects"])
        }

        Repeater {
            id: rows
            model: EventEffects.events

            EventEffectRow {
                id: rowView
                required property var modelData
                visible: !root.overrideMode || root.customized
                Layout.fillWidth: true
                info: modelData
                scope: root
                onChooseRequested: {
                    root.choosingEvent = rowView.event;
                    chooser.ask("", "Event effects · " + rowView.info.label);
                }
                onTried: (reach, on) => root.tried(rowView, reach, on)
                onReplayed: root.replayed(rowView)
            }
        }
    }
}
