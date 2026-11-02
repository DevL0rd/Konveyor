import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import "../catalog/Kdl.js" as Kdl

QQC2.Popup {
    id: popup

    property color initial
    property color picked
    property real hue: 0
    property real saturation: 0
    property real brightness: 0
    property real opacityValue: 1
    property point anchorTop
    property real anchorHeight: 0

    signal chosen(color picked)

    function openFor(start, item) {
        initial = start;
        adopt(start);
        showHex();
        anchorTop = item.mapToItem(parent, 0, 0);
        anchorHeight = item.height;
        open();
    }

    function adopt(color) {
        picked = color;
        if (color.hsvHue >= 0) {
            hue = color.hsvHue;
        }
        saturation = color.hsvSaturation;
        brightness = color.hsvValue;
        opacityValue = color.a;
    }

    function pickFromSliders() {
        picked = Qt.hsva(hue, saturation, brightness, opacityValue);
        showHex();
    }

    function showHex() {
        hexField.text = Kdl.cssColor(picked);
    }

    function parsedHex(text) {
        const match = /^#?([0-9a-f]{6})([0-9a-f]{2})?$/i.exec(text.trim());
        return match ? Qt.color("#" + (match[2] || "ff") + match[1]) : null;
    }

    function accept() {
        chosen(picked);
        close();
    }

    parent: QQC2.Overlay.overlay
    z: 1
    modal: true
    dim: false
    focus: true
    closePolicy: QQC2.Popup.CloseOnEscape | QQC2.Popup.CloseOnPressOutside
    margins: Kirigami.Units.smallSpacing
    padding: Kirigami.Units.largeSpacing
    width: Math.min(Kirigami.Units.gridUnit * 17, parent.width - margins * 2)
    x: anchorTop.x
    y: anchorTop.y + anchorHeight + height + margins <= parent.height ? anchorTop.y + anchorHeight : anchorTop.y - height

    contentItem: ColumnLayout {
        spacing: Kirigami.Units.smallSpacing

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: Kirigami.Units.gridUnit * 3
            Layout.preferredHeight: Kirigami.Units.gridUnit * 8
            Accessible.role: Accessible.Graphic
            Accessible.name: "Hue and saturation"

            Rectangle {
                anchors.fill: parent
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0 / 6; color: "#ff0000" }
                    GradientStop { position: 1 / 6; color: "#ffff00" }
                    GradientStop { position: 2 / 6; color: "#00ff00" }
                    GradientStop { position: 3 / 6; color: "#00ffff" }
                    GradientStop { position: 4 / 6; color: "#0000ff" }
                    GradientStop { position: 5 / 6; color: "#ff00ff" }
                    GradientStop { position: 6 / 6; color: "#ff0000" }
                }
            }

            Rectangle {
                anchors.fill: parent
                gradient: Gradient {
                    GradientStop { position: 0; color: "#00ffffff" }
                    GradientStop { position: 1; color: "#ffffffff" }
                }
            }

            Rectangle {
                anchors.fill: parent
                color: "black"
                opacity: 1 - popup.brightness
            }

            Rectangle {
                x: popup.hue * parent.width - width / 2
                y: (1 - popup.saturation) * parent.height - height / 2
                width: Kirigami.Units.gridUnit * 0.75
                height: width
                radius: width / 2
                color: "transparent"
                border.color: "white"
                border.width: 2

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: -1
                    radius: width / 2
                    color: "transparent"
                    border.color: "black"
                    border.width: 1
                }
            }

            MouseArea {
                anchors.fill: parent
                preventStealing: true

                function pick(pointX, pointY) {
                    popup.hue = Math.max(0, Math.min(1, pointX / width));
                    popup.saturation = Math.max(0, Math.min(1, 1 - pointY / height));
                    popup.pickFromSliders();
                }

                onPressed: mouse => pick(mouse.x, mouse.y)
                onPositionChanged: mouse => pick(mouse.x, mouse.y)
            }
        }

        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: Kirigami.Units.smallSpacing
            rowSpacing: 0

            QQC2.Label {
                text: "Brightness"
            }

            QQC2.Slider {
                Layout.fillWidth: true
                Accessible.name: "Brightness"
                value: popup.brightness
                onMoved: {
                    popup.brightness = value;
                    popup.pickFromSliders();
                }
            }

            QQC2.Label {
                text: "Opacity"
            }

            QQC2.Slider {
                Layout.fillWidth: true
                Accessible.name: "Opacity"
                value: popup.opacityValue
                onMoved: {
                    popup.opacityValue = value;
                    popup.pickFromSliders();
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Kirigami.Units.smallSpacing

            ColorSwatch {
                Layout.preferredWidth: Kirigami.Units.gridUnit * 2
                Layout.fillHeight: true
                color: popup.initial
            }

            ColorSwatch {
                Layout.preferredWidth: Kirigami.Units.gridUnit * 2
                Layout.fillHeight: true
                color: popup.picked
            }

            QQC2.TextField {
                id: hexField

                Layout.fillWidth: true
                Layout.minimumWidth: Kirigami.Units.gridUnit * 5
                Accessible.name: "Hex color"
                font.family: "monospace"
                placeholderText: "#rrggbb"
                onTextEdited: {
                    const color = popup.parsedHex(text);
                    if (color) {
                        popup.adopt(color);
                    }
                }
                onEditingFinished: popup.showHex()
                onAccepted: popup.accept()
            }
        }

        QQC2.DialogButtonBox {
            Layout.fillWidth: true
            padding: 0
            standardButtons: QQC2.DialogButtonBox.Ok | QQC2.DialogButtonBox.Cancel
            onAccepted: popup.accept()
            onRejected: popup.close()
        }
    }
}
