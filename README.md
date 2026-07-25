# Linux Widget Hider

Automatically hides KDE Plasma desktop widgets while a normal window is visible on the current virtual desktop and Activity.

Widgets disappear immediately when a window appears. When the desktop is exposed again, they fade in over 250 ms. Plasma's **Show Desktop** action is detected explicitly, so `Meta+D` reveals the widgets even though the underlying windows remain mapped.

Panel widgets, the taskbar, and the system tray are not affected.

## Requirements

- KDE Plasma 6
- KWin
- `patch`, `kwriteconfig6`, `qdbus6`, and `systemctl`

## Install

```bash
chmod +x install.sh uninstall.sh
./install.sh
```

The installer creates a user-local override of Plasma's desktop containment, installs the KWin script, enables it, and restarts Plasma Shell. It refuses to overwrite an unrelated local desktop-containment override.

## Uninstall

```bash
./uninstall.sh
```

The uninstaller removes only files marked as managed by Linux Widget Hider, disables the KWin script, and restores Plasma's system desktop containment.

## Behavior

- A normal, non-minimized window on the current virtual desktop and Activity hides the desktop widgets.
- Minimizing or closing the last applicable window reveals them.
- `Meta+D` reveals them.
- Switching virtual desktops or Activities recalculates visibility.
- Reveal is animated; hiding is immediate.

## License

GPL-3.0-or-later
