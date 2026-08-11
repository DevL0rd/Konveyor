#!/bin/bash
set -e

REPO_DIR=$(pwd)
if [ ! -f "$REPO_DIR/bin/logmon-collect" ]; then
    echo "Please run this script from the repository directory."
    exit 1
fi

APP="Linux-Log-Monitor"
BIN_DIR="$HOME/.local/bin"
PLASMOID_SRC="$REPO_DIR/plasmoids"
PLASMA_SERVICE="plasma-plasmashell.service"
PLASMA_OVERRIDE_DIR="${XDG_CONFIG_HOME:-$HOME/.config}/systemd/user/$PLASMA_SERVICE.d"
PLASMA_OVERRIDE="$PLASMA_OVERRIDE_DIR/linux-log-monitor.conf"

configure_plasma_local_file_access() {
    if [ -L "$PLASMA_OVERRIDE" ]; then
        echo "Error: refusing to overwrite symbolic link $PLASMA_OVERRIDE" >&2
        exit 1
    fi
    mkdir -p "$PLASMA_OVERRIDE_DIR" "$HOME/.config/environment.d"
    printf '[Service]\nEnvironment=QML_XHR_ALLOW_FILE_READ=1\n' > "$PLASMA_OVERRIDE"
    chmod 0644 "$PLASMA_OVERRIDE"
    printf 'QML_XHR_ALLOW_FILE_READ=1\n' > "$HOME/.config/environment.d/linux-log-monitor.conf"
    systemctl --user set-environment QML_XHR_ALLOW_FILE_READ=1 2>/dev/null || true
    systemctl --user daemon-reload
    echo "Enabled local file access for managed Plasma sessions."
}

# --- sanity check for the tooling we rely on ---
for bin in journalctl python3 kpackagetool6; do
    command -v "$bin" >/dev/null 2>&1 || echo "Warning: '$bin' is not installed or not in PATH."
done

# journal read access: systemd grants it to the systemd-journal / wheel / adm
# groups. Warn (don't fail) if the user is in none of them.
if ! id -nG | grep -qwE 'systemd-journal|wheel|adm'; then
    echo "Warning: your user is in none of systemd-journal/wheel/adm -- you may only"
    echo "  see your own journal. Add yourself:  sudo usermod -aG systemd-journal $USER"
fi

# --- 1. helper scripts onto PATH (symlinked back to the repo) ---
mkdir -p "$BIN_DIR"
chmod +x "$REPO_DIR/bin/logmon-collect" "$REPO_DIR/bin/logmon-ecores"
ln -sf "$REPO_DIR/bin/logmon-collect" "$BIN_DIR/logmon-collect"
echo "Linked helper scripts into $BIN_DIR"

# --- 2. resident collector (systemd --user): follows journalctl and keeps the
#        tmpfs snapshot fresh so the widget only ever reads a file in-process ---
mkdir -p ~/.config/systemd/user
# Pin the light resident collector to efficiency/compact cores when the CPU is
# hybrid (Intel E, AMD Zen 5c, ARM LITTLE). Detector returns nothing otherwise.
AFFINITY=""
ECORES=$(python3 -S "$REPO_DIR/bin/logmon-ecores" 2>/dev/null)
[ -n "$ECORES" ] && AFFINITY="CPUAffinity=$ECORES" && echo "Pinning collector to efficiency cores: $ECORES"
cat <<EOF > ~/.config/systemd/user/linux-log-monitor.service
[Unit]
Description=Linux-Log-Monitor resident collector
After=graphical-session.target

[Service]
Type=simple
WorkingDirectory=$REPO_DIR
ExecStart=/usr/bin/python3 $REPO_DIR/bin/logmon-collect --serve
Restart=always
RestartSec=3
Nice=19
$AFFINITY

[Install]
WantedBy=default.target
EOF
systemctl --user daemon-reload
systemctl --user enable --now linux-log-monitor.service
echo "Enabled resident collector service (linux-log-monitor.service)"

# --- 3. allow the widget to read the tmpfs snapshot in-process via QML XHR ---
# The environment file covers login sessions; the service override also covers
# every managed mid-session Plasma restart.
configure_plasma_local_file_access
rm -f ~/.config/plasma-workspace/env/linux-log-monitor.sh                       # migrate off old login-only location

# --- 4. install the plasmoid(s) ---
if [ ! -e "$REPO_DIR/shared/common/FileWatcher.qml" ]; then
    echo "  ! shared/common (Linux-Plasma-Shared submodule) is empty." >&2
    echo "    Run: git submodule update --init --recursive" >&2
    exit 1
fi
echo "Installing widget(s)..."
for d in "$PLASMOID_SRC"/org.devl0rd.logmon.*; do
    cp -r "$REPO_DIR/shared/lib" "$d/contents/ui/"   # repo-specific components -> ui/lib/
    cp "$REPO_DIR/shared/common/FileWatcher.qml" "$d/contents/ui/lib/"  # shared (submodule) component
    if kpackagetool6 -t Plasma/Applet -u "$d" >/dev/null 2>&1; then
        echo "  upgraded $(basename "$d")"
    else
        kpackagetool6 -t Plasma/Applet -i "$d" >/dev/null 2>&1 && echo "  installed $(basename "$d")"
    fi
done

echo ""
echo "Done! Add it via right-click desktop/panel -> Add Widgets -> search \"System Log\"."
echo "(That restart picks up the QML_XHR flag from environment.d; no logout needed.)"

echo "Restarting Plasma…"
systemctl --user restart "$PLASMA_SERVICE"
