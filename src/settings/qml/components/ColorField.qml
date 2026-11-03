import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import "../catalog/Kdl.js" as Kdl

QQC2.Button {
    id: root

    property string value
    signal edited(string css)

    readonly property int swatchMargin: 4
    readonly property color shown: value.length > 0 ? value : "transparent"

    implicitWidth: 40 + swatchMargin * 2

    onClicked: picker.openFor(value.length > 0 ? shown : Kirigami.Theme.highlightColor, root)

    ColorSwatch {
        anchors.centerIn: parent
        width: parent.width - root.swatchMargin * 2
        height: parent.height - root.swatchMargin * 2
        color: root.enabled ? root.shown : disabledPalette.button

        SystemPalette {
            id: disabledPalette
            colorGroup: SystemPalette.Disabled
        }
    }

    ColorPickerPopup {
        id: picker
        onChosen: picked => root.edited(Kdl.cssColor(picked))
    }
}
