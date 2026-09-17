var procs = []
var framed = []
var byPid = null
var childrenOf = null
var hist = new Map()
var histGen = 0
var HIST_KEYS = 8
var focusHist = {}
var focusHistPid = 0
var memTotal = 0
var vramTotal = 0
var ncpu = 1
var lastRowSig = ""

function ringMake(cap) { return { buf: new Array(cap), head: 0, len: 0, cap: cap } }
function ringPush(r, v) { r.buf[r.head] = v; r.head = (r.head + 1) % r.cap; if (r.len < r.cap) r.len++ }
function ringValues(r) {
    var n = r.len, out = new Array(n), start = (r.head - n + r.cap) % r.cap
    for (var i = 0; i < n; i++) out[i] = r.buf[(start + i) % r.cap]
    return out
}
function histMake(cap) { return { buf: new Float64Array(cap * HIST_KEYS), head: 0, len: 0, cap: cap, gen: 0 } }
function histValues(h, keyIndex) {
    var n = h.len, cap = h.cap, out = new Array(n), start = (h.head - n + cap) % cap, base = keyIndex
    for (var i = 0; i < n; i++) out[i] = h.buf[((start + i) % cap) * HIST_KEYS + base]
    return out
}
function bytesText(b) {
    b = b || 0
    if (b >= 1099511627776) return (b / 1099511627776).toFixed(1) + "T"
    if (b >= 1073741824) return (b / 1073741824).toFixed(1) + "G"
    if (b >= 1048576) return Math.round(b / 1048576) + "M"
    if (b >= 1024) return Math.round(b / 1024) + "K"
    return Math.round(b) + "B"
}
function colVal(p, key, noagg, aggregate) {
    if (noagg) return p[key] || 0
    if (aggregate) { var a = p["a" + key]; return a === undefined ? (p[key] || 0) : a }
    return p[key] || 0
}
function passes(p, showKernel) { return p && (showKernel || !p.kernel) }
function isHidden(p, hideSystemd) { return hideSystemd && (p.pid === 1 || p.name === "systemd") }

function matchesFilter(p, filter) {
    if (filter === "apps") return !!p.icon
    if (filter === "gpu") return (p.agpu || p.gpu || p.avram || p.vram || 0) > 0
    if (filter === "disk") return (p.adisk || p.disk || 0) > 0
    return true
}

function indexProcs() {
    if (byPid) return
    var bp = {}, ch = {}
    for (var k = 0; k < procs.length; k++) {
        var pp = procs[k]
        bp[pp.pid] = pp
        var siblings = ch[pp.ppid]
        if (siblings) siblings.push(pp.pid)
        else ch[pp.ppid] = [pp.pid]
    }
    byPid = bp
    childrenOf = ch
}

function findProc(pid) {
    if (byPid) return byPid[pid]
    for (var i = 0; i < procs.length; i++) {
        if (procs[i].pid === pid) return procs[i]
    }
    return undefined
}

function frameInfo(pid) {
    var best = null
    for (var i = 0; i < framed.length; i++) {
        var p = framed[i]
        var current = p, steps = 0
        while (current && steps < 64) {
            if (current.pid === pid) {
                if (best === null || p.fps > best.fps) best = p
                break
            }
            current = current.ppid > 0 ? findProc(current.ppid) : undefined
            steps++
        }
    }
    return best ? { fps: best.fps, frametime: best.frametime, fpsLow: best.fps_low } : null
}

function focusInfo(pid) {
    var p = findProc(pid)
    if (!p) return null
    var frames = frameInfo(pid)
    var parent = p.ppid > 0 ? findProc(p.ppid) : undefined
    return {
        pid: p.pid, ppid: p.ppid, name: p.name, icon: p.icon || "",
        parentName: parent ? parent.name : "",
        cpu: p.acpu !== undefined ? p.acpu : (p.cpu || 0),
        gpu: p.agpu !== undefined ? p.agpu : (p.gpu || 0),
        ram: p.aram !== undefined ? p.aram : (p.ram || 0),
        vram: p.avram !== undefined ? p.avram : (p.vram || 0),
        disk: p.adisk !== undefined ? p.adisk : (p.disk || 0),
        threads: p.athreads !== undefined ? p.athreads : (p.threads || 0),
        enc: p.aenc || p.enc || 0, dec: p.adec || p.dec || 0,
        fps: frames ? frames.fps : -1, frametime: frames ? frames.frametime : 0, fpsLow: frames ? frames.fpsLow : 0
    }
}

