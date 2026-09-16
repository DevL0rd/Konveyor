import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

ColumnLayout {
    id: options

    property bool expanded: false
    property bool repeat
    property int cooldown
    property string cheatsheetTitle
    property bool hideFromCheatsheet
    property string placeholderTitle
    signal repeatEdited(bool value)
    signal cooldownEdited(int value)
    signal titleEdited(string value)
    signal hiddenEdited(bool value)

    spacing: Kirigami.Units.smallSpacing

    QQC2.ToolButton {
        text: "More options"
        icon.name: options.expanded ? "arrow-down" : "arrow-right"
        onClicked: options.expanded = !options.expanded
    }

    Kirigami.FormLayout {
        visible: options.expanded
        Layout.fillWidth: true

        QQC2.Switch {
            Kirigami.FormData.label: "Repeat while held:"
            checked: options.repeat
            onToggled: options.repeatEdited(checked)
        }

        RowLayout {
            Kirigami.FormData.label: "Cooldown:"
            spacing: Kirigami.Units.smallSpacing

            QQC2.SpinBox {
                from: 0
                to: 5000
                stepSize: 50
                editable: true
                value: options.cooldown
                textFromValue: (number, locale) => number === 0 ? "Off" : number + " ms"
                valueFromText: (text, locale) => parseInt(text) || 0
                onValueModified: options.cooldownEdited(value)
            }

            QQC2.Label {
                text: "Ignore repeats this soon after firing"
                opacity: 0.7
            }
        }

        QQC2.Switch {
            Kirigami.FormData.label: "Cheatsheet:"
            text: "Show in the shortcut cheatsheet"
            checked: !options.hideFromCheatsheet
            onToggled: options.hiddenEdited(!checked)
        }

        QQC2.TextField {
            Kirigami.FormData.label: "Cheatsheet title:"
            enabled: !options.hideFromCheatsheet
            placeholderText: options.placeholderTitle
            text: options.cheatsheetTitle
            onTextEdited: options.titleEdited(text)
            Layout.fillWidth: true
        }
    }
}
