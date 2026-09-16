#!/bin/bash
set -e

REPO_DIR=$(pwd)
if [ ! -f "$REPO_DIR/bin/routermon-collect" ]; then
    echo "Please run this script from the repository directory."
    exit 1
fi

APP="Linux-Router-Monitor"
CFG_DIR="$HOME/.config/$APP"
BIN_DIR="$HOME/.local/bin"
PLASMOID_SRC="$REPO_DIR/plasmoids"
PLASMA_SERVICE="plasma-plasmashell.service"
PLASMA_OVERRIDE_DIR="${XDG_CONFIG_HOME:-$HOME/.config}/systemd/user/$PLASMA_SERVICE.d"
PLASMA_OVERRIDE="$PLASMA_OVERRIDE_DIR/linux-router-monitor.conf"

configure_plasma_local_file_access() {
    if [ -L "$PLASMA_OVERRIDE" ]; then
        echo "Error: refusing to overwrite symbolic link $PLASMA_OVERRIDE" >&2
        exit 1
    fi
    mkdir -p "$PLASMA_OVERRIDE_DIR" "$HOME/.config/environment.d"
    printf '[Service]\nEnvironment=QML_XHR_ALLOW_FILE_READ=1\n' > "$PLASMA_OVERRIDE"
    chmod 0644 "$PLASMA_OVERRIDE"
    printf 'QML_XHR_ALLOW_FILE_READ=1\n' > "$HOME/.config/environment.d/linux-router-monitor.conf"
    systemctl --user set-environment QML_XHR_ALLOW_FILE_READ=1 2>/dev/null || true
    systemctl --user daemon-reload
    echo "Enabled local file access for managed Plasma sessions."
}

# --- sanity check for tooling the widgets shell out to ---
for bin in ssh jq curl python3 kpackagetool6; do
    command -v "$bin" >/dev/null 2>&1 || echo "Warning: '$bin' is not installed or not in PATH."
done

# --- 1. config (git-ignored, holds router + AdGuard credentials) ---
mkdir -p "$CFG_DIR"
if [ ! -f "$CFG_DIR/config.json" ]; then
    cp "$REPO_DIR/config.example.json" "$CFG_DIR/config.json"
    echo "Created $CFG_DIR/config.json from the template."
    echo "  -> Edit it to set your router host/user and AdGuard Home login."
else
    echo "Keeping existing $CFG_DIR/config.json"
fi

# --- 2. helper scripts onto PATH (symlinked back to the repo) ---
mkdir -p "$BIN_DIR"
chmod +x "$REPO_DIR/bin/routermon-collect" "$REPO_DIR/bin/routermon-ctl" "$REPO_DIR/bin/routermon-speedtest" "$REPO_DIR/router/collect.sh"
ln -sf "$REPO_DIR/bin/routermon-collect" "$BIN_DIR/routermon-collect"
ln -sf "$REPO_DIR/bin/routermon-ctl" "$BIN_DIR/routermon-ctl"
ln -sf "$REPO_DIR/bin/routermon-speedtest" "$BIN_DIR/routermon-speedtest"
echo "Linked helper scripts into $BIN_DIR"

# --- resident collector (systemd --user): keeps the tmpfs snapshot fresh so the
#     widgets only ever read a file in-process (no per-poll process spawns) ---
mkdir -p ~/.config/systemd/user
# Pin the light resident collector to efficiency/compact cores when the CPU is
# hybrid (Intel E, AMD Zen 5c, ARM LITTLE). Detector returns nothing otherwise.
AFFINITY=""
ECORES=$(python3 -S "$REPO_DIR/bin/routermon-ecores" 2>/dev/null)
[ -n "$ECORES" ] && AFFINITY="CPUAffinity=$ECORES" && echo "Pinning collector to efficiency cores: $ECORES"
cat <<EOF > ~/.config/systemd/user/linux-router-monitor.service
[Unit]
Description=Linux-Router-Monitor resident collector
After=graphical-session.target

