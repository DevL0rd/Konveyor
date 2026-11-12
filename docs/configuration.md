# Configuration

Press <kbd>Meta</kbd> + <kbd>K</kbd> and open **Settings** at the bottom of the Kontrol Panel's sidebar, or press the gear (<kbd>Ctrl</kbd> + <kbd>,</kbd>). Every option has a page there: layout, appearance, motion, mouse, touch and gestures, shortcuts, window rules, monitors, workspaces, Plasma integration and experiments. Changes apply instantly, and Undo (<kbd>Ctrl</kbd> + <kbd>Z</kbd>) takes them back. `portal-launcher settings` opens the page directly. The same pages are in System Settings under Window Management → Konveyor, behind Configure next to Konveyor under Desktop Effects, and `kcmshell6 kcm_konveyor` opens them on their own (add `--args rules` to start on a page), so they work without the widgets too.

The settings page edits `~/.config/konveyor/config.kdl` in place and keeps your comments and formatting, so you can use both.

Everything lives in `~/.config/konveyor/config.kdl`, a [KDL](https://kdl.dev) file. The shipped default config lists every option with a short explanation. Save the file and the change is live — no restart, no logout.

```kdl
layout {
    gaps 16
    center-focused-column "never"
    default-column-width { proportion 0.5; }

    focus-ring {
        width 2
        active-color "accent"
    }
}

binds {
    Mod+Left  { focus-column-left; }
    Mod+Right { focus-column-right; }
    Mod+R     { switch-preset-column-width; }
}
```

You get gaps, borders, a focus ring, tab indicators, preset widths, centering modes, per-monitor and per-workspace overrides, window rules, animation springs and key binds. Two KDE-specific extras: `active-color "accent"` follows the KDE accent color, and `gestures { titlebar-drag "scroll-view" }` scrolls the row when you drag a window by its title bar instead of moving the window. Dragging a tiled window's edge resizes its column; `gestures { resize-tiled-windows false; }` turns that off.

Layout settings come in four levels: `layout` for every monitor, the first `monitor-profile` a monitor matches, the monitor's own `output` block, and a named `workspace`. Each level changes only the settings it lists and takes everything else from the level before it. An `output` block that only sets `gaps` keeps its profile's column widths, and a workspace that only sets `gaps` keeps the struts and borders of the monitor it is on.

```kdl
monitor-profile "portrait" {
    match aspect-ratio-below=1.0
    layout { default-column-width { proportion 1.0; }; }
}

output "DP-2" {
    layout { gaps 8; }
}
```

## Animated borders

`focus-ring` and `border` can use a named `border-style` instead of a flat color. A style is a stack of layers drawn bottom to top, so you can mix any of them and add or remove them freely. The file's top level holds the styles.

The default config ships two: `Default-Style` for the focused window's ring and `Default-Attention-Style` for windows that want attention, on a 2 pixel wide `focus-ring`. Existing configs that still have the old default ring (width 4, accent color, no style) get the same setup once; anything you changed is kept.

Settings → Appearance edits all of this without touching the file by hand: styles, their layers and colors, ready-made starting points, and which style the focus ring and border use for each state, including per-monitor and profile overrides. Window rule styles are set in the file.

```kdl
border-style "embers" {
    layer "gradient" { colors "#ff5a1f" "#ffb347"; angle 90; speed 0.5; }
    layer "particles" { colors "#ffd27a" "#ff5a1f"; count 64; rate 40; lifetime 1.4; direction "up"; blend "add"; }
}

border-style "alert" {
    layer "pulse" { colors "#ff3b30"; }
}

layout {
    focus-ring {
        active-style "embers"
        urgent-style "alert"
    }
}

window-rule {
    match app-id="firefox"
    border { active-style "none"; }
}
```

`active-style`, `inactive-style` and `urgent-style` choose the style for each state. They follow the same levels as every other border setting: `layout`, `monitor-profile`, `output`, `workspace` and window rules. A state without a style keeps its color or gradient. `"none"` clears a style set at an earlier level. An unknown name logs a warning and keeps the color. The focus ring draws nothing on inactive windows until you give it an `inactive-color`, `inactive-gradient` or `inactive-style`.

Layer types:

- `gradient`: a linear gradient across the frame. `angle` in degrees. It rotates once every 8 seconds at `speed 1`; `speed 0` holds it still, which with one color is a plain solid border.
- `sweep`: a rotating conic gradient, one turn every 4 seconds.
- `comet`: a bright head with a fading tail running around the frame, one lap every 4 seconds. `count` is the number of comets, `tail` the tail length as a share of the frame from 0 to 1. The first color is the head.
- `glow`: a breathing glow outside the frame, one breath every 3 seconds. Give it a large `width`, such as 18.
- `ripple`: a wave that runs around the frame from the top, 0.8 seconds at `speed 1`. Played by an event effect it runs once; in a style on its own it repeats with a rest.
- `pulse-front`: a front of light spreading from the window or from the event's point, lighting each border as it crosses.
- `wave`: a band travelling from one window to another across the borders between them. Played on focus it runs from the old window to the new one.
- `draw-in`: the border traces itself around from the top-left corner.
- `fade-out`: the border fades away.
- `dashes`: marching ants. `dash` and `gap` are lengths in logical pixels. They are adjusted slightly so the pattern closes without a seam.
- `pulse`: a pulsing band with a soft halo, 1.5 seconds per pulse. Good for `urgent-style`.
- `particles`: sparks from the frame. `count` is the pool size from 1 to 256, `rate` the new sparks per second, `size` the radius, `lifetime` their life in seconds and `direction` one of `"out"`, `"up"`, `"down"`, `"along"` or `"random"`. `speed 1` is about 40 logical pixels per second. Colors run from birth to death.

Reactive layer types read a signal or the window's sound:

- `equalizer`: bars rising outward from the frame by frequency band, lows at the bottom. `count` is the number of bars, `gap` the space between them in logical pixels.
- `waveform`: the sound wave as a line around the frame. `thickness` is the line width.
- `spectrum`: the frame colored and brightened by band, lows at the bottom round to highs at the top.
- `shimmer`: a fine sparkle along the edge, by default following `audio.treble`.
- `bass-thump`: width and brightness jump, by default with `audio.bass`.
- `speed-lines`: streaks along the edges. `count` and `tail` set how many and how long. By default they follow `scroll.speed` and `scroll.direction`.
- `scroll-trail`: a light on the frame placed by `position`, by default following `scroll.distance` and `scroll.direction`.
- `bulge`: the border swells out in round bumps that travel along the edges. `width` is the largest swell, `thickness` the border between bumps, `count` the bumps per side and `tail` how long each is. By default the size follows `scroll.speed` and they move with `scroll.direction`.
- `wavy`: the border line itself undulates along the edges. `width` is the largest swing, `thickness` the line, `count` the crests per side. By default the swing follows `scroll.speed` and the crests move with `scroll.direction`.
- `elastic`: the edge the scroll points at stretches outward by up to `width` and springs back with a small overshoot. By default it follows `scroll.speed` and `scroll.direction`.
- `chevrons`: arrows run along the left and right edges. `count` is the arrows per side, `thickness` their line. By default they follow `scroll.speed` and `scroll.direction`.
- `edge-flash`: the top or bottom edge lights up, by direction. `tail` is how far the light runs down the sides. By default it follows `scroll.speed` and `scroll.direction`.
- `gauge`: a marker slides along the right edge, placed by `position`, by default following `scroll.distance`. `tail` is its length, `width` its thickness.

The first three also take `sensitivity` (a multiplier, 1 by default) and `smoothing` (seconds, 0.3 by default). These types and the next take `baseline`, how much shows with no signal, from 0 to 1. `scroll-trail` and `gauge` also take `position`.

A layer driven by the sound, the wheel, the pointer or Konveyor's own scrolling shows nothing until that source moves, appears at once, and fades out when it stops: after a second for input, after two for sound. Nothing repaints while it is faded out.

Any layer's number can follow a signal:

```kdl
border-style "club" {
    layer "glow" {
        colors "accent"
        width 6
        drive "width" "audio.bass" amount=18 smooth=0.05 curve="square"
        drive "opacity" "audio.level" amount=0.5
    }
    layer "particles" {
        colors "#ffffff" "accent"
        count 96
        burst 24
        fire-on "audio.beat" "click"
    }
}
```

- `drive "<setting>" "<signal>"`: the value each frame is the setting plus `amount` times the signal. `amount` defaults to 1, `smooth` is a time in seconds and defaults to 0, `curve` is `"linear"`, `"square"`, `"sqrt"` or `"invert"`. The settings are `speed`, `width`, `opacity`, `angle`, `count`, `tail`, `dash`, `gap`, `rate`, `size`, `lifetime`, `burst`, `thickness`, `baseline` and `position`, plus `direction`, which flips which way the layer runs, and `color-mix`, which blends the whole layer from its first color to its last by the value, on any layer type. A `color-mix` drive never hides its layer: with no signal the layer stays on its first color, and it does not repaint then. For example, `drive "color-mix" "pointer.speed" amount=1.0 smooth=0.1` on a `gradient` with two colors. A `drive` for a setting replaces the type's default drive for that setting, and `amount=0` turns one off.
- `fire-on`: impulse signals that start a `particles` burst or a one-shot layer such as `ripple`. `burst` is the number of sparks per firing. `count` still caps how many are alive at once.
- Signals that hold a value: `audio.level`, `audio.bass`, `audio.mid`, `audio.treble`, `scroll.speed`, `scroll.distance`, `scroll.direction`, `pointer.speed`, `typing.rate`, `view.speed`, `view.distance`, `view.direction`, `time`.
- Impulses, which jump to 1 and fade: `audio.beat`, `click`, `scroll.step`, `keypress`, `view.step`, `focus`, `unfocus`, `urgent`.

Audio signals come from the sound of that window alone. Konveyor finds the PipeWire streams that belong to the window by process and desktop file name and measures them. Nothing is recorded or stored, and a window with no sound reads as silent. This needs PipeWire. Nothing is captured unless a visible window's style uses an audio signal. Keypress signals only say that a key was pressed and how often, never which key.

Every layer also takes:

- `colors`: up to 8 colors as `"#rrggbb"`, CSS colors or `"accent"`. Without it, the layer uses the KDE accent color.
- `speed`: how fast it runs; 1 by default, negative reverses it, 0 holds it still.
- `width`: thickness in logical pixels; by default the `width` of the `focus-ring` or `border` using the style.
- `opacity`: from 0 to 1.
- `blend`: `"normal"`, `"add"`, `"screen"` or `"multiply"`. `"add"` adds light without clipping, so a layer stacked on others or on something bright keeps its color instead of going white.

`animations { reduce-motion; }` or `animations { off; }` draws every style still and skips ripples, pulse fronts, waves, draw-ins and fade-outs. Animated styles also hold a still frame while the machine runs on battery. Fullscreen windows have no border, so nothing runs for them, and a border that is completely hidden stops redrawing until it shows again. Without a style in use, none of this code runs. Animated borders need OpenGL compositing; without it, a window keeps its plain color. They are not drawn inside the Overview's window thumbnails.

## Event effects

The simple way to react to an event is a layer in the style itself: `fire-on "focus"` on a `ripple` layer plays it once each time the window gains focus. This is how the default style does it.

```kdl
border-style "Mine" {
    layer "ripple" { colors "accent"; opacity 0.8; blend "add"; speed 1; fire-on "focus"; }
}
```

`event-effects` is for reaching across windows and for windows with no border. An event can play any border style on window borders, even on windows with no border or focus ring of their own, and on the windows around it. The style then goes away. Any layer kind can be played this way. `event-effects` follows the same levels as the focus ring: `layout`, `monitor-profile`, `output` and `workspace`.

```kdl
border-style "Focus-Ripple" {
    layer "ripple" { colors "accent"; opacity 0.8; blend "add"; speed 1; }
}

border-style "Light-Trail" {
    layer "wave" { colors "accent"; opacity 0.9; blend "add"; }
}

border-style "Pulse-Out" {
    layer "pulse-front" { colors "#ffffff" "accent"; blend "add"; }
}

layout {
    event-effects {
        on "focus" "Focus-Ripple" reach="neighbors"
        on "unfocus" "none"
        on "click" "Pulse-Out" reach="neighbors"
        on "view" "Light-Trail" reach="all" mode="while"
    }
}
```

- Events: `focus`, `unfocus`, `click`, `scroll`, `keypress`, `urgent`, `beat` and `view`, which is Konveyor's own scrolling of the columns.
- The second value is a border style name, or `"none"` to play nothing.
- `reach`: `"self"` by default, `"neighbors"` for windows touching it or next to it in the column layout, or `"all"` for every visible window on that monitor. A `wave` also lights the windows it crosses.
- `mode`: `"once"` plays the style once. `"while"` keeps it going while the event keeps happening, then fades it out. `scroll` and `view` default to `"while"`, the rest to `"once"`.
- `duration`: seconds. By default the longest one-shot layer in the style, or one second if there is none. Other layers fade in and out around it.

Nothing is redrawn between plays. Event effects do not play with `reduce-motion`, with animations off, on battery, or on a monitor showing a fullscreen window. Coming back to the same window does not play `focus` again. A new focus change replaces the focus and unfocus effects still running; other events stack.

## Fixed workspaces

Settings → Workspaces → **Use a fixed number of workspaces** keeps a chosen number of numbered workspaces on every monitor, including empty ones. Choose **Workspaces per monitor** from 1 to 20; Konveyor sets KDE's virtual desktop count to match. The default remains dynamic, with a new empty workspace at the bottom and empty unnamed workspaces removed automatically.

```kdl
layout {
    fixed-workspace-count 4
}
```

`horizontal-desktops` lays out KDE's virtual desktops side by side in one row instead of one column, so pager widgets on a thin top or bottom panel stay thin. Settings → Workspaces → **Lay out KDE desktops horizontally** turns it on. It only changes how KDE arranges its desktops; Konveyor's workspaces still move up and down. This option is global.

`wrap-workspaces` makes the workspace actions wrap around when you reach the end of a fixed set: with four workspaces, going down from the fourth lands on the first and going up from the first lands on the fourth. It covers focusing the workspace above or below, moving a window or column to the workspace above or below, and the shortcuts and wheel binds built on them. Settings → Workspaces → **Wrap around at the first and last workspace** turns it on. It is off by default, only applies with `fixed-workspace-count` above 0 (dynamic mode always ends in a spare empty workspace, so it never wraps), and is global. Touch and touchpad swipes do not wrap.

`fixed-workspace-count 0` restores dynamic workspaces. This option is global. Fixed mode ignores `empty-workspace-above-first`, allows the last workspace to hold windows, and keeps workspace numbers when windows close. Switching modes and changing the count apply immediately.

When reducing the count or collecting workspaces from a disconnected monitor, extra unnamed workspaces combine into a remaining workspace, preserving their windows, tiled columns, and floating windows. Named workspaces are always kept: the number of names on a monitor is its minimum count, even when that exceeds the requested count. Reconnecting a monitor restores named workspaces to their configured home; combined unnamed workspaces stay together.

## Mouse, wheel and touchpad binds

Besides keys, binds take `MouseLeft`, `MouseRight`, `MouseMiddle`, `MouseBack`, `MouseForward`, `WheelScrollUp`/`Down`/`Left`/`Right` and `TouchpadScrollUp`/`Down`/`Left`/`Right`, with or without modifiers. KWin's own wheel shortcuts come first: Meta+Ctrl with the vertical wheel zooms (the Zoom effect) and Meta+Alt with it switches desktops, so a bind on one of those only runs once that KWin shortcut is changed or turned off. `ISO_Level3_Shift` (AltGr) and `ISO_Level5_Shift` work as modifiers and as `mod-key`; KDE's shortcut system cannot hold them, so Konveyor catches those binds itself.

Mouse and wheel binds and the workspace keys (`focus-workspace`, `focus-workspace-up`/`down`/`previous`) act on the monitor under the pointer. Other key binds act on the focused window's monitor. `input { focus-keys-follow-mouse; }` sends the column and window focus keys (`focus-column-*` and `focus-window-*`, except `focus-window-previous`) to the monitor under the pointer too, so Meta+Left and Meta+Right walk that monitor's row. Settings → Mouse → **Focus shortcuts act on the screen under the pointer** turns it on.

## Placing new windows

`new-window-placement "column"` (the default) gives every new window its own column. `new-window-placement "stack"` adds it as a row to the focused column until the column holds `max-rows-per-column` windows, then opens a new column; rows share the height evenly. With `group-app-windows "beside"` a new window stacks into its app's last column instead of the focused one. `new-window-placement`, `group-app-windows` and `max-rows-per-column` work globally, inside `monitor-profile` and `output` layout blocks, and per app in window rules.

The default config ships a `portrait` monitor profile for screens taller than they are wide: columns fill the width and stack two windows, so Mod+Left/Right flips between full-width pages and Mod+Up/Down moves between the rows, then on to the workspace above or below.

`wrap-columns` joins the two ends of a workspace's row into a loop: past the last column the first one shows again, and before the first the last one does, so a wide centered column never leaves an empty strip at the edge. Focusing left or right, wheel binds and touchpad or drag scrolling keep going around instead of stopping at a wall, and moving from the last column to the first slides one step forward. Each window is still shown once: the screen shows each column on the side it is closest to. A row that already fits on the screen stays as it is. `focus-column-or-monitor-left`/`right` still move to the next monitor from the first or last column, and the column move actions keep their order. It is off by default, works globally and inside `monitor-profile`, `output` and `workspace` layout blocks, and Settings → Layout → **Scroll around past the last column** turns it on.

A column stretches to the full width while it is the only one on its workspace, like `expand-column-to-available-width` without the keypress, and goes back to its own width as soon as another column opens. Apps that can't be resized keep their own size. `always-expand-single-column false` in `layout` turns this off. It combines with `always-center-single-column` and works globally and inside `monitor-profile` and `output` layout blocks.

Tiled windows respect application size limits by default. Add `force-resizable true` to a window rule when a specific app should ignore those limits and fill its grid cell. <kbd>Meta</kbd> + <kbd>Alt</kbd> + <kbd>R</kbd> toggles this for the focused app and stores the override in `~/.config/konveyor/force-resizable.kdl`. When force resizing is off, Konveyor never resizes a window that the application marks non-resizable. The app remains free to change its own resolution, and Konveyor updates the grid around its new size.

```kdl
window-rule {
    match app-id=r#"^example-game$"#
    force-resizable true
}
```

Experimental fullscreen focus handling is global and disabled by default. `prevent-fullscreen-minimize` intercepts native Wayland and XWayland applications that try to minimize themselves while fullscreen; XWayland applications may stop drawing until they are focused again. `prevent-fullscreen-exit` prevents an unfocused native Wayland or XWayland application from leaving fullscreen. Both also recognize undecorated windows that cover an entire output.

```kdl
experiments {
    prevent-fullscreen-minimize
    prevent-fullscreen-exit
}
```

```kdl
monitor-profile "portrait" {
    match aspect-ratio-below=1.0
    layout {
        default-column-width { proportion 1.0; }
        new-window-placement "stack"
        max-rows-per-column 2
    }
}
```

## Touch and gestures

Konveyor drives the scrolling row from touchpads and touchscreens:

| Gesture | Default | Option |
| --- | --- | --- |
| 3-finger horizontal swipe | Scroll the row | `horizontal-swipe "scroll-view"` or `"off"` |
| 3-finger vertical swipe | Switch workspace | `vertical-swipe "switch-workspace"` or `"off"` |
| 4-finger swipe left or right | Merge the focused window into the neighbouring column, or pop it back out | `window-horizontal-swipe "consume-or-expel"` or `"off"` |
| 4-finger swipe up or down | Move the window up or down through its column like Mod+Ctrl+Up/Down, then to the workspace above or below once it reaches the end (the focused window on a touchpad, the window under your fingers on a touchscreen) | `window-vertical-swipe "move-window"` or `"off"` (`"move-to-workspace"` is still accepted) |
| 4-finger pinch | Open or close KDE's Overview | `pinch "toggle-overview"` or `"off"` |
| 3-finger tap | Cycle the column's width through `preset-column-widths`, like `switch-preset-column-width` | `three-finger-tap "cycle-width"` |
| 4-finger tap | Open the Kontrol Panel, or close it if it is open | `four-finger-tap "kontrol-panel"` |
| 5-finger tap | Nothing | `five-finger-tap "off"` |
| Touchscreen long press on a title bar, then drag | Move the window | `long-press-to-move`, `long-press-ms 500` |

```kdl
gestures {
    touchpad {
        swipe-fingers 3
        pinch-fingers 4
        window-swipe-fingers 4
        natural-swipe
    }
    touchscreen {
        swipe-fingers 3
        long-press-ms 500
    }
}
```

`touchpad` and `touchscreen` take the same options; `long-press-to-move` and `long-press-ms` are touchscreen only. Finger counts range from 2 to 5. `natural-swipe false` makes the row move against your fingers. `off` inside either block hands every gesture back to KDE.

The window swipes move one step for every stretch of finger travel, so a long swipe carries the window several columns or workspaces; the window goes the way your fingers go. On a touchscreen the gesture moves the window under your fingers; on a touchpad it moves the focused window. If `swipe-fingers` and `window-swipe-fingers` are the same, the row swipes win.

Each tap option takes `"cycle-width"`, `"kontrol-panel"`, `"toggle-overview"` or `"off"`. A tap is all fingers down and lifted within 250 ms without sliding; anything that turns into a swipe or pinch is not a tap. On a touchscreen a width tap acts on the column under your fingers; on a touchpad it acts on the focused column. The Kontrol Panel tap runs `portal-launcher toggle`, so it needs the Konveyor widgets installed, but not the Kontrol Panel button in a panel.

KDE and libinput have no multi-finger tap gesture, so Konveyor reads the touchpad's finger contacts from its `/dev/input/event*` node. Your user needs read access to it (membership in the `input` group); without it Konveyor logs `touchpad taps are unavailable` to the KWin journal and touchpad taps do nothing. libinput's tap-to-click turns a 3-finger tap into a middle click (a right click with the left-middle-right button map). While `three-finger-tap` is not `"off"`, Konveyor takes that click after a 3-finger touch, so apps no longer get a middle click from it; set `three-finger-tap "off"` to get the middle click back. Konveyor never changes libinput's own touchpad settings.

The finger counts Konveyor uses take over the KDE gestures with the same count: by default the 3- and 4-finger swipes that switch virtual desktops and KDE's 4-finger swipe up for Overview. The pinch opens KDE's own Overview, as do Konveyor's hot corners and the `toggle-overview` action. Set a window or row swipe to `"off"` to give that gesture back to KDE. One-finger touch is left to apps: tapping focuses, and dragging a title bar scrolls the row or moves the window like the mouse does.

If a config file has a mistake, Konveyor keeps running with the last good one and tells you what is wrong.

## Scripting

`konveyor msg` queries and controls the running instance:

```sh
konveyor msg windows
konveyor msg action set-column-width +10%
```
