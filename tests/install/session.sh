#!/usr/bin/env bash
set -euo pipefail

systemctl --user set-environment XDG_SESSION_TYPE=wayland XDG_CURRENT_DESKTOP=KDE KDE_FULL_SESSION=true KDE_SESSION_VERSION=6 WAYLAND_DISPLAY=wayland-0
systemd-run --user --unit=headless-kwin --collect kwin_wayland --virtual --xwayland --no-lockscreen --socket wayland-0 --width 1920 --height 1080 >/dev/null
for _ in $(seq 60); do
    [[ -S $XDG_RUNTIME_DIR/wayland-0 ]] && break
    sleep 0.5
done
systemctl --user set-environment QT_QPA_PLATFORM=wayland
systemctl --user start plasma-plasmashell.service
sleep 20
systemctl --user restart plasma-plasmashell.service
sleep 15
systemctl --user is-active headless-kwin.service plasma-plasmashell.service