function recordFocus(info, len) {
    if (!info) return
    if (info.pid !== focusHistPid) {
        focusHist = {}
        focusHistPid = info.pid
    }
    var keys = ["cpu", "gpu", "ram", "vram", "disk", "threads", "enc", "dec", "fps", "frametime"]
    for (var i = 0; i < keys.length; i++) {
        var r = focusHist[keys[i]] || (focusHist[keys[i]] = ringMake(len))
        ringPush(r, Math.max(0, info[keys[i]] || 0))
    }
}

function focusHistory() {
    var out = {}
    for (var key in focusHist) out[key] = ringValues(focusHist[key])
    return out
}

function summary() {
    var cpu = 0, gpu = 0, vram = 0, gpuTop = null
    for (var i = 0; i < procs.length; i++) {
        var p = procs[i]
        cpu += p.cpu || 0
        gpu += p.gpu || 0
        vram += p.vram || 0
        if ((p.gpu || 0) > 0 && (gpuTop === null || p.gpu > gpuTop.gpu)) gpuTop = p
    }
    return {
        count: procs.length, cpu: Math.min(100, cpu), gpu: Math.min(100, gpu), vram: vram, memTotal: memTotal, vramTotal: vramTotal, ncpu: ncpu,
        gpuTop: gpuTop ? { pid: gpuTop.pid, name: gpuTop.name, gpu: gpuTop.gpu } : null
    }
}

function build(s) {
    indexProcs()
    var sc = s.sortColumn, agg = s.aggregate, sk = s.showKernel, hs = s.hideSystemd, noagg = s.sortNoagg
    function sortVal(p) { return sc === "name" ? (p.name || "").toLowerCase() : colVal(p, sc, noagg, agg) }
    function cmp(a, b) {
        var av = sortVal(a), bv = sortVal(b)
        var r = av < bv ? -1 : (av > bv ? 1 : 0)
        return s.sortDescending ? -r : r
    }
    var sortKeyIndex = s.histKeys.indexOf(sc)
    function histArr(pid) {
        if (sortKeyIndex < 0) return []
        var h = hist.get(pid)
        return h ? histValues(h, sortKeyIndex) : []
    }
    var desired = [], pbp = {}, shb = {}, sig = []
    var first = s.windowStart, last = s.windowEnd, shown = {}
    for (var w = 0; w < s.windowPids.length; w++) shown[s.windowPids[w]] = true
    function emit(p, depth, has, exp) {
        var index = desired.length
        if ((index >= first && index <= last) || shown[p.pid] === true) {
            pbp[p.pid] = p
            shb[p.pid] = histArr(p.pid)
        }
        desired.push({ pid: p.pid, depth: depth, hasChildren: has, expanded: exp })
        sig.push(p.pid + (has ? (exp ? "e" : "c") : "") + depth)
    }
    var flat = s.searchText !== "" || s.filter !== "all" || !s.tree
    if (flat) {
        var ql = s.searchText.toLowerCase()
        var mm = procs.filter(function(p) {
            if (!passes(p, sk) || !matchesFilter(p, s.filter)) return false
            if (!s.tree && ql === "" && isHidden(p, hs)) return false
            return ql === "" || (p.name || "").toLowerCase().indexOf(ql) >= 0 || String(p.pid) === ql
        })
        mm.sort(cmp)
        for (var i = 0; i < mm.length; i++) emit(mm[i], 0, false, false)
    } else {
        var rl = procs.filter(function(p) {
            if (!passes(p, sk) || isHidden(p, hs)) return false
            var par = byPid[p.ppid]
            return par === undefined || isHidden(par, hs) || !passes(par, sk)
        })
        rl.sort(cmp)
        var expanded = s.expanded || {}
        var walk = function(p, depth) {
            var cids = childrenOf[p.pid], kids = []
            if (cids) for (var c = 0; c < cids.length; c++) {
                var kk = byPid[cids[c]]
                if (kk && passes(kk, sk) && !isHidden(kk, hs)) kids.push(kk)
            }
            var has = kids.length > 0, exp = expanded[p.pid] === true
            emit(p, depth, has, exp)
            if (has && exp) { kids.sort(cmp); for (var i = 0; i < kids.length; i++) walk(kids[i], depth + 1) }
        }
        for (var j = 0; j < rl.length; j++) walk(rl[j], 0)
    }
    return { desired: desired, procByPid: pbp, sortHistByPid: shb, sig: sig.join(",") }
}

