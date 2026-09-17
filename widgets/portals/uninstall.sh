#!/bin/bash
set -e
BIN_DIR="$HOME/.local/bin"
echo "Removing portal-games..."
rm -f "$BIN_DIR/portal-games"
echo "Stopping friends-presence service..."
systemctl --user disable --now portal-friends.service 2>/dev/null || true
rm -f "$HOME/.config/systemd/user/portal-friends.service"
systemctl --user daemon-reload 2>/dev/null || true
rm -f "$BIN_DIR/portal-friends"
rm -f "$BIN_DIR/portal-ecores"
echo "  (kept ~/.config/Plasma-App-Portal/config.json with your API key)"

echo "Removing runtime snapshot + environment.d entry..."
rm -rf "${XDG_RUNTIME_DIR:-/tmp}/Plasma-App-Portal"
rm -f "$HOME/.config/environment.d/linux-plasma-portals.conf"

echo "Removing widget(s)..."
for id in org.devl0rd.portal org.devl0rd.portal.friends; do
    kpackagetool6 -t Plasma/Applet -r "$id" >/dev/null 2>&1 && echo "  removed $id" || true
done
echo "Done. (Custom art in ~/.local/share/Plasma-App-Portal was left in place.)"
