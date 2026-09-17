import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

Item {
    id: preview

    property string modKey: "Super"
    property int shortcutCount

    readonly property var row: [
        { id: "Ctrl", label: "Ctrl", width: 1.5 },
        { id: "Super", label: "Meta", width: 1.25 },
        { id: "Alt", label: "Alt", width: 1.25 },
        { id: "Space", label: "", width: 6 },
        { id: "ISO_Level3_Shift", label: "AltGr", width: 1.25 },
        { id: "ISO_Level5_Shift", label: "Level 5", width: 1.25 },
        { id: "CtrlRight", label: "Ctrl", width: 1.5 }
    ]

    function highlighted(id) {
        if (preview.modKey === "Ctrl") {
            return id === "Ctrl" || id === "CtrlRight";
        }
        return id === preview.modKey;
    }

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width, Kirigami.Units.gridUnit * 30)
        spacing: Kirigami.Units.smallSpacing

        RowLayout {
            Layout.fillWidth: true
            spacing: Kirigami.Units.smallSpacing

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: Kirigami.Units.gridUnit * 1.8
                radius: Kirigami.Units.cornerRadius
                color: Qt.alpha(Kirigami.Theme.textColor, 0.06)
                border.color: Qt.alpha(Kirigami.Theme.textColor, 0.2)

                QQC2.Label {
                    anchors.centerIn: parent
                    text: "Shift"
                    opacity: 0.6
                }
            }

            Repeater {
                model: ["Z", "X", "C", "V", "B", "N", "M"]

                Rectangle {
                    required property string modelData
                    Layout.preferredWidth: Kirigami.Units.gridUnit * 1.8
                    Layout.preferredHeight: Kirigami.Units.gridUnit * 1.8
                    radius: Kirigami.Units.cornerRadius
                    color: Qt.alpha(Kirigami.Theme.textColor, 0.06)
                    border.color: Qt.alpha(Kirigami.Theme.textColor, 0.2)

                    QQC2.Label {
                        anchors.centerIn: parent
                        text: parent.modelData
                        opacity: 0.6
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Kirigami.Units.smallSpacing

            Repeater {
                model: preview.row

                Rectangle {
                    id: key
                    required property var modelData
                    readonly property bool active: preview.highlighted(modelData.id)

                    Layout.fillWidth: true
                    Layout.preferredWidth: modelData.width * 100
                    Layout.preferredHeight: Kirigami.Units.gridUnit * 1.8
                    radius: Kirigami.Units.cornerRadius
                    color: active ? Qt.alpha(Kirigami.Theme.highlightColor, 0.35) : Qt.alpha(Kirigami.Theme.textColor, 0.06)
                    border.color: active ? Kirigami.Theme.highlightColor : Qt.alpha(Kirigami.Theme.textColor, 0.2)
                    border.width: active ? 2 : 1
                    scale: active ? 1.06 : 1

                    Behavior on scale {
                        NumberAnimation {
                            duration: Kirigami.Units.longDuration
                            easing.type: Easing.OutBack
                        }
                    }

                    Behavior on color {
                        ColorAnimation {
                            duration: Kirigami.Units.longDuration
                        }
                    }

                    QQC2.Label {
                        anchors.centerIn: parent
                        text: key.active ? "Mod" : key.modelData.label
                        font.bold: key.active
                        opacity: key.active ? 1 : 0.6
                    }
                }
            }
        }

        QQC2.Label {
            Layout.alignment: Qt.AlignHCenter
            text: preview.shortcutCount + " shortcuts · “Mod” means the highlighted key"
            opacity: 0.8
        }
    }
}
