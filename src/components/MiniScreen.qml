import QtQuick
import org.kde.kirigami as Kirigami

Rectangle {
    property bool compact: false

    radius: compact ? 3 : Kirigami.Units.cornerRadius * 2
    color: Qt.darker(Kirigami.Theme.backgroundColor, 1.6)
    border.color: Qt.alpha(Kirigami.Theme.textColor, 0.35)
    border.width: compact ? 1 : 2
    clip: true
}
