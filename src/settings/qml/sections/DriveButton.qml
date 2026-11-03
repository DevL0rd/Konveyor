import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import "../catalog/Kdl.js" as Kdl
import "../catalog/Signals.js" as Signals
import org.kde.konveyor.settings

QQC2.ToolButton {
    id: button

    property string layerPath
    property string setting
    property string settingLabel
    property var layerValues: null
    property real amountRange: 1
    readonly property var drive: ((layerValues || {}).drives || []).find(entry => entry.setting === setting) || null
    readonly property bool active: drive !== null && drive.amount !== 0
    readonly property var entries: Signals.pickerEntries()

    function entryPath() {
        const found = SettingsStore.children(layerPath, "drive").find(entry => (entry.node.args || [])[0] === setting);
        return found ? found.path : "";
    }

    function write(signal, amount, smooth, curve) {
        const path = entryPath();
        if (signal.length === 0) {
            if (drive && !path.length) {
                SettingsStore.append(layerPath, Kdl.leaf("drive", [setting, drive.signal], { amount: 0 }));
            } else if (path.length) {
                SettingsStore.remove(path);
            }
            return;
        }
        const node = Kdl.leaf("drive", [setting, signal], { amount: Number(amount.toFixed(3)), smooth: Number(smooth.toFixed(2)), curve: curve });
        if (path.length) {
            SettingsStore.setNode(path, node);
        } else {
            SettingsStore.append(layerPath, node);
        }
    }

    function commit() {
        write(entries[signalPicker.currentIndex].value, amountSlider.value, smoothSlider.value, Signals.curves[curvePicker.currentIndex].value);
    }

    icon.name: "draw-bezier-curves"
    display: QQC2.AbstractButton.IconOnly
    text: active ? settingLabel + " reacts to " + entries.find(entry => entry.value === drive.signal).label : "Make " + settingLabel + " react to something"
    onClicked: popup.open()
    QQC2.ToolTip.text: text
    QQC2.ToolTip.visible: hovered
    QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay

    Rectangle {
        z: -1
        anchors.fill: parent
        visible: button.active
        radius: Kirigami.Units.cornerRadius
        color: Qt.alpha(Kirigami.Theme.highlightColor, 0.3)
        border.width: 1
        border.color: Kirigami.Theme.highlightColor
    }

    QQC2.Popup {
        id: popup
        parent: button
        x: button.width - width
        margins: Kirigami.Units.smallSpacing
        modal: false
        focus: true
        padding: Kirigami.Units.largeSpacing
        onAboutToShow: {
            const below = button.mapToItem(QQC2.Overlay.overlay, 0, button.height).y;
            y = below + height + margins > QQC2.Overlay.overlay.height ? -height : button.height;
        }

        GridLayout {
            columns: 2
            columnSpacing: Kirigami.Units.largeSpacing
            rowSpacing: Kirigami.Units.smallSpacing

            QQC2.Label {
                text: "React to"
            }

            QQC2.ComboBox {
                id: signalPicker
                Layout.preferredWidth: Kirigami.Units.gridUnit * 13
                model: button.entries
                textRole: "label"
                currentIndex: button.active ? Math.max(0, button.entries.findIndex(entry => entry.value === button.drive.signal)) : 0
                onActivated: button.commit()
            }

            QQC2.Label {
                text: "Amount"
            }

            QQC2.Slider {
                id: amountSlider
                Layout.fillWidth: true
                enabled: signalPicker.currentIndex > 0
                from: -button.amountRange
                to: button.amountRange
                value: button.drive ? button.drive.amount : button.amountRange / 2
                onPressedChanged: {
                    if (!pressed) {
                        button.commit();
                    }
                }
            }

            QQC2.Label {
                text: "Smoothing"
            }

            QQC2.Slider {
                id: smoothSlider
                Layout.fillWidth: true
                enabled: signalPicker.currentIndex > 0
                from: 0
                to: 1
                value: button.drive ? button.drive.smooth : 0.1
                onPressedChanged: {
                    if (!pressed) {
                        button.commit();
                    }
                }
            }

            QQC2.Label {
                text: "Curve"
            }

            QQC2.ComboBox {
                id: curvePicker
                Layout.fillWidth: true
                enabled: signalPicker.currentIndex > 0
                model: Signals.curves
                textRole: "label"
                currentIndex: button.drive ? Math.max(0, Signals.curves.findIndex(entry => entry.value === button.drive.curve)) : 0
                onActivated: button.commit()
            }
        }
    }
}