[Service]
Type=simple
WorkingDirectory=$REPO_DIR
ExecStart=/usr/bin/python3 -S $REPO_DIR/bin/routermon-collect --serve
Restart=always
RestartSec=3
Nice=19
$AFFINITY

[Install]
WantedBy=default.target
EOF
systemctl --user daemon-reload
systemctl --user enable --now linux-router-monitor.service
echo "Enabled resident collector service (linux-router-monitor.service)"

# --- allow the widgets to read the tmpfs snapshot in-process via QML XHR ---
# The environment file covers login sessions; the service override also covers
# every managed mid-session Plasma restart.
configure_plasma_local_file_access
rm -f ~/.config/plasma-workspace/env/linux-router-monitor.sh                    # migrate off old login-only location
case ":$PATH:" in
    *":$BIN_DIR:"*) ;;
    *) echo "  Note: $BIN_DIR is not in your PATH (widgets use absolute paths, so this is fine)." ;;
esac

# --- 3. push the remote collector to the router ---
read_cfg() { python3 -c "import json;print(json.load(open('$CFG_DIR/config.json')).get('$1',''))"; }
read_cfg_nested() { python3 -c "import json;print(json.load(open('$CFG_DIR/config.json')).get('$1',{}).get('$2',''))"; }
HOST=$(read_cfg host); USER=$(read_cfg user); KEY=$(read_cfg ssh_key)
REMOTE=$(read_cfg remote_script); [ -z "$REMOTE" ] && REMOTE="/jffs/lrm-collect.sh"
KEY="${KEY/#\~/$HOME}"
if [ -n "$HOST" ] && [ -n "$KEY" ]; then
    echo "Pushing remote collector to $USER@$HOST:$REMOTE ..."
    if ssh -o BatchMode=yes -o ConnectTimeout=8 -o StrictHostKeyChecking=accept-new \
           -i "$KEY" "$USER@$HOST" "cat > $REMOTE && chmod +x $REMOTE" < "$REPO_DIR/router/collect.sh"; then
        echo "  Remote collector installed."
    else
        echo "  Could not reach the router. Fix SSH/config and re-run, or push manually:"
        echo "    ssh $USER@$HOST 'cat > $REMOTE && chmod +x $REMOTE' < router/collect.sh"
    fi
fi

# --- 4. install the plasmoids ---
if [ ! -e "$REPO_DIR/shared/common/FileWatcher.qml" ]; then
    echo "  ! shared/common (Linux-Plasma-Shared submodule) is empty." >&2
    echo "    Run: git submodule update --init --recursive" >&2
    exit 1
fi
echo "Installing widgets..."
UI_SRC="$PLASMOID_SRC/org.devl0rd.routermon.panel"
for d in "$PLASMOID_SRC"/org.devl0rd.routermon.*; do
    if [ "$d" != "$UI_SRC" ]; then
        rm -rf "$d/contents/ui/tabs"
        cp "$UI_SRC/contents/ui/"*.qml "$d/contents/ui/"
        cp -r "$UI_SRC/contents/ui/tabs" "$d/contents/ui/"
        cp "$UI_SRC/contents/config/main.xml" "$d/contents/config/"
    fi
    mkdir -p "$d/contents/ui/lib"
    cp -r "$REPO_DIR/shared/lib" "$d/contents/ui/"   # repo-specific components -> ui/lib/
    cp "$REPO_DIR/shared/common/"*.qml "$REPO_DIR/shared/common/"*.js "$d/contents/ui/lib/"  # shared (submodule) components
    if kpackagetool6 -t Plasma/Applet -u "$d" >/dev/null 2>&1; then
        echo "  upgraded $(basename "$d")"
    else
        kpackagetool6 -t Plasma/Applet -i "$d" >/dev/null 2>&1 && echo "  installed $(basename "$d")"
    fi
done

echo ""
echo "Done! Widgets available: Router (Panel), System, Network, WiFi, DNS, Clients, Speed Test."
echo "Add them via right-click desktop/panel -> Add Widgets -> search \"Router\"."
echo "Logs: $HOME/.local/state/$APP/monitor.log"

echo "Restarting Plasma…"
systemctl --user restart "$PLASMA_SERVICE"
