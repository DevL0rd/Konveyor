var procs = []
var byPid = {}
var childrenOf = {}
var hist = {}
var focusHist = {}
var focusHistPid = 0
var memTotal = 0
var vramTotal = 0
var ncpu = 1

function ringMake(cap) { return { buf: new Array(cap), head: 0, len: 0, cap: cap } }
function ringPush(r, v) { r.buf[r.head] = v; r.head = (r.head + 1) % r.cap; if (r.len < r.cap) r.len++ }
function ringValues(r) {
    var n = r.len, out = new Array(n), start = (r.head - n + r.cap) % r.cap
    for (var i = 0; i < n; i++) out[i] = r.buf[(start + i) % r.cap]
    return out
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

function frameInfo(pid) {
    var best = null
    var stack = [pid], seen = {}
    while (stack.length > 0) {
        var current = stack.pop()
        if (seen[current]) continue
        seen[current] = true
        var p = byPid[current]
        if (p && p.fps !== undefined && (best === null || p.fps > best.fps)) best = p
        var kids = childrenOf[current]
        if (kids) for (var i = 0; i < kids.length; i++) stack.push(kids[i])
    }
    return best ? { fps: best.fps, frametime: best.frametime, fpsLow: best.fps_low } : null
}

function focusInfo(pid) {
    var p = byPid[pid]
    if (!p) return null
    var frames = frameInfo(pid)
    return {
        pid: p.pid, ppid: p.ppid, name: p.name, icon: p.icon || "",
        parentName: byPid[p.ppid] ? byPid[p.ppid].name : "",
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
    var cpu = 0, vram = 0, gpuTop = null, cpuTop = null
    for (var i = 0; i < procs.length; i++) {
        var p = procs[i]
        cpu += p.cpu || 0
        vram += p.vram || 0
        if ((p.gpu || 0) > 0 && (gpuTop === null || p.gpu > gpuTop.gpu)) gpuTop = p
        if (!p.kernel && (p.acpu || 0) > 0 && p.pid !== 1 && p.name !== "systemd" && (cpuTop === null || p.cpu > cpuTop.cpu)) cpuTop = p
    }
    return {
        count: procs.length, cpu: Math.min(100, cpu), vram: vram, memTotal: memTotal, vramTotal: vramTotal, ncpu: ncpu,
        gpuTop: gpuTop ? { pid: gpuTop.pid, name: gpuTop.name, gpu: gpuTop.gpu } : null,
        topCpu: cpuTop ? cpuTop.pid : 0
    }
}

function build(s) {
    var sc = s.sortColumn, agg = s.aggregate, sk = s.showKernel, hs = s.hideSystemd, noagg = s.sortNoagg
    function sortVal(p) { return sc === "name" ? (p.name || "").toLowerCase() : colVal(p, sc, noagg, agg) }
    function cmp(a, b) {
        var av = sortVal(a), bv = sortVal(b)
        var r = av < bv ? -1 : (av > bv ? 1 : 0)
        return s.sortDescending ? -r : r
    }
    function histArr(pid) {
        if (sc === "name" || sc === "pid") return []
        var h = hist[pid]
        return (h && h[sc]) ? ringValues(h[sc]) : []
    }
    var desired = [], pbp = {}, shb = {}
    function emit(p, depth, has, exp) {
        pbp[p.pid] = p
        shb[p.pid] = histArr(p.pid)
        desired.push({ pid: p.pid, depth: depth, hasChildren: has, expanded: exp })
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
    return { desired: desired, procByPid: pbp, sortHistByPid: shb }
}

WorkerScript.onMessage = function(msg) {
    var s = msg.state
    if (msg.text) {
        var d
        try { d = JSON.parse(msg.text) } catch (e) { return }
        var ps = d.procs || []
        memTotal = d.mem_total || 0
        vramTotal = d.vram_total || 0
        ncpu = d.ncpu || 1
        var keys = s.histKeys
        var fields = s.aggregate ? keys.map(function(k) { return "a" + k }) : keys
        var bp = {}, ch = {}, nh = {}
        for (var k = 0; k < ps.length; k++) {
            var pp = ps[k]
            bp[pp.pid] = pp
            ;(ch[pp.ppid] = ch[pp.ppid] || []).push(pp.pid)
            var hh = hist[pp.pid] || {}
            for (var m = 0; m < keys.length; m++) {
                var key = keys[m]
                var r = hh[key] || (hh[key] = ringMake(s.histLen))
                ringPush(r, pp[fields[m]] || 0)
            }
            nh[pp.pid] = hh
        }
        procs = ps; byPid = bp; childrenOf = ch
        hist = nh
        recordFocus(focusInfo(s.focusPid), s.histLen)
    }
    var out = { focus: focusInfo(s.focusPid), focusHistory: focusHistory(), summary: summary(), full: s.full }
    if (s.full) {
        var built = build(s)
        out.desired = built.desired
        out.procByPid = built.procByPid
        out.sortHistByPid = built.sortHistByPid
    }
    WorkerScript.sendMessage(out)
}
