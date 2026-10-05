import QtQuick
import org.kde.plasma.plasma5support as P5Support

Item {
    function shq(text) {
        return "'" + String(text).replace(/'/g, "'\\''") + "'"
    }
    P5Support.DataSource {
        id: runner
        engine: "executable"
        onNewData: function(source, data) { disconnectSource(source) }
    }
    P5Support.DataSource {
        id: reader
        engine: "executable"
        property var callbacks: ({})
        onNewData: function(source, data) {
            const callback = callbacks[source]
            disconnectSource(source)
            if (callback)
                callback((data.stdout || "").trim())
        }
    }
    function run(command) {
        runner.connectSource(command)
    }
    function readCommand(command, callback) {
        const next = Object.assign({}, reader.callbacks)
        next[command] = callback
        reader.callbacks = next
        reader.connectSource(command)
    }
    TextEdit { id: clipboard; visible: false }
    function copyText(text) {
        clipboard.text = text
        clipboard.selectAll()
        clipboard.copy()
        clipboard.text = ""
    }
    function commandLineCommand(pid) {
        return "tr '\\0' ' ' < /proc/" + pid + "/cmdline"
    }
    function copyCmdline(pid) {
        readCommand(commandLineCommand(pid), text => copyText(text))
    }
    function signalProc(pid, signal) {
        run("kill -" + signal + " " + pid)
    }
    function openLocation(pid) {
        run("sh -c " + shq("d=$(dirname \"$(readlink -f /proc/" + pid + "/exe 2>/dev/null)\"); [ -d \"$d\" ] && xdg-open \"$d\""))
    }
    function openJournal(name) {
        run("konsole -e journalctl _COMM=" + shq(name) + " -e")
    }
    function restartProc(pid) {
        run("bash -c " + shq("p=" + pid + "; mapfile -d '' a < /proc/$p/cmdline; cwd=$(readlink /proc/$p/cwd); kill \"$p\"; cd \"$cwd\" 2>/dev/null; setsid \"${a[@]}\" >/dev/null 2>&1 &"))
    }
    function forceKillAsRoot(pid) {
        run("pkexec kill -9 " + pid)
    }
}
