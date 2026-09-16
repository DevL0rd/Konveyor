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

If a config file has a mistake, Konveyor keeps running with the last good one and tells you what is wrong.

## Scripting

`konveyor msg` queries and controls the running instance:

```sh
konveyor msg windows
konveyor msg action set-column-width +10%
```
