.pragma library

const presets = [
    { name: "neon-sweep", label: "Neon sweep", tooltip: "Rotating neon colors with a ripple on focus", layers: [
        ["sweep", { colors: ["#ff2fd0", "#2fd6ff", "accent"], speed: 1, blend: "add" }],
        ["ripple", { colors: ["#ffffff"], speed: 1, opacity: 0.8, blend: "add" }]] },
    { name: "comet-trail", label: "Comet trail", tooltip: "Two comets chasing around a dark frame", layers: [
        ["gradient", { colors: ["#202028"], speed: 0 }],
        ["comet", { colors: ["#ffffff", "accent"], count: 2, tail: 0.3, speed: 0.8, blend: "add" }]] },
    { name: "soft-glow", label: "Soft glow", tooltip: "An accent border that breathes", layers: [
        ["gradient", { colors: ["accent"], speed: 0 }],
        ["glow", { colors: ["accent"], width: 18, speed: 1, opacity: 0.7, blend: "add" }]] },
    { name: "embers", label: "Embers", tooltip: "Warm colors with rising sparks", layers: [
        ["gradient", { colors: ["#ff5a1f", "#ffb347"], angle: 90, speed: 0.5 }],
        ["particles", { colors: ["#ffd27a", "#ff5a1f"], count: 64, rate: 40, size: 2.5, lifetime: 1.4, speed: 0.8, direction: "up", blend: "add" }]] },
    { name: "alert", label: "Alert", tooltip: "A red pulse with marching dashes, for urgent windows", layers: [
        ["pulse", { colors: ["#ff1f1f", "#ff8a1f"], speed: 1.5 }],
        ["dashes", { colors: ["#ffffff"], dash: 14, gap: 10, speed: 1, opacity: 0.9 }]] },
    { name: "siren-sweep", label: "Siren sweep", tooltip: "Red and blue chasing each other, for urgent windows", layers: [
        ["sweep", { colors: ["#ff1f1f", "#1f4bff"], speed: 2, blend: "add" }],
        ["pulse", { colors: ["#ffffff"], speed: 2, opacity: 0.35 }]] },
    { name: "marching-ants", label: "Marching ants", tooltip: "Amber dashes marching over a dark frame", layers: [
        ["gradient", { colors: ["#202028"], speed: 0 }],
        ["dashes", { colors: ["#ffc21f"], dash: 10, gap: 8, speed: 1 }]] },
    { name: "aurora", label: "Aurora", tooltip: "Slow northern lights colors with a gentle glow", layers: [
        ["gradient", { colors: ["#2fffb0", "#2f7bff", "#b14bff"], angle: 30, speed: 0.25 }],
        ["glow", { colors: ["#2fffb0", "#b14bff"], width: 14, speed: 0.5, opacity: 0.5, blend: "screen" }]] },
    { name: "circuit-trace", label: "Circuit trace", tooltip: "Short sparks of light running along a dark frame", layers: [
        ["gradient", { colors: ["#101820"], speed: 0 }],
        ["comet", { colors: ["#2fd6ff"], count: 4, tail: 0.12, speed: 1.2, blend: "add" }]] },
    { name: "rainbow", label: "Rainbow", tooltip: "Every color, rotating", layers: [
        ["sweep", { colors: ["#ff3b3b", "#ffb03b", "#f4ff3b", "#3bff6a", "#3bc8ff", "#b43bff"], speed: 0.5 }]] },
    { name: "equalizer", label: "Equalizer", group: "Audio", tooltip: "Bars dancing around the window", layers: [
        ["gradient", { colors: ["accent"], speed: 0, opacity: 0.5 }],
        ["equalizer", { colors: ["#2fd6ff", "#ff2fd0"], width: 18, blend: "add" }]] },
    { name: "bass-club", label: "Bass club", group: "Audio", tooltip: "A thumping neon frame with sparks on the beat", layers: [
        ["bass-thump", { colors: ["#ff2fd0", "#ffffff"], width: 6, blend: "add" }],
        ["particles", { colors: ["#ffffff", "#ff2fd0"], count: 96, burst: 24, size: 2.5, lifetime: 0.8, speed: 1.5, blend: "add", "fire-on": ["audio.beat"] }]] },
    { name: "waveform-neon", label: "Waveform neon", group: "Audio", tooltip: "The sound as a neon line", layers: [
        ["gradient", { colors: ["#101820"], speed: 0 }],
        ["waveform", { colors: ["#2fffb0", "#2f7bff"], width: 16, thickness: 2.5, blend: "add" }]] },
    { name: "beat-embers", label: "Beat embers", group: "Audio", tooltip: "Warm sparks rising on the beat, glowing with the volume", layers: [
        ["gradient", { colors: ["#401000", "#ff5a1f", "#ffd27a"], speed: 0, width: 4, drives: [
            { setting: "color-mix", signal: "audio.level", amount: 1, smooth: 0.15 }] }],
        ["particles", { colors: ["#ffd27a", "#ff5a1f"], count: 128, burst: 32, size: 2.5, lifetime: 1.2, speed: 1, direction: "up", blend: "add", "fire-on": ["audio.beat"] }]] },
    { name: "spectrum", label: "Spectrum", group: "Audio", tooltip: "Lows at the bottom, highs at the top", layers: [
        ["spectrum", { colors: ["#ff3b3b", "#ffb03b", "#3bff6a", "#3bc8ff"], width: 5 }]] },
    { name: "heat", label: "Heat", group: "Audio", tooltip: "Cold when quiet, white hot when loud", layers: [
        ["gradient", { colors: ["#1f4bff", "#ff5a1f", "#ffe9a0"], speed: 0, width: 5, drives: [
            { setting: "color-mix", signal: "audio.level", amount: 1, smooth: 0.15 }] }]] },
    { name: "volume-glow", label: "Volume glow", group: "Audio", tooltip: "A soft glow that grows with the volume", layers: [
        ["glow", { colors: ["accent"], width: 6, speed: 0, blend: "add", drives: [
            { setting: "width", signal: "audio.level", amount: 20, smooth: 0.08 },
            { setting: "opacity", signal: "audio.level", amount: 0.6, smooth: 0.08 }] }]] },
    { name: "beat-sparks", label: "Beat sparks", group: "Audio", tooltip: "A burst of sparks on every beat", layers: [
        ["particles", { colors: ["#ffffff", "accent"], count: 96, burst: 24, size: 2.5, lifetime: 0.9, speed: 1.5, blend: "add", "fire-on": ["audio.beat"] }]] },
    { name: "beat-ripple", label: "Beat ripple", group: "Audio", tooltip: "A wave around the frame on every beat", layers: [
        ["ripple", { colors: ["#ffffff", "accent"], speed: 1.5, blend: "add", "fire-on": ["audio.beat"] }]] },
    { name: "treble-shimmer", label: "Treble shimmer", group: "Audio", tooltip: "Fine sparkle that follows the highs", layers: [
        ["gradient", { colors: ["accent"], speed: 0, opacity: 0.5 }],
        ["shimmer", { colors: ["#ffffff", "#9fd8ff"], width: 6, blend: "add" }]] },
    { name: "tempo-comet", label: "Tempo comet", group: "Audio", tooltip: "A comet that runs faster with the music and kicks on the beat", layers: [
        ["gradient", { colors: ["#202028"], speed: 0 }],
        ["comet", { colors: ["#ffffff", "accent"], count: 2, tail: 0.3, speed: 0.2, blend: "add", drives: [
            { setting: "speed", signal: "audio.level", amount: 1.6, smooth: 0.15 },
            { setting: "speed", signal: "audio.beat", amount: 1.5, smooth: 0.02 }] }]] },
    { name: "scroll-comet", label: "Scroll comet", group: "Scroll", tooltip: "A comet that runs the way you scroll", layers: [
        ["comet", { colors: ["#ffffff", "accent"], count: 1, tail: 0.3, speed: 0.05, blend: "add", drives: [
            { setting: "speed", signal: "scroll.speed", amount: 3, smooth: 0.1 },
            { setting: "direction", signal: "scroll.direction", amount: 1 }] }]] },
    { name: "speed-lines", label: "Speed lines", group: "Scroll", tooltip: "Streaks that race along the edges as you scroll", layers: [
        ["speed-lines", { colors: ["#ffffff", "accent"], width: 8, blend: "add" }]] },
    { name: "scroll-trail", label: "Scroll trail", group: "Scroll", tooltip: "A light that follows how far you scroll", layers: [
        ["scroll-trail", { colors: ["#ffffff", "accent"], width: 5, blend: "add" }]] },
    { name: "typing-sparks", label: "Typing sparks", group: "Input", tooltip: "A few sparks with every keypress", layers: [
        ["particles", { colors: ["#ffffff", "accent"], count: 64, burst: 8, size: 2, lifetime: 0.6, speed: 1.2, blend: "add", "fire-on": ["keypress"] }]] },
    { name: "click-ripple", label: "Click ripple", group: "Input", tooltip: "A wave around the frame on every click", layers: [
        ["ripple", { colors: ["#ffffff", "accent"], speed: 1.2, blend: "add", "fire-on": ["click"] }]] },
    { name: "focus-pulse", label: "Focus pulse", group: "Focus", tooltip: "A light front spreading out from the window you move to", layers: [
        ["pulse-front", { colors: ["#ffffff", "accent"], opacity: 0.85, blend: "add" }]] },
    { name: "focus-wave", label: "Focus wave", group: "Focus", tooltip: "A band of light from the old window to the new one", layers: [
        ["wave", { colors: ["#ffffff", "accent"], blend: "add" }]] },
    { name: "focus-draw-in", label: "Focus draw in", group: "Focus", tooltip: "The frame traces itself in when the window gets focus", layers: [
        ["draw-in", { colors: ["#ffffff", "accent"], blend: "add" }]] },
    { name: "unfocus-fade", label: "Unfocus fade", group: "Focus", tooltip: "The frame lights up and fades as focus leaves", layers: [
        ["fade-out", { colors: ["accent"], blend: "add" }]] },
    { name: "scroll-flare", label: "Scroll flare", group: "Scroll", tooltip: "A glow that flares while you scroll", layers: [
        ["glow", { colors: ["accent"], width: 4, speed: 0, blend: "add", drives: [
            { setting: "width", signal: "scroll.speed", amount: 18, smooth: 0.1 },
            { setting: "opacity", signal: "scroll.speed", amount: 0.7, smooth: 0.1 }] }]] },
    { name: "bulge", label: "Bulge", group: "Scroll", tooltip: "Round bumps swell out and travel along the edges as you scroll", layers: [
        ["bulge", { colors: ["accent", "#ffffff"], width: 22 }]] },
    { name: "wavy-line", label: "Wavy line", group: "Scroll", tooltip: "The border undulates as you scroll", layers: [
        ["wavy", { colors: ["#ffffff", "accent"], width: 12, thickness: 3, blend: "add" }]] },
    { name: "elastic", label: "Elastic", group: "Scroll", tooltip: "The edge stretches the way you scroll and springs back", layers: [
        ["elastic", { colors: ["accent", "#ffffff"], width: 28, thickness: 3, blend: "add" }]] },
    { name: "chevrons", label: "Chevrons", group: "Scroll", tooltip: "Arrows race along the sides the way you scroll", layers: [
        ["chevrons", { colors: ["#ffffff", "accent"], width: 20, thickness: 3, blend: "add" }]] },
    { name: "edge-flash", label: "Edge flash", group: "Scroll", tooltip: "The top or bottom edge lights up the way you scroll", layers: [
        ["edge-flash", { colors: ["#ffffff", "accent"], width: 14, blend: "add" }]] },
    { name: "gauge", label: "Gauge", group: "Scroll", tooltip: "A marker slides along the right edge as you scroll", layers: [
        ["gauge", { colors: ["#ffffff", "accent"], width: 8 }]] },
    { name: "speed-color", label: "Speed color", group: "Input", tooltip: "The border runs from the accent color to hot as the pointer speeds up", layers: [
        ["gradient", { colors: ["accent", "#ff3b1f"], speed: 0, drives: [
            { setting: "color-mix", signal: "pointer.speed", amount: 1, smooth: 0.1 }] }]] }
];
