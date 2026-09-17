import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import "../previews"
import org.kde.konveyor.settings

Card {
    id: card

    property string name
    property string title
    property string description
    property string iconName
    readonly property string base: "animations/" + name
    readonly property var params: SettingsStore.values[base] || ({ enabled: true, kind: "easing", "duration-ms": 250, curve: "ease-out-cubic", bezier: [0, 0, 1, 1] })
    readonly property bool isSpring: params.kind === "spring"
    readonly property bool masterOn: SettingsStore.values["animations/enabled"] === true
    readonly property real slowdown: SettingsStore.values["animations/slowdown"] || 1
    property bool showMore: false

    readonly property var springDefaults: ({ "damping-ratio": 1, stiffness: 800, epsilon: 0.0001 })
    readonly property var curveOptions: [
        { value: "linear", title: "Linear", description: "Constant speed" },
        { value: "ease-out-quad", title: "Gentle", description: "Soft slow-down" },
        { value: "ease-out-cubic", title: "Smooth", description: "Natural slow-down" },
        { value: "ease-out-expo", title: "Snappy", description: "Fast start, long glide" },
        { value: "cubic-bezier", title: "Custom", description: "Drag your own curve" }
    ]

    function writeSpring(changes) {
        const current = isSpring ? params : springDefaults;
        const next = Object.assign({ "damping-ratio": current["damping-ratio"], stiffness: current.stiffness, epsilon: current.epsilon }, changes);
        SettingsStore.remove(base + "/duration-ms");
        SettingsStore.remove(base + "/curve");
        SettingsStore.setValue(base + "/spring", [], {
            "damping-ratio": Math.round(next["damping-ratio"] * 100) / 100,
            stiffness: Math.max(1, Math.round(next.stiffness)),
            epsilon: next.epsilon
        });
    }

    function writeCurve(curve, bezier) {
        SettingsStore.remove(base + "/spring");
        if (!SettingsStore.has(base + "/duration-ms")) {
            SettingsStore.setValue(base + "/duration-ms", [Math.round(isSpring ? 250 : params["duration-ms"])]);
        }
        SettingsStore.setValue(base + "/curve", curve === "cubic-bezier" ? ["cubic-bezier"].concat(bezier) : [curve]);
    }

    function dampingText(ratio) {
        if (ratio < 0.95) {
            return "Overshoots a little, then settles";
        }
        if (ratio <= 1.05) {
            return "Settles quickly without overshooting";
        }
        return "Eases in slowly and softly";
    }

    SettingRow {
        label: card.title
        description: card.description
        iconName: card.iconName
        resetPaths: [card.base]

        QQC2.Switch {
            checked: card.params.enabled
            onToggled: SettingsStore.setFlag(card.base + "/off", !checked)
            Accessible.name: card.title
        }
    }

    ColumnLayout {
        visible: card.params.enabled
        Layout.fillWidth: true
        Layout.leftMargin: Kirigami.Units.largeSpacing * 2
        Layout.rightMargin: Kirigami.Units.largeSpacing * 2
        Layout.bottomMargin: Kirigami.Units.largeSpacing
        spacing: Kirigami.Units.largeSpacing

        RowLayout {
            Layout.fillWidth: true
            spacing: Kirigami.Units.largeSpacing * 2

            CurveGraph {
                params: card.params
                playhead: demo.playhead
                Layout.fillWidth: true
                Layout.preferredHeight: Kirigami.Units.gridUnit * 7
            }

            ColumnLayout {
                Layout.alignment: Qt.AlignTop
                spacing: Kirigami.Units.smallSpacing

                Segmented {
                    currentValue: card.isSpring ? "spring" : "easing"
                    options: [
                        { value: "spring", label: "Spring", icon: "games-config-custom", tooltip: "Physics-based motion that follows your hand" },
                        { value: "easing", label: "Curve", icon: "draw-bezier-curves", tooltip: "A fixed duration with an easing curve" }
                    ]
                    onChosen: value => value === "spring" ? card.writeSpring({}) : card.writeCurve("ease-out-cubic", [0.25, 0.1, 0.25, 1])
                }

                QQC2.Label {
                    text: card.isSpring ? card.dampingText(card.params["damping-ratio"]) : Math.round(card.params["duration-ms"]) + " ms, " + (card.curveOptions.find(option => option.value === card.params.curve) || { title: "" }).title.toLowerCase()
                    wrapMode: Text.Wrap
                    opacity: 0.7
                    font: Kirigami.Theme.smallFont
                    Layout.maximumWidth: Kirigami.Units.gridUnit * 12
                }
            }
        }

        AnimationDemo {
            id: demo
            params: card.params
            slowdown: card.slowdown
            animationsOn: card.masterOn
            Layout.fillWidth: true
        }
    }

    SliderRow {
        visible: card.params.enabled && card.isSpring
        label: "Bounciness"
        description: "Lower values overshoot and wobble, higher values glide in without bouncing."
        resetPaths: [card.base + "/spring"]
        from: 0.1
        to: 10
        stepSize: 0.05
        decimals: 2
        lowLabel: "Bouncy"
        highLabel: "Firm"
        value: card.params["damping-ratio"] || 1
        onEdited: value => card.writeSpring({ "damping-ratio": value })
    }

    SliderRow {
        visible: card.params.enabled && card.isSpring
        label: "Stiffness"
        description: "How strongly the window is pulled into place. Stiffer springs finish faster."
        resetPaths: [card.base + "/spring"]
        from: 50
        to: 3000
        stepSize: 50
        lowLabel: "Loose"
        highLabel: "Tight"
        value: card.params.stiffness || 800
        onEdited: value => card.writeSpring({ stiffness: value })
    }

    SliderRow {
        visible: card.params.enabled && !card.isSpring
        label: "Duration"
        description: "How long the animation takes before the slowdown factor is applied."
        resetPaths: [card.base + "/duration-ms"]
        from: 0
        to: 2000
        stepSize: 10
        unit: "ms"
        lowLabel: "Quick"
        highLabel: "Slow"
        value: card.params["duration-ms"] || 250
        onEdited: value => SettingsStore.setValue(card.base + "/duration-ms", [Math.round(value)])
    }

    SettingRow {
        visible: card.params.enabled && !card.isSpring
        label: "Curve"
        description: "The shape of the motion from start to finish."
        resetPaths: [card.base + "/curve"]
        wideControl: true

        ChoiceCards {
            width: parent.width
            cardHeight: Kirigami.Units.gridUnit * 5.5
            currentValue: card.params.curve
            options: card.curveOptions.map(option => Object.assign({ preview: curveThumb }, option))
            onChosen: value => card.writeCurve(value, card.params.bezier || [0.25, 0.1, 0.25, 1])
        }
    }

    SettingRow {
        visible: card.params.enabled && !card.isSpring && card.params.curve === "cubic-bezier"
        label: "Custom curve"
        description: "Drag the two handles. Pulling a handle above the top line makes the motion overshoot before it settles."
        resetPaths: [card.base + "/curve"]

        BezierEditor {
            bezier: card.params.bezier || [0.25, 0.1, 0.25, 1]
            onEdited: bezier => card.writeCurve("cubic-bezier", bezier)
        }
    }

    SettingRow {
        visible: card.params.enabled && card.isSpring && card.showMore
        label: "Finish precision"
        description: "How close to its target the motion must get before it stops."
        resetPaths: [card.base + "/spring"]

        Segmented {
            currentValue: card.params.epsilon
            options: [
                { value: 0.01, label: "Rough" },
                { value: 0.001, label: "Quick" },
                { value: 0.0001, label: "Normal" },
                { value: 0.00001, label: "Precise" }
            ]
            onChosen: value => card.writeSpring({ epsilon: value })
        }
    }

    QQC2.ToolButton {
        visible: card.params.enabled && card.isSpring
        Layout.alignment: Qt.AlignRight
        Layout.margins: Kirigami.Units.smallSpacing
        text: card.showMore ? "Fewer options" : "More options"
        icon.name: card.showMore ? "arrow-up" : "arrow-down"
        onClicked: card.showMore = !card.showMore
    }

    Component {
        id: curveThumb

        CurveGraph {
            compact: true
            lineColor: parent && parent.selected ? Kirigami.Theme.highlightColor : Kirigami.Theme.textColor
            params: ({ kind: "easing", "duration-ms": 300, curve: parent ? parent.choice.value : "linear", bezier: card.params.bezier || [0.25, 0.1, 0.25, 1] })
        }
    }
}
