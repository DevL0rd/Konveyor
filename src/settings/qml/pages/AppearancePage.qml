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

    title: "Appearance"
    jumpSections: ["Focus ring", "Border", "Animated styles", "Event effects", "Tab indicator", "Drop hint", "Window corners"]
    previewBackdrop: true
    preview: RowLayout {
        id: stage
        readonly property var values: SettingsStore.revision >= 0 ? SettingsStore.scope("layout") : ({})
        readonly property bool wide: width >= Kirigami.Units.gridUnit * 46
        spacing: Kirigami.Units.gridUnit

        DecorationPreview {
            values: parent.values
            cornerRadius: SettingsStore.revision >= 0 ? Math.min(12, CornerRule.find(SettingsStore).radii[0] / 2) : 0
            tryOn: (gallery.item && gallery.item.tryOn) || (eventEffects.item && eventEffects.item.tryOn) || null
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: 3
        }

        TabIndicatorPreview {
            visible: stage.wide
            values: parent.values
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: 1.3
        }

        InsertHintPreview {
            visible: stage.wide
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

    LaterSection {
        sourceComponent: RingSection {
            key: "border"
            title: "Border"
            summary: "An outline on every window. Unlike the focus ring, it takes up space inside the column."
            iconName: "window"
        }
    }

    LaterSection {
        id: gallery
        sourceComponent: BorderGallerySection {}
    }

    LaterSection {
        id: eventEffects
        sourceComponent: EventEffectsSection {}
    }

    LaterSection {
        sourceComponent: TabIndicatorSection {}
    }

    LaterSection {
        sourceComponent: InsertHintSection {}
    }

    LaterSection {
        sourceComponent: CornersSection {}
    }
}
