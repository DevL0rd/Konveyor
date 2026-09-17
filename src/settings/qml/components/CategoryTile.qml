import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

QQC2.AbstractButton {
    id: tile

    property string title
    property string description
    property string summary
    property string iconName

    implicitHeight: Kirigami.Units.gridUnit * 6.5
    hoverEnabled: true
    Accessible.name: title
    Accessible.description: description

    background: Rectangle {
        radius: Kirigami.Units.cornerRadius * 3
        color: tile.hovered ? Qt.alpha(Kirigami.Theme.highlightColor, 0.12) : Qt.alpha(Kirigami.Theme.textColor, 0.04)
        border.color: tile.visualFocus || tile.hovered ? Kirigami.Theme.highlightColor : Qt.alpha(Kirigami.Theme.textColor, 0.12)
        border.width: tile.visualFocus ? 2 : 1

        Behavior on color {
            ColorAnimation {
                duration: Kirigami.Units.shortDuration
            }
        }
    }

    contentItem: RowLayout {
        spacing: Kirigami.Units.largeSpacing

        Rectangle {
            Layout.alignment: Qt.AlignTop
            Layout.margins: Kirigami.Units.largeSpacing
            implicitWidth: Kirigami.Units.gridUnit * 3
            implicitHeight: implicitWidth
            radius: Kirigami.Units.cornerRadius * 2
            color: Qt.alpha(Kirigami.Theme.highlightColor, 0.18)

            Kirigami.Icon {
                anchors.centerIn: parent
                width: Kirigami.Units.iconSizes.medium
                height: width
                source: tile.iconName
                scale: tile.hovered ? 1.1 : 1

                Behavior on scale {
                    NumberAnimation {
                        duration: Kirigami.Units.shortDuration
                        easing.type: Easing.OutBack
                    }
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.rightMargin: Kirigami.Units.largeSpacing
            spacing: Kirigami.Units.smallSpacing / 2

            Kirigami.Heading {
                level: 4
                text: tile.title
                Layout.fillWidth: true
                elide: Text.ElideRight
            }

            QQC2.Label {
                text: tile.description
                wrapMode: Text.Wrap
                maximumLineCount: 2
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            QQC2.Label {
                text: tile.summary
                color: Kirigami.Theme.highlightColor
                font: Kirigami.Theme.smallFont
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
        }
    }
}
