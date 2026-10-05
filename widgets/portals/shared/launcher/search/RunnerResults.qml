import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import ".."

ColumnLayout {
    id: runnerKind
    property string kind
    property int contentCount: 0
    readonly property bool hasContent: contentCount > 0
    function recount() {
        let total = konveyorSettings.grid.count
        for (let i = 0; i < kindGroups.count; ++i) {
            const group = kindGroups.itemAt(i)
            if (group)
                total += group.grid.count
        }
        contentCount = total
    }
    spacing: Kirigami.Units.largeSpacing * 1.5
    function grids() {
        const list = [konveyorSettings.grid]
        for (let i = 0; i < kindGroups.count; ++i) {
            const group = kindGroups.itemAt(i)
            if (group)
                list.push(group.grid)
        }
        return list
    }
    KonveyorSettingsGroup {
        id: konveyorSettings
        model: runnerKind.kind === "settings" ? page.settingMatches : []
        Connections {
            target: konveyorSettings.grid
            function onCountChanged() { runnerKind.recount() }
        }
    }
    Repeater {
        id: kindGroups
        model: page.groupsByKind[runnerKind.kind] || []
        delegate: ResultGroup {
            required property var modelData
            readonly property var runnerGroup: launcherData.runner.modelForRow(modelData)
            Connections {
                target: grid
                function onCountChanged() { runnerKind.recount() }
            }
            title: runnerKind.kind === "answer" ? i18n("Answer") : runnerGroup ? runnerGroup.name : ""
            trailing: runnerKind.kind === "answer" ? "" : grid.count + ""
            cellWidth: runnerKind.kind === "answer" ? width : Math.floor(width / Math.max(1, Math.floor(width / page.rowWidth)))
            cellHeight: Kirigami.Units.gridUnit * (runnerKind.kind === "answer" ? 3.8 : 3.2)
            model: runnerGroup
            delegate: KickerRow {
                id: runnerRow
                emphasize: runnerKind.kind === "answer"
                subtitle: runnerKind.kind === "answer" ? page.term + " =" : (model.description || "")
                trailing: runnerKind.kind === "answer" ? i18n("Enter copies the result") : ""
                function activate() {
                    if (runnerKind.kind === "answer") {
                        launcherData.copyText(model.display || "")
                        launcher.hide()
                        return
                    }
                    launcher.trigger(sourceModel, sourceIndex, "")
                }
            }
        }
        onItemAdded: {
            runnerKind.recount()
            Qt.callLater(page.rebuildSections)
        }
        onItemRemoved: {
            runnerKind.recount()
            Qt.callLater(page.rebuildSections)
        }
    }
}
