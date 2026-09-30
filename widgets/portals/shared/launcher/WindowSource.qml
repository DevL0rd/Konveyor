import QtQuick
import org.kde.taskmanager as TaskManager
import "lib/Highlight.js" as Highlight

QtObject {
    id: source

    property var windows: []

    function collect() {
        const list = []
        for (let i = 0; i < rows.count; ++i) {
            const row = rows.objectAt(i)
            if (row && row.isWindow)
                list.push(row.entry)
        }
        windows = list
    }
    function matches(term) {
        return windows.filter(window => Highlight.matchesAny([window.title, window.appName, window.genericName, window.appId], term))
    }
    function activate(window) {
        tasksModel.requestActivate(tasksModel.makeModelIndex(window.row.index))
    }
    function close(window) {
        tasksModel.requestClose(tasksModel.makeModelIndex(window.row.index))
    }

    readonly property TaskManager.TasksModel tasks: TaskManager.TasksModel {
        id: tasksModel
        groupMode: TaskManager.TasksModel.GroupDisabled
        filterByVirtualDesktop: false
        filterByScreen: false
        filterByActivity: false
        filterNotMinimized: false
    }

    readonly property Instantiator rows: Instantiator {
        id: rows
        model: tasksModel
        delegate: QtObject {
            id: row
            required property int index
            required property var model
            readonly property bool isWindow: !!model.IsWindow
            readonly property var entry: ({
                row: row,
                title: model.display || "",
                appName: model.AppName || "",
                genericName: model.GenericName || "",
                appId: model.AppId || "",
                icon: model.decoration,
                minimized: !!model.IsMinimized
            })
            onEntryChanged: Qt.callLater(source.collect)
            onIsWindowChanged: Qt.callLater(source.collect)
        }
        onObjectAdded: Qt.callLater(source.collect)
        onObjectRemoved: Qt.callLater(source.collect)
    }
}
