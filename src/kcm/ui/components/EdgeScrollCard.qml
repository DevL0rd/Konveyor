import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Card {
    id: card

    property string path
    property string triggerName
    property var values: ({})
    property string label
    property string description
    property string triggerLabel
    property string iconName
    readonly property bool hovered: hover.hovered

    HoverHandler {
        id: hover
    }

    SettingRow {
        label: card.label
        description: card.description
        iconName: card.iconName
        resetPaths: [card.path]
    }

    SliderRow {
        label: card.triggerLabel
        description: "How close to the edge the pointer has to be. Set to 0 to turn this off."
        resetPaths: [card.path + "/" + card.triggerName]
        from: 0
        to: 300
        stepSize: 5
        unit: "px"
        lowLabel: "Off"
        highLabel: "Wide"
        value: card.values.trigger || 0
        onEdited: value => kcm.setValue(card.path + "/" + card.triggerName, [Math.round(value)])
    }

    SliderRow {
        label: "Wait before scrolling"
        description: "Keeps quick passes over the edge from scrolling by accident."
        resetPaths: [card.path + "/delay-ms"]
        from: 0
        to: 2000
        stepSize: 10
        unit: "ms"
        lowLabel: "Instant"
        highLabel: "Patient"
        value: card.values["delay-ms"] || 0
        onEdited: value => kcm.setValue(card.path + "/delay-ms", [Math.round(value)])
    }

    SliderRow {
        label: "Top speed"
        description: "How fast it scrolls with the pointer pressed all the way into the edge."
        resetPaths: [card.path + "/max-speed"]
        from: 100
        to: 10000
        stepSize: 100
        unit: "px/s"
        lowLabel: "Slow"
        highLabel: "Fast"
        value: card.values["max-speed"] || 0
        onEdited: value => kcm.setValue(card.path + "/max-speed", [Math.round(value)])
    }
}
