import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import "../catalog/BorderStyles.js" as BorderStyles
import "../catalog/Signals.js" as Signals
import "../previews"
import org.kde.konveyor.settings

QQC2.AbstractButton {
    id: button

    property string styleName
    readonly property var styles: SettingsStore.values["border-styles"] || []
    readonly property var layers: BorderStyles.layersNamed(styles, styleName)
    readonly property bool missing: !styles.some(style => style.name === styleName)
    readonly property bool darkTheme: Kirigami.ColorUtils.brightnessForColor(Kirigami.Theme.backgroundColor) === Kirigami.ColorUtils.Dark

    implicitWidth: 190
    implicitHeight: 42
    text: styleName
    hoverEnabled: true

    QQC2.ToolTip.text: [styleName, Signals.tooltipFor(layers)].filter(text => text.length > 0).join("\n")
    QQC2.ToolTip.visible: hovered && styleName.length > 0
    QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay

    background: Rectangle {
        radius: 3
        color: Kirigami.Theme.backgroundColor
        border.width: 1
        border.color: button.hovered || button.visualFocus ? Kirigami.Theme.highlightColor : Qt.alpha(Kirigami.Theme.textColor, 0.3)
    }

    contentItem: RowLayout {
        spacing: Kirigami.Units.smallSpacing * 2

        Item {
            Layout.leftMargin: 2
            Layout.preferredWidth: 58
            Layout.preferredHeight: 36
            clip: true

            StyleBackdrop {
                anchors.fill: parent
            }

            MockWindow {
                anchors.fill: parent
                anchors.margins: 7
                zoom: 1
                cornerRadius: 3
                barHeight: 0
                state_: "active"
                ring: ({ enabled: false, width: 3 })
                overridePart: "focus-ring"
                overrideLayers: button.layers
            }

            SignalBadge {
                anchors.top: parent.top
                anchors.left: parent.left
                showTips: false
                layers: button.layers
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 0

            QQC2.Label {
                Layout.fillWidth: true
                text: button.styleName
                elide: Text.ElideRight
            }

            QQC2.Label {
                Layout.fillWidth: true
                visible: button.missing
                text: "Style not found"
                elide: Text.ElideRight
                font: Kirigami.Theme.smallFont
                color: button.darkTheme ? "#ff7b86" : "#b01f2e"
            }
        }

        Kirigami.Icon {
            Layout.rightMargin: 6
            implicitWidth: Kirigami.Units.iconSizes.small
            implicitHeight: implicitWidth
            source: "arrow-right"
        }
    }
}
