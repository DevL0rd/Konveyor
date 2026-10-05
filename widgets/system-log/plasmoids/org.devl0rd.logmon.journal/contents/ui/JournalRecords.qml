import QtQuick
import org.kde.plasma.plasmoid

PlasmoidItem {
    id: root

    function shq(s) { return "'" + String(s).replace(/'/g, "'\\''") + "'" }
    function escapeRegex(s) { return String(s).replace(/[.*+?^${}()|[\]\\]/g, "\\$&") }

    function rowFor(r) {
        const d = new Date(r.t / 1000)
        const pad = n => ("0" + n).slice(-2)
        return { key: r.t, time: pad(d.getHours()) + ":" + pad(d.getMinutes()) + ":" + pad(d.getSeconds()),
                 date: d.toLocaleDateString(Qt.locale(), Locale.ShortFormat),
                 app: r.id, unit: r.u || "", pid: r.pid, msg: r.m.replace(/\x1b\[[0-9;?]*[ -\/]*[@-~]/g, ""), prio: r.p, expanded: false }
    }

    function decodeUtf8(bytes) {
        let out = ""
        let i = 0
        while (i < bytes.length) {
            const lead = bytes[i]
            const size = lead < 0x80 ? 1 : lead >= 0xc2 && lead < 0xe0 ? 2 : lead >= 0xe0 && lead < 0xf0 ? 3 : lead >= 0xf0 && lead < 0xf5 ? 4 : 0
            let code = size === 1 ? lead : lead & (0xff >> (size + 1))
            let used = 1
            while (size > 1 && used < size && (bytes[i + used] & 0xc0) === 0x80) {
                code = (code << 6) | (bytes[i + used] & 0x3f)
                used++
            }
            const valid = size > 0 && used === size && !(size === 3 && (code < 0x800 || (code >= 0xd800 && code < 0xe000)))
                && !(size === 4 && (code < 0x10000 || code > 0x10ffff))
            out += valid ? String.fromCodePoint(code) : "\ufffd"
            i += valid ? used : Math.max(1, used)
        }
        return out
    }

    function journalText(value, separator) {
        if (!Array.isArray(value))
            return String(value)
        if (value.every(item => typeof item === "number"))
            return decodeUtf8(value)
        return value.map(item => journalText(item, separator)).join(separator)
    }

    function parseRec(line) {
        let j
        try { j = JSON.parse(line) } catch (e) { return null }
        const unit = journalText(j._SYSTEMD_UNIT || j.UNIT || "", ", ")
        let id = journalText(j.SYSLOG_IDENTIFIER || j._COMM || "", ", ")
        if (!id)
            id = (j._TRANSPORT === "kernel") ? "kernel"
               : (unit.indexOf(".service") >= 0 ? unit.replace(".service", "") : (unit || "?"))
        const priority = Array.isArray(j.PRIORITY) ? j.PRIORITY[0] : j.PRIORITY
        return { cursor: String(j.__CURSOR), t: parseInt(j.__REALTIME_TIMESTAMP || 0),
                 p: parseInt(priority !== undefined ? priority : 6),
                 id: id.slice(0, 40),
                 u: unit, pid: journalText(j._PID || j.SYSLOG_PID || "", ", "), m: journalText(j.MESSAGE || "", "\n") }
    }
}
