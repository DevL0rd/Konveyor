import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PlasmaComponents
import org.kde.plasma.extras as PlasmaExtras

Item {
    id: page

    property var category: null
    property bool allApps: false
    property var sections: []
    property var favorites: []
    property var favSet: ({})
    property string viewMode: "grid"
    property string searchText: ""
    property string sortMode: "recent"
    property var usage: ({})

    function activateFirst() {
        if (!allApps)
            return appGrid.activateFirst()
        if (favSection.activateFirst())
            return true
        for (let i = 0; i < sectionRepeater.count; i++) {
            const section = sectionRepeater.itemAt(i)
            if (section && section.grid.activateFirst())
                return true
        }
        return false
    }

    component SectionHeader: RowLayout {
        property string text
        property int count: 0
        Layout.fillWidth: true
        Layout.topMargin: Kirigami.Units.smallSpacing
        spacing: Kirigami.Units.smallSpacing
        PlasmaComponents.Label {
            text: parent.text
            font.pointSize: Kirigami.Theme.smallFont.pointSize
            font.weight: Font.DemiBold
            font.capitalization: Font.AllUppercase
            font.letterSpacing: 0.6
            opacity: 0.65
        }
        PlasmaComponents.Label {
            text: parent.count
            font.pointSize: Kirigami.Theme.smallFont.pointSize
            font.features: { "tnum": 1 }
            opacity: 0.45
        }
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 1
            color: Qt.alpha(Kirigami.Theme.textColor, 0.1)
        }
    }

    AppGrid {
        id: appGrid
        anchors.fill: parent
        visible: !page.allApps
        appModel: page.allApps ? null : root.appModelFor(page.category)
        favSet: page.favSet
        viewMode: page.viewMode
        searchText: page.searchText
        sortMode: page.sortMode
        usage: page.usage
        onLaunchedKey: function(key) { root.recordLaunch(key); root.launchAndClose() }
        onFavToggle: function(resource, add) { root.toggleFavorite(resource, add) }
    }

    QQC2.ScrollView {
        id: sectionsScroll
        anchors.fill: parent
        visible: page.allApps
        contentWidth: availableWidth
        clip: true

        ColumnLayout {
            width: sectionsScroll.availableWidth
            spacing: Kirigami.Units.smallSpacing

            SectionHeader {
                visible: favSection.items.length > 0
                text: i18n("Favourites")
                count: favSection.items.length
            }
            FavGrid {
                id: favSection
                Layout.fillWidth: true
                Layout.preferredHeight: contentHeightHint
                visible: items.length > 0
                sectionMode: true
                viewMode: page.viewMode
                favorites: page.favorites
                searchText: page.searchText
                onLaunched: root.launchAndClose()
                onRemoveFav: function(resource) { root.toggleFavorite(resource, false) }
            }

            Repeater {
                id: sectionRepeater
                model: page.sections
                delegate: ColumnLayout {
                    required property var modelData
                    readonly property alias grid: catGrid
                    Layout.fillWidth: true
                    visible: catGrid.items.length > 0
                    spacing: Kirigami.Units.smallSpacing
                    SectionHeader {
                        text: modelData.label
                        count: catGrid.items.length
                    }
                    AppGrid {
                        id: catGrid
                        Layout.fillWidth: true
                        Layout.preferredHeight: contentHeightHint
                        appModel: root.appModelFor(modelData)
                        favSet: page.favSet
                        excludeFavorites: true
                        sectionMode: true
                        viewMode: page.viewMode
                        searchText: page.searchText
                        sortMode: page.sortMode
                        usage: page.usage
                        onLaunchedKey: function(key) { root.recordLaunch(key); root.launchAndClose() }
                        onFavToggle: function(resource, add) { root.toggleFavorite(resource, add) }
                    }
                }
            }
        }
    }
}
