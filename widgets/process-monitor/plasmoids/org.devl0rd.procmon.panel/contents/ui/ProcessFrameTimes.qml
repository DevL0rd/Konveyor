import QtQuick
import org.kde.plasma.workspace.dbus as DBus
import "lib/Ring.js" as Ring

ProcessActions {
    id: root

    property var frametimeRings: ({})
    property int frametimeGeneration: 0

    readonly property var frametimeWatchPids: {
        const seen = {}
        const result = []
        function add(pid) {
            pid = Number(pid) || 0
            if (pid > 0 && seen[pid] !== true) {
                seen[pid] = true
                result.push(pid)
            }
        }
        add(focusPid)
        if (focusProc)
            add(focusProc.framePid)
        for (const pid of monitorOverlay.pids) {
            add(pid)
            const proc = overlayProcByPid[pid]
            if (proc)
                add(proc.framePid)
        }
        return result.sort((a, b) => a - b)
    }
    readonly property string frametimeWatchKey: frametimeWatchPids.join(",")
    onFrametimeWatchKeyChanged: syncFrametimeWatch()

    function syncFrametimeWatch() {
        const message = {
            service: "org.devl0rd.ProcessMonitor.FrameTelemetry",
            path: "/FrameTelemetry",
            iface: "org.devl0rd.ProcessMonitor.FrameTelemetry",
            member: "Watch",
            arguments: [JSON.stringify(frametimeWatchPids)]
        }
        DBus.SessionBus.asyncCall(message)
        const keep = {}
        for (const pid of frametimeWatchPids)
            if (frametimeRings[pid]) keep[pid] = frametimeRings[pid]
        frametimeRings = keep
    }

    function recordFrametime(pid, frametime) {
        if (pid <= 0 || frametime <= 0 || frametime > 2000)
            return
        let ring = frametimeRings[pid]
        if (!ring) {
            ring = Ring.make(240)
            frametimeRings[pid] = ring
        }
        Ring.push(ring, frametime)
        frametimeGeneration++
    }

    function frametimesFor(pid) {
        frametimeGeneration
        const ring = frametimeRings[pid]
        return ring ? Ring.values(ring) : []
    }
}
