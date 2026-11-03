import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import "../components"
import "../catalog/BorderStyles.js" as BorderStyles
import "../catalog/Kdl.js" as Kdl
import org.kde.konveyor.settings

SettingRow {
    id: row

    property string layerPath
    property var rawColors: []
    property var paints: []
    property var layerValues: null
    readonly property string colorsPath: layerPath + "/colors"
    readonly property bool canAdd: rawColors.length < BorderStyles.maxColors

    function write(colors) {
        if (colors.length > 0) {
            SettingsStore.setValue(colorsPath, colors);
        } else {
            SettingsStore.remove(colorsPath);
        }
    }

    function replaced(index, value) {
        const colors = rawColors.slice();
        colors[index] = value;
        return colors;
    }

    label: "Colors"
    iconName: "color-picker"
    resetPaths: [colorsPath]
    wideControl: true

    Flow {
        width: parent.width
        spacing: Kirigami.Units.smallSpacing

        QQC2.Label {
            visible: row.rawColors.length === 0
            height: addColor.height
            verticalAlignment: Text.AlignVCenter
            text: "Follows the accent color"
            opacity: 0.7
        }

        Repeater {
            model: row.rawColors.length

            RowLayout {
                id: swatch

                required property int index
                readonly property var paint: row.paints[index] || null
                readonly property bool isFixed: !paint || paint.source === "color"
                readonly property bool showRemove: swatchHover.hovered || fixedColor.visualFocus || themeColor.visualFocus
                    || removeColor.visualFocus || Kirigami.Settings.hasTransientTouchInput
                spacing: 0

                HoverHandler {
                    id: swatchHover
                }

                ColorField {
                    id: fixedColor
                    visible: swatch.isFixed
                    Accessible.name: "Layer color"
                    value: swatch.paint && swatch.paint.color ? swatch.paint.color : ""
                    onEdited: css => row.write(row.replaced(swatch.index, css))
                }

                QQC2.Button {
                    id: themeColor
                    visible: !swatch.isFixed
                    text: swatch.paint ? (Kdl.themeColors.find(entry => entry.value === swatch.paint.source) || { label: "Theme" }).label : ""
                    icon.name: "color-management"
                    onClicked: row.write(row.replaced(swatch.index, Kdl.cssColor(Kirigami.Theme.highlightColor)))
                    QQC2.ToolTip.text: "Follows your Plasma colors. Click to switch to a color you pick."
                    QQC2.ToolTip.visible: hovered
                    QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                }

                QQC2.ToolButton {
                    id: removeColor
                    Layout.preferredWidth: Kirigami.Units.gridUnit * 1.5
                    Layout.preferredHeight: Kirigami.Units.gridUnit * 1.5
                    opacity: swatch.showRemove ? 0.75 : 0
                    padding: 0
                    icon.name: "dialog-close-symbolic"
                    icon.width: Kirigami.Units.iconSizes.small
                    icon.height: Kirigami.Units.iconSizes.small
                    display: QQC2.AbstractButton.IconOnly
                    text: "Remove this color"
                    onClicked: {
                        const colors = row.rawColors.slice();
                        colors.splice(swatch.index, 1);
                        row.write(colors);
                    }
                    QQC2.ToolTip.text: text
                    QQC2.ToolTip.visible: hovered
                }
            }
        }

        QQC2.Button {
            id: addColor
            enabled: row.canAdd
            icon.name: "list-add"
            text: "Color"
            onClicked: row.write(row.rawColors.concat(["#ffffff"]))
        }

        QQC2.Button {
            enabled: row.canAdd
            icon.name: "list-add"
            text: "Accent"
            onClicked: row.write(row.rawColors.concat(["accent"]))
        }

        DriveButton {
            layerPath: row.layerPath
            setting: "color-mix"
            settingLabel: "Color"
            layerValues: row.layerValues
        }
    }
}
