import QtQuick
import QtQuick.Layouts
import QtQml.Models
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PlasmaComponents
import org.kde.plasma.private.kicker as Kicker
import "../lib"
import ".."

PopScroll {
    id: page

    readonly property var sections: [places, documents, folders]

    Kicker.RecentUsageModel {
        id: recentFolders
        shownItems: Kicker.RecentUsageModel.OnlyFolders
    }

    DelegateModel {
        id: placeItems
        model: launcherData.places
        groups: DelegateModelGroup {
            name: "locations"
            includeByDefault: true
        }
        filterOnGroup: "locations"
        items.onChanged: {
            for (let i = items.count - 1; i >= 0; --i) {
                const entry = items.get(i)
                if (entry.inLocations && !entry.model.url)
                    items.removeGroups(i, 1, "locations")
            }
        }
        delegate: KickerTile {
            sourceModel: launcherData.places
            sourceIndex: DelegateModel.itemsIndex
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: Kirigami.Units.largeSpacing
        Kirigami.Heading {
            level: 2
            text: i18n("Files")
            Layout.fillWidth: true
        }
        Segment {
            text: i18n("Search files")
            iconName: "search-symbolic"
            onClicked: launcher.setQuery("f ")
        }
        Segment {
            text: i18n("Open file manager")
            iconName: "system-file-manager-symbolic"
            onClicked: {
                Qt.openUrlExternally(launcherData.homeUrl)
                root.hide()
            }
        }
    }

    SectionHeader {
        title: i18n("Places")
    }
    TileGrid {
        id: places
        Layout.fillWidth: true
        Layout.preferredHeight: implicitHeight
        cellWidth: Math.floor(width / Math.max(1, Math.floor(width / (Kirigami.Units.gridUnit * 7))))
        cellHeight: Kirigami.Units.gridUnit * 6
        iconSize: Kirigami.Units.iconSizes.large
        model: placeItems
    }

    SectionHeader {
        title: i18n("Recent documents")
        trailing: documents.count > 0 ? documents.count + "" : ""
    }
    RowLayout {
        visible: documents.count === 0
        Layout.fillWidth: true
        spacing: Kirigami.Units.largeSpacing
        Kirigami.Icon {
            Layout.preferredWidth: Kirigami.Units.iconSizes.medium
            Layout.preferredHeight: Layout.preferredWidth
            source: "document-open-recent-symbolic"
            color: launcher.ink
            isMask: true
            opacity: 0.35
        }
        ColumnLayout {
            spacing: 0
            PlasmaComponents.Label {
                text: i18n("Nothing opened recently")
                font.weight: Font.DemiBold
                opacity: 0.8
            }
            PlasmaComponents.Label {
                text: i18n("Files you open show up here. Type f and a name to search every file.")
                opacity: 0.55
            }
        }
    }
    TileGrid {
        id: documents
        Layout.fillWidth: true
        Layout.preferredHeight: implicitHeight
        cellWidth: Math.floor(width / Math.max(1, Math.floor(width / (Kirigami.Units.gridUnit * 16))))
        cellHeight: Kirigami.Units.gridUnit * 2.8
        model: launcherData.recentDocs
        delegate: KickerRow {}
    }

    SectionHeader {
        visible: folders.count > 0
        title: i18n("Recent folders")
    }
    TileGrid {
        id: folders
        visible: count > 0
        Layout.fillWidth: true
        Layout.preferredHeight: implicitHeight
        cellWidth: Math.floor(width / Math.max(1, Math.floor(width / (Kirigami.Units.gridUnit * 16))))
        cellHeight: Kirigami.Units.gridUnit * 2.8
        model: recentFolders
        delegate: KickerRow {}
    }

    Item {
        Layout.fillHeight: true
    }
}
