import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import "../catalog/Kdl.js" as Kdl
import "../previews"
import org.kde.konveyor.settings

GridLayout {
    id: editor

    property string blockPath
    property string colorName
    property string gradientName
    property var paint
    property bool allowAuto: false
    property string autoLabel: "Auto"
    property var autoPaint: null
    property var extraModes: []
    property string extraMode
    property var resetPaths: []
    property bool stacked: false
    default property alias slotContent: slot.data
    readonly property string mode: extraMode.length > 0 ? extraMode : Kdl.paintMode(paint)
    readonly property var gradient: paint && paint.gradient ? paint.gradient : ({ from: "#ff7f00ff", to: "#ff00b4ff", angle: 180, "relative-to": "window", "in": "srgb" })
    readonly property bool modified: SettingsStore.revision >= 0 && resetPaths.some(path => !SettingsStore.isDefault(path))
    signal extraChosen(string value)
    signal plainChosen()

    function writeColor(text) {
        SettingsStore.remove(blockPath + "/" + gradientName);
        SettingsStore.setValue(blockPath + "/" + colorName, [text]);
    }

    function writeGradient(changes) {
        SettingsStore.remove(blockPath + "/" + colorName);
        SettingsStore.setValue(blockPath + "/" + gradientName, [], Kdl.gradientProps(Object.assign({}, gradient, changes)));
    }

    function choosePlain(value) {
        const source = paint && paint.source;
        if (value === "none") {
            SettingsStore.remove(blockPath + "/" + colorName);
            SettingsStore.remove(blockPath + "/" + gradientName);
        } else if (value === "theme") {
            writeColor(source && source !== "color" ? source : "accent");
        } else if (value === "color") {
            writeColor(Kdl.cssColor(Qt.color(paint && paint.color ? paint.color : gradient.from)));
        } else {
            writeGradient({});
        }
    }

    columns: stacked ? 2 : 3
    columnSpacing: Kirigami.Units.smallSpacing
    rowSpacing: Kirigami.Units.smallSpacing

    Segmented {
        Layout.columnSpan: editor.stacked ? 2 : 1
        compact: true
        currentValue: editor.mode
        options: (editor.allowAuto ? [{ value: "none", label: editor.autoLabel }] : []).concat([
            { value: "theme", label: "Theme", tooltip: "Follow a color from your Plasma color scheme" },
            { value: "color", label: "Color" },
            { value: "gradient", label: "Gradient" }
        ], editor.extraModes)
        onChosen: value => {
            if (editor.extraModes.some(option => option.value === value)) {
                editor.extraChosen(value);
                return;
            }
            editor.plainChosen();
            editor.choosePlain(value);
        }
    }

    RowLayout {
        id: slot
        Layout.minimumWidth: 190
        Layout.maximumWidth: 190
        spacing: Kirigami.Units.smallSpacing

        QQC2.ComboBox {
            visible: editor.mode === "theme"
            Layout.fillWidth: true
            model: Kdl.themeColors
            textRole: "label"
            valueRole: "value"
            currentIndex: Math.max(0, Kdl.themeColors.findIndex(entry => editor.paint && entry.value === editor.paint.source))
            onActivated: editor.writeColor(currentValue)
        }

        ColorField {
            visible: editor.mode === "color"
            value: editor.paint && editor.paint.color ? editor.paint.color : ""
            onEdited: css => editor.writeColor(css)
        }

        QQC2.Button {
            id: gradientButton
            visible: editor.mode === "gradient"
            Layout.fillWidth: true
            text: "Edit gradient"
            onClicked: options.open()

            contentItem: Rectangle {
                implicitHeight: Kirigami.Units.gridUnit
                radius: Kirigami.Units.cornerRadius
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0; color: editor.gradient.from }
                    GradientStop { position: 1; color: editor.gradient.to }
                }
            }

            QQC2.ToolTip.text: text
            QQC2.ToolTip.visible: hovered
            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay

            GradientOptions {
                id: options
                gradient: editor.gradient
                onEdited: changes => editor.writeGradient(changes)
            }
        }

        PaintFrame {
            visible: editor.mode === "none" && editor.autoPaint !== null && editor.autoPaint !== undefined
            Layout.preferredWidth: 46
            Layout.preferredHeight: Kirigami.Units.gridUnit * 1.4
            filled: true
            radius: Kirigami.Units.cornerRadius
            paint: editor.autoPaint
        }
    }

    Item {
        visible: editor.resetPaths.length > 0
        Layout.preferredWidth: 26
        Layout.preferredHeight: 26

        ModifiedBadge {
            anchors.centerIn: parent
            visible: editor.modified
            onResetRequested: editor.resetPaths.forEach(path => SettingsStore.resetToDefault(path))
        }
    }
}
