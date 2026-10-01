import QtQuick
import QtQuick.Window
import org.kde.taskmanager as TaskManager
import org.kde.plasma.workspace.dbus as DBus
import "Order.js" as Order
import "Sync.js" as Sync

TaskbarView {
    id: root
    objectName: "konveyor-taskbar"
    property alias tasksModel: tasks
    property string outputName: ""
    property bool busy: false
    property var revisions: ({})
    property string scopeKey: ""
    property bool initialized: false
    property string editedScope: ""
    property int editRevision: 0
    property int appliedEdit: 0
    property var groupingBlacklist: []
    property string groupingSignature: ""
    property string syncError: ""
    signal pinsChanged(var launchers)
    signal groupingChanged(string mode)
    readonly property rect screenGeometry: Qt.rect(Screen.virtualX, Screen.virtualY, Screen.width, Screen.height)

    TaskManager.TasksModel {
        id: tasks
        sortMode: TaskManager.TasksModel.SortManual
        groupMode: root.grouped ? TaskManager.TasksModel.GroupApplications : TaskManager.TasksModel.GroupDisabled
        groupingWindowTasksThreshold: -1
        groupingAppIdBlacklist: root.groupingBlacklist
        separateLaunchers: false
        launchInPlace: true
        launcherList: root.pinnedLaunchers
        filterByCurrentVirtualDesktop: true
        filterByScreen: true
        screenGeometry: root.screenGeometry
        onLauncherListChanged: {
            if (JSON.stringify(launcherList) !== JSON.stringify(root.pinnedLaunchers))
                root.pinsChanged(launcherList)
        }
    }
    Instantiator {
        id: rows
        model: tasks
        delegate: QtObject {
            required property int index
            required property var model
            readonly property var entry: ({
                index: index,
                appId: String(model.AppId || ""),
                title: String(model.display || model.AppName || ""),
                icon: model.decoration,
                active: !!model.IsActive,
                launcher: !!model.IsLauncher,
                group: !!model.IsGroupParent,
                windowIds: model.WinIdList || [],
                launcherUrl: String(model.LauncherUrlWithoutIcon || "")
            })
            onEntryChanged: Qt.callLater(root.collect)
        }
        onObjectAdded: Qt.callLater(root.collect)
        onObjectRemoved: Qt.callLater(root.collect)
    }
    function collect() {
        const next = []
        for (let i = 0; i < rows.count; ++i) {
            const row = rows.objectAt(i)
            if (row)
                next.push(row)
        }
        next.sort((a, b) => a.index - b.index)
        if (next.length !== entries.length || next.some((row, index) => row !== entries[index]))
            entries = next
    }
    function call(member, args, done) {
        const reply = DBus.SessionBus.asyncCall({service: "org.kde.Konveyor", path: "/Konveyor", iface: "org.kde.Konveyor", member: member, arguments: args})
        const complete = () => done(reply.isError ? null : reply.value)
        if (reply.isFinished)
            complete()
        else
            reply.finished.connect(complete)
    }
    function moveNative(order) {
        for (let index = 0; index < order.length; ++index) {
            if (order[index] !== index) {
                tasks.move(Sync.entry(entries[order[index]]).index, Sync.entry(entries[index]).index)
                Qt.callLater(collect)
                return true
            }
        }
        return false
    }
    function reconcile(workspace, allWindows) {
        const scoped = Sync.scoped(allWindows, outputName, workspace.id)
        const grouping = Sync.grouping(entries, scoped, groupingMode, workspace.taskbar_grouping)
        const signature = JSON.stringify(grouping)
        if (signature !== groupingSignature) {
            groupingSignature = signature
            grouped = grouping.grouped
            groupingBlacklist = grouping.blacklist
            return
        }
        if (workspace.taskbar_interactive || !Sync.ready(entries, scoped))
            return
        const scope = outputName + ":" + workspace.id
        scopeKey = scope
        const revision = workspace.taskbar_revision
        const edit = editRevision
        const edited = edit !== appliedEdit && editedScope === scope
        const fresh = !initialized
        const changed = revisions[scope] !== revision
        const launched = !fresh && revisions[scope] !== undefined && changed && workspace.taskbar_source === "launch"
        if (launched || scoped.length === 0) {
            if (moveNative(Sync.pinned(entries, Array.from(pinnedLaunchers))))
                return
        }
        if (!edited && !launched) {
            if (moveNative(Sync.reverse(entries, scoped)))
                return
            revisions[scope] = revision
            initialized = true
            return
        }
        const groups = Order.groups(entries, scoped, outputName)
        const action = {name: "order-taskbar-columns", arguments: [outputName, JSON.stringify(groups), String(workspace.id), String(revision)]}
        busy = true
        call("Action", [JSON.stringify(action)], result => {
            syncError = result === null ? qsTr("Konveyor is unavailable") : String(result)
            if (!syncError) {
                revisions[scope] = revision
                initialized = true
                appliedEdit = edit
            }
            busy = false
        })
    }
    function syncColumns() {
        if (busy)
            return
        busy = true
        call("Outputs", [], value => {
            if (value === null) {
                busy = false
                return
            }
            const output = JSON.parse(value).find(item => item.logical.x === screenGeometry.x && item.logical.y === screenGeometry.y)
            outputName = output ? output.name : ""
            call("Workspaces", [], value => {
                if (value === null || !outputName) {
                    busy = false
                    return
                }
                const workspace = JSON.parse(value).find(item => item.output === outputName && item.is_active)
                call("Windows", [], value => {
                    busy = false
                    if (value !== null && workspace)
                        reconcile(workspace, JSON.parse(value))
                })
            })
        })
    }
    function windowIndex(entry) {
        if (!entry.group)
            return tasks.makeModelIndex(entry.index)
        const parent = tasks.makeModelIndex(entry.index)
        for (let child = 0; child < tasks.rowCount(parent); ++child) {
            const index = tasks.makeModelIndex(entry.index, child)
            if (tasks.data(index, TaskManager.AbstractTasksModel.IsActive))
                return index
        }
        return tasks.makeModelIndex(entry.index, 0)
    }
    onMoveRequested: (from, to) => {
        if (tasks.move(from, to)) {
            tasks.syncLaunchers()
            Qt.callLater(collect)
            editedScope = scopeKey
            ++editRevision
        }
    }
    onActivateRequested: entry => {
        if (entry.active && !entry.launcher)
            tasks.requestToggleMinimized(windowIndex(entry))
        else
            tasks.requestActivate(windowIndex(entry))
    }
    onCloseRequested: entry => {
        if (!entry.launcher)
            tasks.requestClose(windowIndex(entry))
    }
    onPinRequested: entry => {
        if (pinnedLaunchers.includes(entry.launcherUrl))
            tasks.requestRemoveLauncher(entry.launcherUrl)
        else
            tasks.requestAddLauncher(entry.launcherUrl)
    }
    onGroupingRequested: mode => groupingChanged(mode)
    Timer {
        interval: 300
        running: true
        repeat: true
        onTriggered: root.syncColumns()
    }
}
