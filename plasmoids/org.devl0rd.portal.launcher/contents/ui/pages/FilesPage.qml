import QtQuick
import QtQuick.Layouts
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
        model: launcherData.places
        delegate: KickerTile {}
    }

    SectionHeader {
        title: i18n("Recent documents")
        trailing: documents.count > 0 ? documents.count + "" : ""
    }
    PlasmaComponents.Label {
        visible: documents.count === 0
        text: i18n("Nothing opened recently")
        opacity: 0.6
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
