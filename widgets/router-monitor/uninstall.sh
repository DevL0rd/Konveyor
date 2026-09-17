#!/bin/bash
set -e

APP="Linux-Router-Monitor"
CFG_DIR="$HOME/.config/$APP"
BIN_DIR="$HOME/.local/bin"
RUN_DIR="${XDG_RUNTIME_DIR:-/tmp}/$APP"
STATE_DIR="$HOME/.local/state/$APP"

echo "Stopping resident collector service..."
systemctl --user disable --now linux-router-monitor.service 2>/dev/null || true
rm -f ~/.config/systemd/user/linux-router-monitor.service
systemctl --user daemon-reload 2>/dev/null || true
rm -f ~/.config/environment.d/linux-router-monitor.conf ~/.config/plasma-workspace/env/linux-router-monitor.sh

echo "Removing widgets..."
for id in system network wifi dns clients speedtest panel; do
    kpackagetool6 -t Plasma/Applet -r "org.devl0rd.routermon.$id" >/dev/null 2>&1 \
        && echo "  removed org.devl0rd.routermon.$id" || true
done

echo "Removing helper script links..."
rm -f "$BIN_DIR/routermon-collect" "$BIN_DIR/routermon-ctl" "$BIN_DIR/routermon-speedtest"

echo "Clearing runtime cache..."
rm -rf "$RUN_DIR"

# Best-effort: remove the collector from the router.
if [ -f "$CFG_DIR/config.json" ] && command -v python3 >/dev/null 2>&1; then
    HOST=$(python3 -c "import json;print(json.load(open('$CFG_DIR/config.json')).get('host',''))" 2>/dev/null || true)
    USER=$(python3 -c "import json;print(json.load(open('$CFG_DIR/config.json')).get('user',''))" 2>/dev/null || true)
    KEY=$(python3 -c "import json;print(json.load(open('$CFG_DIR/config.json')).get('ssh_key',''))" 2>/dev/null || true)
    REMOTE=$(python3 -c "import json;print(json.load(open('$CFG_DIR/config.json')).get('remote_script','/jffs/lrm-collect.sh'))" 2>/dev/null || true)
    KEY="${KEY/#\~/$HOME}"
    if [ -n "$HOST" ] && [ -n "$KEY" ]; then
        ssh -o BatchMode=yes -o ConnectTimeout=6 -i "$KEY" "$USER@$HOST" "rm -f $REMOTE" 2>/dev/null \
            && echo "Removed remote collector from $HOST" || true
    fi
fi

echo ""
echo "Uninstallation complete."
echo "Kept your config and logs:"
echo "  $CFG_DIR  (router + AdGuard credentials)"
echo "  $STATE_DIR  (log)"
echo "Delete them manually if you want them gone."
