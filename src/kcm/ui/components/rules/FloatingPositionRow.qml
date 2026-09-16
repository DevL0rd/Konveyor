import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import ".."

SettingRow {
    id: row

    property string path
    readonly property var node: kcm.revision >= 0 ? kcm.node(path) : ({})
    readonly property bool custom: node.name !== undefined
    readonly property var props: custom ? node.props : ({ x: 32, y: 32, "relative-to": "top-left" })
    readonly property string anchor: props["relative-to"] || "top-left"
    readonly property var cells: ["top-left", "top", "top-right", "left", "", "right", "bottom-left", "bottom", "bottom-right"]

    function write(changes) {
        const next = Object.assign({ x: 0, y: 0, "relative-to": "top-left" }, props, changes);
        kcm.setValue(path, [], { x: Math.round(next.x), y: Math.round(next.y), "relative-to": next["relative-to"] });
    }

    wideControl: true

    RowLayout {
        width: parent.width
        spacing: Kirigami.Units.gridUnit

        Segmented {
            Layout.alignment: Qt.AlignTop
            currentValue: row.custom ? "custom" : "default"
            options: [
                { value: "default", label: "Default" },
                { value: "custom", label: "Custom" }
            ]
            onChosen: value => value === "default" ? kcm.remove(row.path) : row.write({})
        }

        Rectangle {
            visible: row.custom
            implicitWidth: Kirigami.Units.gridUnit * 9
            implicitHeight: Kirigami.Units.gridUnit * 5.5
            radius: Kirigami.Units.cornerRadius
            color: Qt.alpha(Kirigami.Theme.textColor, 0.05)
            border.color: Qt.alpha(Kirigami.Theme.textColor, 0.3)

            Accessible.name: "Anchor for the floating window"

            GridLayout {
                anchors.fill: parent
                anchors.margins: Kirigami.Units.smallSpacing
                columns: 3
                rowSpacing: 0
                columnSpacing: 0

                Repeater {
                    model: row.cells

                    Item {
                        id: cell
                        required property string modelData
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        Rectangle {
                            anchors.centerIn: parent
                            visible: cell.modelData.length > 0
                            width: Kirigami.Units.gridUnit * (row.anchor === cell.modelData ? 1.1 : 0.7)
                            height: width
                            radius: width / 2
                            color: row.anchor === cell.modelData ? Kirigami.Theme.highlightColor : Qt.alpha(Kirigami.Theme.textColor, area.containsMouse ? 0.45 : 0.2)

                            MouseArea {
                                id: area
                                anchors.fill: parent
                                anchors.margins: -6
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: row.write({ "relative-to": cell.modelData })
                            }

                            QQC2.ToolTip.text: cell.modelData.replace("-", " ")
                            QQC2.ToolTip.visible: area.containsMouse
                        }
                    }
                }
            }
        }

        Kirigami.FormLayout {
            visible: row.custom
            Layout.alignment: Qt.AlignTop

            QQC2.SpinBox {
                Kirigami.FormData.label: "Across:"
                from: -8000
                to: 8000
                stepSize: 8
                editable: true
                value: row.props.x
                textFromValue: (number, locale) => number + " px"
                valueFromText: (text, locale) => parseInt(text)
                onValueModified: row.write({ x: value })
            }

            QQC2.SpinBox {
                Kirigami.FormData.label: "Down:"
                from: -8000
                to: 8000
                stepSize: 8
                editable: true
                value: row.props.y
                textFromValue: (number, locale) => number + " px"
                valueFromText: (text, locale) => parseInt(text)
                onValueModified: row.write({ y: value })
            }
        }

        Item {
            Layout.fillWidth: true
        }
    }
}
