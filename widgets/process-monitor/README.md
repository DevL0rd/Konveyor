# Linux-Process-Mon

A native KDE Plasma 6 widget: a **searchable, sortable process tree** with
per-process **CPU**, **RAM** and **GPU** usage, styled to match the rest of the
suite (Router-Monitor, Log-Monitor, App Portal).

Processes nest under their parent and collapse; click a column header to sort,
type to filter.

## How it works (and why it's light)

A single resident helper (`bin/procmon-collect`) does all the work, exactly like
the router/log collectors:

* **One systemd `--user` service** (`linux-process-mon.service`) samples `/proc`
  and the GPU once per interval and writes a JSON snapshot to
  `$XDG_RUNTIME_DIR/Linux-Process-Mon/data.json`.
* **Pinned to the E-cores** (`CPUAffinity` from `bin/procmon-ecores`) and
  `Nice=19`, so it stays out of the way of foreground work.
* **The widget reads the snapshot in-process** via `file://` XHR (needs
  `QML_XHR_ALLOW_FILE_READ=1`, set by `install.sh` via `environment.d`) -- no
  process is spawned per refresh.

GPU usage is **per-process NVIDIA SM utilisation** from `nvidia-smi pmon`
(shows `0` on machines without an NVIDIA GPU, or for processes not using it).

## Install

Clone **with submodules** — the shared QML components live in the
[Plasma-Shared](https://github.com/DevL0rd/Plasma-Shared) submodule:

```sh
git clone --recurse-submodules https://github.com/DevL0rd/Linux-Process-Mon.git
cd Linux-Process-Mon
# already cloned without it?  git submodule update --init --recursive
./install.sh
```

Then add **Process Monitor** or **Process Monitor (Panel)** from *Add Widgets*.
Uninstall with `./uninstall.sh`.

## Panel widget

**Process Monitor (Panel)** shows the app you're using right in the panel: its
icon and name, how much CPU and GPU it and its child processes use, and its frame
rate while it's drawing (green at 60 and up, amber from 30, red below). With no
app focused it shows the busiest process instead. Middle-click jumps straight to
that app in the popup.

Click it for the full view:

- **Focused app card** with live graphs for CPU, GPU, frame time, RAM, VRAM, disk
  and threads, plus buttons to end, force kill, pause, resume, open its folder or
  copy its command
- **Process table** as a tree or a flat list, sortable by any column, with a
  small history graph behind every row
- **Filters** for apps, processes using the GPU and processes using the disk
- **Search** by name or PID
- **Click a process** to see what it and its children use, its frame rate and
  command line, and every action; right-click for the full menu
- **Columns** menu to show GPU, video encode and decode, VRAM, disk, threads and
  PID

### Frame rates

Frame rates come from [MangoHud](https://github.com/flightlessmango/MangoHud).
When MangoHud is installed, `install.sh` adds a few lines to
`~/.config/MangoHud/MangoHud.conf` so it logs frame times to a folder in memory
that the collector reads. Your MangoHud overlay looks and behaves the same, even
when it's hidden. Any game or app you run with MangoHud (`mangohud %command%` in
Steam, or `MANGOHUD=1`) then shows its frame rate. `uninstall.sh` puts your
previous MangoHud settings back.

## Settings

Widget (right-click → Configure): panel icon, refresh interval, show kernel
threads, colorize CPU/GPU usage, GPU column.

Collector sampling rate: `poll_interval` (seconds) in
`~/.config/Linux-Process-Mon/config.json`.
