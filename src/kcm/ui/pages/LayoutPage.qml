import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import "../components"
import "../previews"
import "../sections"

SettingsPage {
    id: page

    title: "Layout"
    preview: StripPreview {
        values: kcm.revision >= 0 ? kcm.scope("layout") : ({})
    }

    SizesSection {
        Layout.fillWidth: true
    }

    PlacementSection {
        Layout.fillWidth: true
    }
}
