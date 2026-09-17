#!/bin/bash
set -e

BIN_DIR="$HOME/.local/bin"

echo "Stopping resident collector..."
systemctl --user disable --now linux-log-monitor.service 2>/dev/null || true
rm -f ~/.config/systemd/user/linux-log-monitor.service
systemctl --user daemon-reload 2>/dev/null || true

echo "Removing helper scripts and env flag..."
rm -f "$BIN_DIR/logmon-collect"
rm -f ~/.config/environment.d/linux-log-monitor.conf ~/.config/plasma-workspace/env/linux-log-monitor.sh
rm -rf "${XDG_RUNTIME_DIR:-/tmp}/Linux-Log-Monitor"

echo "Removing widget(s)..."
for id in org.devl0rd.logmon.journal; do
    kpackagetool6 -t Plasma/Applet -r "$id" >/dev/null 2>&1 && echo "  removed $id" || true
done

echo "Done. Remove the widget from your panel/desktop if it's still placed."
