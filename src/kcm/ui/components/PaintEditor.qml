import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import "../catalog/Kdl.js" as Kdl

RowLayout {
    id: editor

    property string blockPath
    property string colorName
    property string gradientName
    property var paint
    property bool allowAuto: false
    property string autoLabel: "Auto"
    readonly property string mode: Kdl.paintMode(paint)
    readonly property var gradient: paint && paint.gradient ? paint.gradient : ({ from: "#ff7f00ff", to: "#ff00b4ff", angle: 180, "relative-to": "window", "in": "srgb" })

    function writeColor(text) {
        kcm.remove(blockPath + "/" + gradientName);
        kcm.setValue(blockPath + "/" + colorName, [text]);
    }

    function writeGradient(changes) {
        const next = Object.assign({}, gradient, changes);
        const props = { from: Kdl.cssColor(Qt.color(next.from)), to: Kdl.cssColor(Qt.color(next.to)), angle: Math.round(next.angle) };
        if (next["relative-to"] !== "window") {
            props["relative-to"] = next["relative-to"];
        }
        if (next["in"] !== "srgb") {
            props["in"] = next["in"];
        }
        kcm.setValue(blockPath + "/" + gradientName, [], props);
    }

    spacing: Kirigami.Units.smallSpacing

    Segmented {
        currentValue: editor.mode
        options: (editor.allowAuto ? [{ value: "none", label: editor.autoLabel }] : []).concat([
            { value: "theme", label: "Theme", tooltip: "Follow a color from your Plasma color scheme" },
            { value: "color", label: "Color" },
            { value: "gradient", label: "Gradient" }
        ])
        onChosen: value => {
            if (value === "none") {
                kcm.remove(editor.blockPath + "/" + editor.colorName);
                kcm.remove(editor.blockPath + "/" + editor.gradientName);
            } else if (value === "theme") {
                editor.writeColor("accent");
            } else if (value === "color") {
                editor.writeColor(Kdl.cssColor(Qt.color(editor.gradient.from)));
            } else {
                editor.writeGradient({});
            }
        }
    }

    QQC2.ComboBox {
        visible: editor.mode === "theme"
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

    ColorField {
        visible: editor.mode === "gradient"
        value: editor.gradient.from
        onEdited: css => editor.writeGradient({ from: css })
    }

    Rectangle {
        visible: editor.mode === "gradient"
        implicitWidth: Kirigami.Units.gridUnit * 3
        implicitHeight: Kirigami.Units.gridUnit * 1.4
        radius: Kirigami.Units.cornerRadius
        rotation: 0
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0; color: editor.gradient.from }
            GradientStop { position: 1; color: editor.gradient.to }
        }
    }

    ColorField {
        visible: editor.mode === "gradient"
        value: editor.gradient.to
        onEdited: css => editor.writeGradient({ to: css })
    }

    AngleDial {
        visible: editor.mode === "gradient"
        angle: editor.gradient.angle
        onEdited: value => editor.writeGradient({ angle: value })
    }

    QQC2.ToolButton {
        visible: editor.mode === "gradient"
        icon.name: "configure"
        display: QQC2.AbstractButton.IconOnly
        text: "Gradient options"
        onClicked: options.open()

        QQC2.ToolTip.text: text
        QQC2.ToolTip.visible: hovered

        GradientOptions {
            id: options
            gradient: editor.gradient
            onEdited: changes => editor.writeGradient(changes)
        }
    }
}
