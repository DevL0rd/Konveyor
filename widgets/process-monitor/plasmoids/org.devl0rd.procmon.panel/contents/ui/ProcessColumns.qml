import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.plasma.plasmoid
import "lib/PopStyle.js" as Style

PlasmoidItem {
    id: root

    readonly property var allColumns: [
        { key: "cpu", label: i18n("CPU"), kind: "pct", heat: true, show: true },
        { key: "ram", label: i18n("RAM"), kind: "bytes", heat: false, show: true },
        { key: "gpu", label: i18n("GPU"), kind: "pct", heat: true, show: Plasmoid.configuration.showGpuColumn },
        { key: "fps", label: i18n("FPS"), kind: "fps", heat: true, show: Plasmoid.configuration.showFpsColumn, noagg: true },
        { key: "dec", label: i18n("DEC"), kind: "pct", heat: true, show: Plasmoid.configuration.showDecColumn },
        { key: "enc", label: i18n("ENC"), kind: "pct", heat: true, show: Plasmoid.configuration.showEncColumn },
        { key: "vram", label: i18n("VRAM"), kind: "bytes", heat: false, show: Plasmoid.configuration.showVramColumn },
        { key: "disk", label: i18n("Disk"), kind: "rate", heat: false, show: Plasmoid.configuration.showDiskColumn },
        { key: "threads", label: i18n("Threads"), kind: "int", heat: false, show: Plasmoid.configuration.showThreadsColumn },
        { key: "pid", label: i18n("PID"), kind: "int", heat: false, show: Plasmoid.configuration.showPidColumn, noagg: true }
    ]
    readonly property var columns: allColumns.filter(column => column.show)
    readonly property var columnConfigKeys: ({ gpu: "showGpuColumn", fps: "showFpsColumn", dec: "showDecColumn", enc: "showEncColumn", vram: "showVramColumn",
                                               disk: "showDiskColumn", threads: "showThreadsColumn", pid: "showPidColumn" })

    function colOf(key) {
        return allColumns.find(column => column.key === key) || null
    }
    function colVal(p, column) {
        if (column.noagg) return p[column.key] || 0
        if (Plasmoid.configuration.aggregateChildren) {
            const aggregate = p["a" + column.key]
            return aggregate === undefined ? (p[column.key] || 0) : aggregate
        }
        return p[column.key] || 0
    }
    function fmtValue(value, kind) {
        if (kind === "pct") return Math.round(value) + "%"
        if (kind === "bytes") return value > 0 ? Style.bytes(value) : "—"
        if (kind === "rate") return value > 0 ? Style.bytes(value) + "/s" : "—"
        return value + ""
    }
    function fmtCol(p, column) {
        if (column.kind === "fps") return p.fps === undefined ? "—" : Math.round(p.fps) + ""
        return fmtValue(colVal(p, column), column.kind)
    }
    function heatColor(value, theme) {
        const colors = theme || Kirigami.Theme
        if (!Plasmoid.configuration.colorizeUsage || value <= 0)
            return colors.textColor
        const t = Math.max(0, Math.min(1, value / 100))
        return Qt.hsla((1 - t) * 0.33, 0.62, Style.isDark(colors) ? 0.62 : 0.42, 1)
    }
    function colColor(p, column, theme) {
        const colors = theme || Kirigami.Theme
        if (column.kind === "fps" && p.fps !== undefined) return fpsColor(p.fps, colors)
        return column.heat && column.kind === "pct" ? heatColor(colVal(p, column), colors) : colors.textColor
    }
    function graphMax(key) {
        if (key === "ram") return summary.memTotal
        if (key === "vram") return summary.vramTotal
        if (key === "fps") return 0
        const column = colOf(key)
        return column && column.kind === "pct" ? 100 : 0
    }
    function fpsColor(fps, theme) {
        const colors = theme || Kirigami.Theme
        if (fps >= Plasmoid.configuration.fpsGood) return colors.positiveTextColor
        if (fps >= Plasmoid.configuration.fpsWarn) return colors.neutralTextColor
        return colors.negativeTextColor
    }
}