function readSnapshot(path) {
    var xhr = new XMLHttpRequest()
    xhr.open("GET", "file://" + path, false)
    xhr.send()
    return xhr.responseText
}

function ingest(text, s) {
    var d
    try { d = JSON.parse(text) } catch (e) { return false }
    var ps = d.procs || []
    memTotal = d.mem_total || 0
    vramTotal = d.vram_total || 0
    ncpu = d.ncpu || 1
    var keys = s.histKeys
    var fields = s.aggregate ? keys.map(function(k) { return "a" + k }) : keys
    var f0 = fields[0], f1 = fields[1], f2 = fields[2], f3 = fields[3], f4 = fields[4], f5 = fields[5], f6 = fields[6], f7 = fields[7]
    var cap = s.histLen
    var gen = ++histGen
    var fr = []
    for (var k = 0; k < ps.length; k++) {
        var pp = ps[k]
        if (pp.fps !== undefined) fr.push(pp)
        var h = hist.get(pp.pid)
        if (!h || h.cap !== cap) {
            h = histMake(cap)
            hist.set(pp.pid, h)
        }
        h.gen = gen
        var buf = h.buf, o = h.head * HIST_KEYS
        buf[o] = pp[f0] || 0
        buf[o + 1] = pp[f1] || 0
        buf[o + 2] = pp[f2] || 0
        buf[o + 3] = pp[f3] || 0
        buf[o + 4] = pp[f4] || 0
        buf[o + 5] = pp[f5] || 0
        buf[o + 6] = pp[f6] || 0
        buf[o + 7] = pp[f7] || 0
        h.head = (h.head + 1) % cap
        if (h.len < cap) h.len++
    }
    if (hist.size > ps.length) {
        hist.forEach(function(entry, pid) { if (entry.gen !== gen) hist.delete(pid) })
    }
    procs = ps
    framed = fr
    byPid = null
    childrenOf = null
    return true
}

function compactSignature(focus, sum) {
    if (!focus)
        return "-|" + sum.count + "|" + Math.round(sum.cpu) + "|" + Math.round(sum.gpu)
    return focus.pid + "|" + focus.name + "|" + focus.icon + "|" + focus.parentName + "|" + Math.round(focus.cpu) + "|" + Math.round(focus.gpu)
        + "|" + bytesText(focus.vram) + "|" + bytesText(focus.ram) + "|" + focus.fps + "|" + focus.frametime.toFixed(1) + "|" + focus.fpsLow
}

WorkerScript.onMessage = function(msg) {
    var s = msg.state
    var focus
    if (msg.panel) {
        var compact
        try { compact = JSON.parse(readSnapshot(msg.panel)) } catch (e) { return }
        WorkerScript.sendMessage({ focus: compact.focus, summary: compact.summary, full: false,
                                   compactSig: compactSignature(compact.focus, compact.summary) })
        return
    }
    if (msg.path) {
        var text = readSnapshot(msg.path)
        if (!text || !ingest(text, s)) return
        focus = focusInfo(s.focusPid)
        recordFocus(focus, s.histLen)
    } else if (msg.text) {
        if (!ingest(msg.text, s)) return
        focus = focusInfo(s.focusPid)
        recordFocus(focus, s.histLen)
    } else {
        focus = focusInfo(s.focusPid)
    }
    var sum = summary()
    var out = { focus: focus, summary: sum, full: s.full, compactSig: compactSignature(focus, sum) }
    if (s.full) out.focusHistory = focusHistory()
    if (s.full) {
        var built = build(s)
        if (built.sig !== lastRowSig) {
            lastRowSig = built.sig
            out.desired = built.desired
        }
        out.procByPid = built.procByPid
        out.sortHistByPid = built.sortHistByPid
    } else {
        lastRowSig = ""
    }
    WorkerScript.sendMessage(out)
}
