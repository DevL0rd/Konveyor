import QtQuick
import org.kde.plasma.plasma5support as P5Support
import "Journal.js" as Journal

Item {
    id: logActions

    required property var model
    required property var lineText
    required property TextEdit clipboard
    signal copied()

    function copyText(text) {
        logActions.clipboard.text = text
        logActions.clipboard.selectAll()
        logActions.clipboard.copy()
        logActions.copied()
    }
    function copyAll() {
        const out = []
        for (let i = 0; i < logActions.model.count; i++)
            out.push(logActions.lineText(logActions.model.get(i)))
        copyText(out.join("\n"))
    }
    function askClaude(time, app, pid, msg, prio) {
        const ctx = "I saw this entry in my systemd journal (journalctl):\n"
                  + "time: " + time + "\napp: " + app + (pid ? " (pid " + pid + ")" : "")
                  + "\npriority: " + prio + "\nmessage: " + msg
                  + "\n\nWhat does it mean, and is there anything I should do about it?"
        launcher.connectSource("konsole --workdir \"$HOME\" -e claude " + Journal.shq(ctx))
    }
    P5Support.DataSource {
        id: launcher
        engine: "executable"
        onNewData: function(source, d) { disconnectSource(source) }
    }
}
