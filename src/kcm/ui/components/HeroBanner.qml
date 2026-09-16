import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

Rectangle {
    id: hero

    implicitHeight: Kirigami.Units.gridUnit * 8
    clip: true
    gradient: Gradient {
        orientation: Gradient.Horizontal
        GradientStop { position: 0; color: Qt.alpha(Kirigami.Theme.highlightColor, 0.35) }
        GradientStop { position: 1; color: Qt.alpha(Kirigami.Theme.highlightColor, 0.05) }
    }

    Item {
        id: belt
        anchors.fill: parent
        opacity: 0.7

        readonly property real tileWidth: Kirigami.Units.gridUnit * 5
        readonly property real gap: Kirigami.Units.gridUnit * 0.6
        property real offset: 0

        NumberAnimation on offset {
            from: 0
            to: belt.tileWidth + belt.gap
            duration: 2600
            loops: Animation.Infinite
            running: hero.visible
        }

        Repeater {
            model: Math.ceil(hero.width / (belt.tileWidth + belt.gap)) + 2

            Rectangle {
                required property int index
                x: hero.width - (index * (belt.tileWidth + belt.gap)) + belt.offset - belt.tileWidth
                y: Kirigami.Units.gridUnit * (index % 3 === 0 ? 1.2 : 1.8)
                width: belt.tileWidth * (index % 4 === 1 ? 1.5 : 1)
                height: hero.height - y - Kirigami.Units.gridUnit
                radius: Kirigami.Units.cornerRadius * 2
                color: Qt.alpha(Kirigami.Theme.backgroundColor, 0.8)
                border.width: index % 5 === 2 ? 2 : 1
                border.color: index % 5 === 2 ? Kirigami.Theme.highlightColor : Qt.alpha(Kirigami.Theme.textColor, 0.2)
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0; color: Kirigami.Theme.backgroundColor }
            GradientStop { position: 0.35; color: Qt.alpha(Kirigami.Theme.backgroundColor, 0.85) }
            GradientStop { position: 0.7; color: Qt.alpha(Kirigami.Theme.backgroundColor, 0) }
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: Kirigami.Units.gridUnit
        spacing: Kirigami.Units.largeSpacing

        ColumnLayout {
            spacing: 0

            Kirigami.Heading {
                level: 1
                text: "Konveyor"
            }

            QQC2.Label {
                text: "Scrolling column tiling for Plasma"
            }

            RowLayout {
                Layout.topMargin: Kirigami.Units.smallSpacing
                spacing: Kirigami.Units.smallSpacing

                Rectangle {
                    implicitWidth: 8
                    implicitHeight: 8
                    radius: 4
                    color: kcm.live.running ? Kirigami.Theme.positiveTextColor : Kirigami.Theme.neutralTextColor
                }

                QQC2.Label {
                    text: kcm.live.running ? "Running" : "Not running"
                    font: Kirigami.Theme.smallFont
                }
            }
        }

        Item {
            Layout.fillWidth: true
        }

        QQC2.Button {
            icon.name: "document-edit"
            text: "Open config file"
            onClicked: kcm.openConfigFile()
        }
    }
}
