import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

ColumnLayout {
    id: root

    property string scopePath
    property var excluded: []
    readonly property int overrideCount: kcm.revision >= 0 ? (kcm.node(scopePath).children || []).length : 0

    spacing: 0

    Kirigami.InlineMessage {
        Layout.fillWidth: true
        Layout.maximumWidth: Kirigami.Units.gridUnit * 46
        Layout.alignment: Qt.AlignHCenter
        Layout.margins: Kirigami.Units.largeSpacing
        visible: true
        type: Kirigami.MessageType.Information
        text: root.overrideCount > 0
            ? root.overrideCount + (root.overrideCount === 1 ? " layout setting differs" : " layout settings differ") + " from your main layout here. Everything else follows the Layout and Look pages."
            : "Everything here follows the Layout and Look pages. Tick a setting to change it just for this one."
    }

    SizesSection {
        Layout.fillWidth: true
        scopePath: root.scopePath
        overrideMode: true
        excluded: root.excluded
    }

    PlacementSection {
        Layout.fillWidth: true
        scopePath: root.scopePath
        overrideMode: true
        excluded: root.excluded
    }

    RingSection {
        Layout.fillWidth: true
        scopePath: root.scopePath
        overrideMode: true
        excluded: root.excluded
        key: "focus-ring"
        title: "Focus ring"
        summary: "A colored outline drawn around the window that has focus."
        iconName: "window-new"
    }

    RingSection {
        Layout.fillWidth: true
        scopePath: root.scopePath
        overrideMode: true
        excluded: root.excluded
        key: "border"
        title: "Border"
        summary: "An outline on every window. Unlike the focus ring, it takes up space inside the column."
        iconName: "window"
    }

    TabIndicatorSection {
        Layout.fillWidth: true
        scopePath: root.scopePath
        overrideMode: true
        excluded: root.excluded
    }

    InsertHintSection {
        Layout.fillWidth: true
        scopePath: root.scopePath
        overrideMode: true
        excluded: root.excluded
    }
}
