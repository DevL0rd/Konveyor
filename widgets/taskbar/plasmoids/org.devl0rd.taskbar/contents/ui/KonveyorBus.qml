import QtQuick
import org.kde.plasma.workspace.dbus as DBus

Item {
    id: bus

    readonly property bool available: watcher.registered
    property var workspaces: []
    property var windows: []
    property var outputs: []
    property var binds: []
    property bool superHeld: false
    property bool altHeld: false

    signal layoutRefreshed()
    signal taskbarItemRequested(int number)

    function message(member, args) {
        return { service: "org.kde.Konveyor", path: "/Konveyor", iface: "org.kde.Konveyor", member: member, arguments: args || [] }
    }

    function unwrap(reply) {
        let value = reply && reply.value !== undefined ? reply.value : reply
        while (value !== null && typeof value === "object" && value.value !== undefined)
            value = value.value
        return value
    }

    function query(member, apply, args) {
        DBus.SessionBus.asyncCall(message(member, args), reply => {
            try {
                apply(JSON.parse(String(unwrap(reply))))
            } catch (error) {
                apply(null)
            }
        }, () => apply(null))
    }

    function refreshLayout() {
        let pending = 2
        const settled = () => {
            if (--pending === 0)
                bus.layoutRefreshed()
        }
        query("Workspaces", value => {
            bus.workspaces = value || []
            settled()
        })
        query("Windows", value => {
            bus.windows = value || []
            settled()
        })
    }

    function refreshAll() {
        query("Outputs", value => bus.outputs = value || [])
        query("Binds", value => bus.binds = value || [])
        query("ModifiersHeld", value => bus.holdModifiers(!!value && value.super === true, !!value && value.alt === true))
        refreshLayout()
    }

    function holdModifiers(superDown, altDown) {
        const pressed = superDown && !superHeld
        superHeld = superDown
        altHeld = altDown
        if (pressed)
            query("Binds", value => bus.binds = value || [])
    }

    function perform(name, args, id, focus) {
        const action = { name: name, arguments: (args || []).map(String), properties: {} }
        if (id !== undefined)
            action.id = id
        if (focus === false)
            action.properties.focus = "false"
        DBus.SessionBus.asyncCall(message("Action", [JSON.stringify(action)]))
    }

    function moveColumn(id, index) {
        perform("move-column-to-index", [index], id)
    }

    function ownsFocus(output) {
        return !workspaces.some(each => each.is_focused) || workspaces.some(each => each.output === output && each.is_focused)
    }

    function focusWorkspace(output, workspace) {
        const focused = workspaces.some(each => each.output === output && each.is_focused)
        if (!focused)
            perform("focus-monitor", [output])
        perform("focus-workspace", [workspace.idx])
    }

    function appRules(id, done) {
        query("AppRules", done, [JSON.stringify({ id: id })])
    }

    function setAppRule(id, option, enabled) {
        DBus.SessionBus.asyncCall(message("SetAppRule", [JSON.stringify({ id: id, option: option, enabled: enabled })]))
    }

    onAvailableChanged: if (available) refreshAll()
    Component.onCompleted: if (available) refreshAll()

    Timer {
        id: settle
        interval: 40
        onTriggered: {
            bus.query("Outputs", value => bus.outputs = value || [])
            bus.refreshLayout()
        }
    }

    DBus.DBusServiceWatcher {
        id: watcher
        busType: DBus.BusType.Session
        watchedService: "org.kde.Konveyor"
    }

    DBus.SignalWatcher {
        enabled: bus.available
        busType: DBus.BusType.Session
        service: "org.kde.Konveyor"
        path: "/Konveyor"
        iface: "org.kde.Konveyor"

        function dbusLayoutChanged() {
            settle.restart()
        }

        function dbusTaskbarItemRequested(number) {
            bus.taskbarItemRequested(Number(bus.unwrap(number)))
        }

        function dbusModifiersHeldChanged(superDown, altDown) {
            bus.holdModifiers(String(bus.unwrap(superDown)) === "true", String(bus.unwrap(altDown)) === "true")
        }
    }
}
