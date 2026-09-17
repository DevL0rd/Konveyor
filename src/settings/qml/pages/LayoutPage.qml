import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import "../components"
import "../previews"
import "../sections"
import org.kde.konveyor.settings

SettingsPage {
    id: page

    title: "Layout"
    preview: StripPreview {
        values: SettingsStore.revision >= 0 ? SettingsStore.scope("layout") : ({})
    }

    SizesSection {
        Layout.fillWidth: true
    }

    PlacementSection {
        Layout.fillWidth: true
    }
}
