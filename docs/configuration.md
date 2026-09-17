# Configuration

Open **System Settings → Window Management → Konveyor** (or the Configure button next to Konveyor in Desktop Effects). Every option has a page there with a live preview: layout, look, motion, mouse, touch and gestures, shortcuts, window rules, monitors, workspaces and Plasma integration. Press Apply and the change is live. `systemsettings kcm_konveyor shortcuts` opens a page directly.

The settings page edits `~/.config/konveyor/config.kdl` in place and keeps your comments and formatting, so you can use both.

Everything lives in `~/.config/konveyor/config.kdl`, a [KDL](https://kdl.dev) file. The shipped default config lists every option with a short explanation. Save the file and the change is live — no restart, no logout.

```kdl
layout {
    gaps 16
    center-focused-column "never"
    default-column-width { proportion 0.5; }

    focus-ring {
        width 4
        active-color "accent"
    }
}

binds {
    Mod+Left  { focus-column-left; }
    Mod+Right { focus-column-right; }
    Mod+R     { switch-preset-column-width; }
}
```

You get gaps, borders, a focus ring, tab indicators, preset widths, centering modes, per-monitor and per-workspace overrides, window rules, animation springs and key binds. Two KDE-specific extras: `active-color "accent"` follows the KDE accent color, and `gestures { titlebar-drag "scroll-view" }` scrolls the row when you drag a window by its title bar.

## Placing new windows

`new-window-placement "column"` (the default) gives every new window its own column. `new-window-placement "stack"` adds it as a row to the focused column until the column holds `max-rows-per-column` windows, then opens a new column; rows share the height evenly. With `group-app-windows "beside"` a new window stacks into its app's last column instead of the focused one. `new-window-placement`, `group-app-windows` and `max-rows-per-column` work globally, inside `monitor-profile` and `output` layout blocks, and per app in window rules.

The default config ships a `portrait` monitor profile for screens taller than they are wide: columns fill the width and stack two windows, so Mod+Left/Right flips between full-width pages and Mod+Up/Down moves between the rows, then on to the workspace above or below.

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
| 4-finger swipe up or down | Carry the focused window to the workspace above or below | `window-vertical-swipe "move-to-workspace"` or `"off"` |
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

Each tap option takes `"cycle-width"`, `"kontrol-panel"`, `"toggle-overview"` or `"off"`. A tap is all fingers down and lifted within 250 ms without sliding; anything that turns into a swipe or pinch is not a tap. On a touchscreen a width tap acts on the column under your fingers; on a touchpad it acts on the focused column. The Kontrol Panel tap runs `portal-launcher toggle`, so it needs the Konveyor widgets installed.

KDE and libinput have no multi-finger tap gesture, so Konveyor reads the touchpad's finger contacts from its `/dev/input/event*` node. Your user needs read access to it (membership in the `input` group); without it Konveyor logs `touchpad taps are unavailable` to the KWin journal and touchpad taps do nothing. libinput's tap-to-click turns a 3-finger tap into a middle click (a right click with the left-middle-right button map). While `three-finger-tap` is not `"off"`, Konveyor takes that click after a 3-finger touch, so apps no longer get a middle click from it; set `three-finger-tap "off"` to get the middle click back. Konveyor never changes libinput's own touchpad settings.

The finger counts Konveyor uses take over the KDE gestures with the same count: by default the 3- and 4-finger swipes that switch virtual desktops and KDE's 4-finger swipe up for Overview. The pinch opens KDE's own Overview, as do Konveyor's hot corners and the `toggle-overview` action. Set a window or row swipe to `"off"` to give that gesture back to KDE. One-finger touch is left to apps: tapping focuses, and dragging a title bar scrolls the row or moves the window like the mouse does.

If a config file has a mistake, Konveyor keeps running with the last good one and tells you what is wrong.

## Scripting

`konveyor msg` queries and controls the running instance:

```sh
konveyor msg windows
konveyor msg action set-column-width +10%
```
