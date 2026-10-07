# Konveyor Taskbar

An icons-only task manager for KDE Plasma 6 panels, built for Konveyor. Its icons are your columns: they sit in the same order as the columns on the current workspace of the screen the panel is on, and moving either one moves the other.

## Features

- **Every window one click away** — each window has its own icon. Windows that share a column sit together in a rounded capsule, top to bottom, with a small tab mark when the column shows them as tabs, so the taskbar mirrors the layout without hiding anything.
- **Two-way column order** — drag an icon or a capsule and its column moves in Konveyor; move a column in Konveyor and the icon follows.
- **Pinned apps** — pins keep their spot when nothing is open. When a pinned app opens, its column is placed by the pin order, so pins A, B and C give columns A, B and C whichever one you start first.
- **Workspaces in the same widget** — a slim strip shows the workspaces of the panel's screen with a dot for each column, the focused column stretched and the current workspace in your accent colour. Named workspaces show their name. Click one to switch, or scroll over the strip to step through them.
- **Grouping like Konveyor** — by default an app's neighbouring single-window columns share one icon when Konveyor's `group-app-windows` is `beside` or `stack`. Hover a group for a list of its windows and click one to go straight to it; clicking the group cycles through them. You can always group or never group instead. Shared columns are never grouped.
- **Shortcut numbers** — hold <kbd>Meta</kbd> and the workspaces show the keys that switch to them (<kbd>Meta</kbd>+<kbd>1</kbd>…<kbd>9</kbd> by default). Hold <kbd>Meta</kbd>+<kbd>Alt</kbd> and the apps show the keys bound to `focus-column` instead (<kbd>Meta</kbd>+<kbd>Alt</kbd>+<kbd>1</kbd>…<kbd>9</kbd> by default), one per column. The numbers come from your binds, so custom keys show too.
- **Konveyor in the right-click menu** — widen, narrow, cycle the preset widths, go full width, maximize or center a column; move it left, right, to either end, to another workspace or monitor, or bring it to this screen; join the neighbouring column or move out to its own; show the column as tabs, float, go fullscreen or show on every workspace. All of them act on that icon's window without focusing it first.
- **Remember for an app** — the same menu writes Konveyor window rules for the app: always open as this column, on this workspace, on this monitor, at this size on this monitor, floating, on every workspace, stacked with its other windows, or as a row in the current column. Each shows whether it is on, unchecking removes the rule, and **Edit Rules** opens it in Konveyor's settings. The rules go through Konveyor's config writer, so your comments and formatting in `config.kdl` stay, and Konveyor applies them right away.
- **Open new windows where you want them** — next to the window you right-clicked, or as a row below it.
- **State at a glance** — a dot or a bar for each window, an accent highlight on the focused app, hollow dots for minimized windows and a pulse when an app wants attention. Minimized windows keep the place their column had.

Icons, the strip and the badges follow the panel's thickness and edge, on horizontal and vertical panels, and use your Plasma colours.

## How it works

The widget lists windows with Plasma's task manager model and asks Konveyor over D-Bus (`org.kde.Konveyor`) where they are:

- `Workspaces` lists each workspace's columns in order with their window ids, which of them show as tabs, and its `group-app-windows` mode.
- `Windows` gives each window's KWin uuid, which matches the task manager's window ids.
- `LayoutChanged` fires whenever the columns, workspaces or focus change, so the widget never polls.
- `ModifiersHeld` and `ModifiersHeldChanged` tell it when <kbd>Meta</kbd>, and <kbd>Meta</kbd>+<kbd>Alt</kbd>, are held.
- `Action` with a window id runs the menu's layout actions and `move-column-to-index` for dragging, without taking focus.
- `AppRules` and `SetAppRule` read and write the app's remembered window rules.

Without Konveyor the widget behaves like a plain icon task manager for the current desktop and screen.

## Install

`widgets/install.sh` installs it with the other Konveyor widgets. On the first install it is put on the panel right after the Kontrol Panel button, and Plasma's Icons-only Task Manager or Task Manager on that panel is removed. Its pinned apps carry over. Uninstalling puts Plasma's task manager back where the taskbar was, with the taskbar's pins.

## Settings

| Page | Setting | Default |
|---|---|---|
| Appearance | Icon size | Follows the panel thickness, or a fixed size |
| | Space between icons | 2 px |
| | Padding around each icon | 3 px |
| | Focused app | Filled, outline or no highlight |
| | Window indicators | A dot for each window, a bar or none |
| | Indicator position | Next to the screen edge or on the opposite side |
| | Separator between the workspaces and the apps | On |
| | Animations | On |
| | Pulse on apps that want attention | On |
| Behavior | Group windows | Like Konveyor's `group-app-windows`, always, or one icon per column |
| | Clicking the focused app | Minimizes it, does nothing or switches to its next window |
| | Middle-click | Closes the window, opens a new one or does nothing |
| | Scroll over the apps to switch between them | Off |
| | Show windows from | This screen only, or every screen |
| | Floating windows | Shown |
| | Tooltips | On |
| | Shortcut numbers while Meta or Meta+Alt is held | On |
| Workspaces | Show the workspaces of this screen | On |
| | Position | Before the apps, or after them |
| | Each workspace shows | A dot for each column, its name or number, or its number |
| | Empty workspaces | Shown |
| | Scroll over the workspaces to switch between them | On |
| Pinned Apps | Open a pinned app's first window in pin order | On |
| | The pins | Reorder, unpin, or add from an app search or by desktop file |

## License

MIT
