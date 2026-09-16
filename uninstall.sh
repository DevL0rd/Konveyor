#!/bin/bash
set -e
BIN_DIR="$HOME/.local/bin"

echo "Stopping resident collector..."
systemctl --user disable --now linux-process-mon.service 2>/dev/null || true
rm -f ~/.config/systemd/user/linux-process-mon.service
systemctl --user daemon-reload 2>/dev/null || true

echo "Restoring MangoHud settings..."
python3 "$(dirname "$(readlink -f "$0")")/bin/procmon-mangohud" --restore

echo "Removing collector + env flag..."
rm -f "$BIN_DIR/procmon-collect"
rm -f ~/.config/environment.d/linux-process-mon.conf
rm -rf "${XDG_RUNTIME_DIR:-/tmp}/Linux-Process-Mon"

echo "Removing widget..."
for id in org.devl0rd.procmon org.devl0rd.procmon.panel; do
    kpackagetool6 -t Plasma/Applet -r "$id" >/dev/null 2>&1 \
        && echo "  removed $id" || true
done

echo "Done. (Kept ~/.config/Linux-Process-Mon/config.json.)"
