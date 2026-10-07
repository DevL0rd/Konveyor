# Konveyor Taskbar

An icons-only task manager for KDE Plasma 6 panels, built for Konveyor. Its icons are your columns: they sit in the same order as the columns on the current workspace of the screen the panel is on, and moving either one moves the other.

## Features

- **Two-way column order** — drag an icon and its column moves in Konveyor; move a column in Konveyor and the icon follows.
- **Pinned apps** — pins keep their spot when nothing is open. When a pinned app opens, its column is placed by the pin order, so pins A, B and C give columns A, B and C whichever one you start first.
- **Workspaces in the same widget** — a slim strip shows the workspaces of the panel's screen with a dot for each column, the focused column stretched and the current workspace in your accent colour. Named workspaces show their name. Click one to switch, or scroll over the strip to step through them.
- **Grouping like Konveyor** — by default an app's neighbouring columns share one icon when Konveyor's `group-app-windows` is `beside` or `stack`, and stacked or tabbed windows in one column always do. You can always group or never group instead.
- **Shortcut numbers** — hold <kbd>Meta</kbd> and the workspaces show the keys that switch to them (<kbd>Meta</kbd>+<kbd>1</kbd>…<kbd>9</kbd> by default). Icons show a number when you bind `focus-column` to a key.
- **Clicks** — click an icon to focus its window and scroll to its column, click the focused one to minimize it, middle-click to close it and right-click for its windows, a new window, pinning and closing. A group cycles through its windows.
- **State at a glance** — a dot for each window, an accent bar under the focused app, hollow dots for minimized windows and a pulse when an app wants attention. Minimized windows keep the place their column had.

Icons, the strip and the badges follow the panel's thickness and edge, on horizontal and vertical panels, and use your Plasma colours.

## How it works

The widget lists windows with Plasma's task manager model and asks Konveyor over D-Bus (`org.kde.Konveyor`) where they are:

- `Workspaces` lists each workspace's columns in order with their window ids, and its `group-app-windows` mode.
- `Windows` gives each window's KWin uuid, which matches the task manager's window ids.
- `LayoutChanged` fires whenever the columns, workspaces or focus change, so the widget never polls.
- `SuperHeld` and `SuperHeldChanged` tell it when <kbd>Meta</kbd> is held.
- Dragging an icon calls the `move-column-to-index` action with the window's id, which moves that column without taking focus.

Without Konveyor the widget behaves like a plain icon task manager for the current desktop and screen.

## Install

`widgets/install.sh` installs it with the other Konveyor widgets. On the first install it is put on the panel right after the Kontrol Panel button, and Plasma's Icons-only Task Manager or Task Manager on that panel is removed. Its pinned apps carry over. Uninstalling puts Plasma's task manager back where the taskbar was, with the taskbar's pins.

## Settings

| Setting | Default | |
|---|---|---|
| Group windows | Like Konveyor | Like Konveyor's `group-app-windows`, always group an app's neighbouring columns, or one icon per column |
| Workspaces of this screen | On | The workspace strip |
| Shortcut numbers while Meta is held | On | Badges on workspaces and icons |
| Open new windows in pin order | On | Moves a pinned app's first column next to its pinned neighbours |

## License

MIT
