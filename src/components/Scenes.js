.pragma library

const GAP = 0.035;
const TOP = 0.08;
const HEIGHT = 0.84;
const ROW_GAP = 0.05;
const PAUSE = 400;
const HOLD = 700;
const RETURN = 500;
const DEFAULTS = { f: 0, o: 1, s: 1, z: 0, tabs: 0, tab: 0, alt: 0, label: "" };

function presetWidth(proportion) {
    return proportion * (1 - GAP) - GAP;
}

function columns(list, options) {
    const opts = options || {};
    const result = {};
    let x = (opts.monitor || 0) + (opts.offset || 0) + GAP;
    for (const column of list) {
        const keys = column.keys || [column.key];
        const tabbed = column.tabbed === true;
        const rows = tabbed ? 1 : keys.length;
        const weights = column.heights || keys.map(() => 1);
        const total = weights.reduce((sum, weight) => sum + weight, 0);
        const free = HEIGHT - ROW_GAP * (rows - 1);
        let y = (opts.workspace || 0) + TOP;
        keys.forEach((key, index) => {
            const h = tabbed ? HEIGHT : free * weights[index] / total;
            const active = keys.indexOf(column.focus) >= 0 ? keys.indexOf(column.focus) : 0;
            result[key] = Object.assign({}, DEFAULTS, {
                x: x,
                y: y,
                w: column.w,
                h: h,
                f: column.focus === key ? 1 : 0,
                o: tabbed && index !== active ? 0 : 1,
                tabs: tabbed ? keys.length : 0,
                tab: tabbed ? active : 0,
                alt: column.alt ? 1 : 0,
                label: column.label || ""
            });
            if (!tabbed) {
                y += h + ROW_GAP;
            }
        });
        x += column.w + GAP;
    }
    return result;
}

function scrollFrames(widths, mode) {
    const starts = [];
    let x = GAP;
    widths.forEach(w => {
        starts.push(x);
        x += w + GAP;
    });
    let offset = 0;
    return widths.map((w, focus) => {
        const start = starts[focus];
        const neighbor = focus + 1 < widths.length ? widths[focus + 1] + GAP : 0;
        const center = mode === "always" || (mode === "on-overflow" && w + neighbor + 2 * GAP > 1);
        if (center) {
            offset = 0.5 - start - w / 2;
        } else if (start + w + GAP + offset > 1) {
            offset = 1 - start - w - GAP;
        } else if (start - GAP + offset < 0) {
            offset = GAP - start;
        }
        return columns(widths.map((width, index) => ({ key: "c" + index, w: width, focus: index === focus ? "c" + index : "" })), { offset: offset });
    });
}

function merge() {
    return Object.assign.apply(null, [{}].concat(Array.from(arguments)));
}

function scene(frames, options) {
    const opts = options || {};
    const keys = [];
    frames.forEach(frame => Object.keys(frame).forEach(key => {
        if (keys.indexOf(key) < 0) {
            keys.push(key);
        }
    }));
    const windows = keys.map(key => {
        const present = frames.map(frame => frame[key] || null);
        return present.map((rect, index) => {
            if (rect) {
                return Object.assign({}, DEFAULTS, rect);
            }
            let nearest = null;
            for (let distance = 1; distance < frames.length && !nearest; ++distance) {
                nearest = present[index - distance] || present[index + distance] || null;
            }
            return Object.assign({}, DEFAULTS, nearest, { o: 0, s: 0.8, f: 0 });
        });
    });
    return {
        windows: windows,
        homes: windows.map(list => Math.floor(list[0].y)),
        count: frames.length,
        cycle: opts.cycle === true,
        monitors: opts.monitors || 1,
        stackedMonitors: opts.stackedMonitors === true,
        key: opts.key || "",
        panel: opts.panel || null,
        still: opts.still === undefined ? Math.min(1, frames.length - 1) : opts.still
    };
}

function moveDuration(built) {
    return built.count > 2 || built.cycle ? 1000 : 1400;
}

function moves(built) {
    return built.cycle ? built.count : built.count - 1;
}

function duration(built) {
    return PAUSE + moves(built) * (moveDuration(built) + HOLD) + (built.cycle ? 0 : RETURN);
}

function inOutCubic(t) {
    return t < 0.5 ? 4 * t * t * t : 1 - Math.pow(-2 * t + 2, 3) / 2;
}

function inOutQuad(t) {
    return t < 0.5 ? 2 * t * t : 1 - Math.pow(-2 * t + 2, 2) / 2;
}

function glowBefore(remaining) {
    return Math.max(0, 1 - remaining / 220);
}

function cursor(built, clock) {
    const move = moveDuration(built);
    const count = built.count;
    let time = clock - PAUSE;
    if (time < 0) {
        return { from: 0, to: 0, t: 0, glow: glowBefore(-time) };
    }
    for (let i = 0; i < moves(built); ++i) {
        const next = (i + 1) % count;
        if (time < move) {
            return { from: i, to: next, t: inOutCubic(time / move), glow: Math.max(0, 1 - time / 420) };
        }
        time -= move;
        if (time < HOLD) {
            const last = i === moves(built) - 1 && !built.cycle;
            return { from: next, to: next, t: 0, glow: last ? 0 : glowBefore(HOLD - time) };
        }
        time -= HOLD;
    }
    return { from: count - 1, to: 0, t: inOutQuad(Math.min(1, time / RETURN)), glow: 0 };
}

function still(built) {
    return { from: built.still, to: built.still, t: 0, glow: 0 };
}

function lerp(a, b, t) {
    return a + (b - a) * t;
}

function rect(built, index, at) {
    const frames = built.windows[index];
    const a = frames[at.from];
    const b = frames[at.to];
    const t = at.t;
    return {
        x: lerp(a.x, b.x, t),
        y: lerp(a.y, b.y, t),
        w: lerp(a.w, b.w, t),
        h: lerp(a.h, b.h, t),
        f: lerp(a.f, b.f, t),
        o: lerp(a.o, b.o, t),
        s: lerp(a.s, b.s, t),
        z: Math.max(a.z, b.z),
        tabs: Math.max(a.tabs, b.tabs),
        tab: t < 0.5 ? a.tab : b.tab,
        tabbed: lerp(a.tabs > 0 ? 1 : 0, b.tabs > 0 ? 1 : 0, t),
        alt: lerp(a.alt, b.alt, t),
        label: t < 0.5 ? a.label : b.label,
        home: built.homes[index]
    };
}

function panel(built, at) {
    return built.panel ? lerp(built.panel[at.from], built.panel[at.to], at.t) : 0;
}
