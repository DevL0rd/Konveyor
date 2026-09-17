# Configuration

Open **System Settings → Window Management → Konveyor** (or the Configure button next to Konveyor in Desktop Effects). Every option has a page there with a live preview: layout, look, motion, mouse and gestures, shortcuts, window rules, monitors, workspaces and Plasma integration. Press Apply and the change is live. `systemsettings kcm_konveyor shortcuts` opens a page directly.

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

## Touch and gestures

Konveyor drives the scrolling row from touchpads and touchscreens:

| Gesture | Default | Option |
| --- | --- | --- |
| 3-finger horizontal swipe | Scroll the row | `horizontal-swipe "scroll-view"` or `"off"` |
| 3-finger vertical swipe | Switch workspace | `vertical-swipe "switch-workspace"` or `"off"` |
| 4-finger pinch | Open or close the overview | `pinch "toggle-overview"` or `"off"` |
| Touchscreen long press on a title bar, then drag | Move the window | `long-press-to-move`, `long-press-ms 500` |

```kdl
gestures {
    touchpad {
        swipe-fingers 3
        pinch-fingers 4
        natural-swipe
    }
    touchscreen {
        swipe-fingers 3
        long-press-ms 500
    }
}
```

`touchpad` and `touchscreen` take the same options; `long-press-to-move` and `long-press-ms` are touchscreen only. Finger counts range from 2 to 5. `natural-swipe false` makes the row move against your fingers. `off` inside either block hands every gesture back to KDE.

The finger counts Konveyor uses take over the KDE gestures with the same count (by default the 3-finger swipes that switch virtual desktops). Gestures with other finger counts, such as KDE's 4-finger swipe up for Overview, keep working. One-finger touch is left to apps: tapping focuses, and dragging a title bar scrolls the row or moves the window like the mouse does.

If a config file has a mistake, Konveyor keeps running with the last good one and tells you what is wrong.

## Scripting

`konveyor msg` queries and controls the running instance:

```sh
konveyor msg windows
konveyor msg action set-column-width +10%
```
