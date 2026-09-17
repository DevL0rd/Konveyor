import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import "../components"
import "../previews"
import "../sections"
import "../sections/CornerRule.js" as CornerRule
import org.kde.konveyor.settings

SettingsPage {
    id: page

    title: "Look"
    preview: RowLayout {
        readonly property var values: SettingsStore.revision >= 0 ? SettingsStore.scope("layout") : ({})
        spacing: Kirigami.Units.gridUnit

        DecorationPreview {
            values: parent.values
            cornerRadius: SettingsStore.revision >= 0 ? Math.min(12, CornerRule.find(SettingsStore).radii[0] / 2) : 0
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: 3
        }

        TabIndicatorPreview {
            values: parent.values
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: 1.3
        }

        InsertHintPreview {
            values: parent.values
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: 1.6
        }
    }

    RingSection {
        Layout.fillWidth: true
        key: "focus-ring"
        title: "Focus ring"
        summary: "A colored outline drawn around the window that has focus."
        iconName: "window-new"
    }

    RingSection {
        Layout.fillWidth: true
        key: "border"
        title: "Border"
        summary: "An outline on every window. Unlike the focus ring, it takes up space inside the column."
        iconName: "window"
    }

    TabIndicatorSection {
        Layout.fillWidth: true
    }

    InsertHintSection {
        Layout.fillWidth: true
    }

    CornersSection {
        Layout.fillWidth: true
    }
}
