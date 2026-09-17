import QtQuick
import QtQuick.Layouts
import QtQuick.Shapes
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

RowLayout {
    id: editor

    property var radii: [0, 0, 0, 0]
    property bool linked: radii.every(r => r === radii[0])
    signal edited(var radii)

    function setCorner(index, value) {
        const next = editor.linked ? [value, value, value, value] : editor.radii.slice();
        next[index] = value;
        editor.edited(next);
    }

    spacing: Kirigami.Units.largeSpacing

    GridLayout {
        columns: 3
        rowSpacing: 2
        columnSpacing: 2

        component CornerSpin: QQC2.SpinBox {
            required property int corner
            property var values: []
            signal picked(int corner, int value)
            from: 0
            to: 64
            editable: true
            value: values[corner] || 0
            visible: !linkButton.checked
            textFromValue: (number, locale) => number + " px"
            valueFromText: (text, locale) => parseInt(text)
            onValueModified: picked(corner, value)
            Layout.preferredWidth: Kirigami.Units.gridUnit * 4.5
        }

        CornerSpin {
            corner: 0
            values: editor.radii
            onPicked: (corner, value) => editor.setCorner(corner, value)
        }

        Item {
            implicitWidth: 1
        }

        CornerSpin {
            corner: 1
            values: editor.radii
            onPicked: (corner, value) => editor.setCorner(corner, value)
        }

        Item {
            implicitWidth: 1
        }

        Shape {
            id: diagram
            Layout.preferredWidth: Kirigami.Units.gridUnit * 6
            Layout.preferredHeight: Kirigami.Units.gridUnit * 4
            preferredRendererType: Shape.CurveRenderer
            readonly property real zoom: 0.8

            ShapePath {
                strokeColor: Kirigami.Theme.highlightColor
                strokeWidth: 2
                fillColor: Qt.alpha(Kirigami.Theme.highlightColor, 0.15)

                PathRectangle {
                    x: 1
                    y: 1
                    width: diagram.width - 2
                    height: diagram.height - 2
                    topLeftRadius: editor.radii[0] * diagram.zoom
                    topRightRadius: editor.radii[1] * diagram.zoom
                    bottomRightRadius: editor.radii[2] * diagram.zoom
                    bottomLeftRadius: editor.radii[3] * diagram.zoom
                }
            }
        }

        Item {
            implicitWidth: 1
        }

        CornerSpin {
            corner: 3
            values: editor.radii
            onPicked: (corner, value) => editor.setCorner(corner, value)
        }

        Item {
            implicitWidth: 1
        }

        CornerSpin {
            corner: 2
            values: editor.radii
            onPicked: (corner, value) => editor.setCorner(corner, value)
        }
    }

    ColumnLayout {
        spacing: Kirigami.Units.smallSpacing

        QQC2.Button {
            id: linkButton
            checkable: true
            checked: editor.linked
            icon.name: checked ? "link" : "remove-link"
            text: checked ? "Same for every corner" : "Each corner separately"
            onToggled: {
                if (checked) {
                    editor.edited([editor.radii[0], editor.radii[0], editor.radii[0], editor.radii[0]]);
                }
            }
        }

        RowLayout {
            visible: linkButton.checked

            QQC2.Slider {
                from: 0
                to: 32
                stepSize: 1
                value: editor.radii[0]
                Layout.preferredWidth: Kirigami.Units.gridUnit * 8
                onMoved: editor.setCorner(0, value)
            }

            QQC2.Label {
                text: Math.round(editor.radii[0]) + " px"
            }
        }
    }
}
