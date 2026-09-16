# Linux-System-Monitor

A native KDE Plasma 6 widget: a clean local-system dashboard — total + per-core
**CPU** (split into **P-cores / E-cores** on Intel hybrid, each with a group
total), frequency, package **power** (Intel RAPL) and **temperature**, **RAM** +
swap, and the **NVIDIA GPU** (usage, VRAM, temp, power, clocks, fan). Styled to
match the rest of the suite (Router-Monitor, Log-Monitor, App Portal,
Process-Mon).

The cards stack in a single vertical column.

## How it works

A resident helper (`bin/sysmon-collect`) does all the work, like the other
collectors: a systemd `--user` service samples `/proc`, `/sys` and `nvidia-smi`
once per interval and writes a JSON snapshot to
`$XDG_RUNTIME_DIR/Linux-System-Monitor/data.json`. It's **pinned to the E-cores**
(`bin/sysmon-ecores`) with `Nice=19`, and the widget reads the snapshot
in-process via `file://` XHR (needs `QML_XHR_ALLOW_FILE_READ=1`, set by
`install.sh` via `environment.d`).

## Install

Clone **with submodules** — the shared QML/JS components live in the
[Linux-Plasma-Shared](https://github.com/DevL0rd/Linux-Plasma-Shared) submodule:

```sh
git clone --recurse-submodules https://github.com/DevL0rd/Linux-System-Monitor.git
cd Linux-System-Monitor
# already cloned without it?  git submodule update --init --recursive
./install.sh
```

Add **System Monitor** or **System Monitor (Panel)** from *Add Widgets*.
Uninstall with `./uninstall.sh`.

## Panel widget

**System Monitor (Panel)** is made for a panel. The button shows CPU, GPU and
memory use with a thin bar under each, plus CPU and GPU temperatures, and each
value turns amber or red when it runs hot. It stacks neatly in vertical panels
too. Middle-click opens KDE System Monitor.

Click it for the full dashboard:

- **Rings** for CPU, GPU and memory — click one to jump to its card
- **CPU**: a history graph with Usage, Temp, Clock, Power and Fan tabs, plus
  now / peak / average for the visible history
- **Cores**: performance and efficiency cores, each with a group bar and a bar
  per core
- **GPU**: the same graph tabs, VRAM, memory clock, power limit and max clock
- **Memory**: RAM and swap with a history graph
- **Search** filters the dashboard — try `temp`, `power`, `e-core`, `vram` or
  `swap`
- Cards fold away with the arrow in their corner and stay folded

Everything in the popup is only built while it's open, so a closed popup costs
nothing beyond the panel button.

Settings (right-click → Configure): panel icon, refresh interval, accent colour,
history graphs, per-core bars, GPU section. Collector sampling rate is
`poll_interval` in `~/.config/Linux-System-Monitor/config.json`.
