#!/bin/bash
set -e

REPO_DIR=$(pwd)
if [ ! -f "$REPO_DIR/bin/sysmon-collect" ]; then
    echo "Please run this script from the repository directory."
    exit 1
fi

BIN_DIR="$HOME/.local/bin"
PLASMOID_SRC="$REPO_DIR/plasmoids"
CFG_DIR="$HOME/.config/Linux-System-Monitor"
PLASMA_SERVICE="plasma-plasmashell.service"
PLASMA_OVERRIDE_DIR="${XDG_CONFIG_HOME:-$HOME/.config}/systemd/user/$PLASMA_SERVICE.d"
PLASMA_OVERRIDE="$PLASMA_OVERRIDE_DIR/linux-system-monitor.conf"

configure_plasma_local_file_access() {
    if [ -L "$PLASMA_OVERRIDE" ]; then
        echo "Error: refusing to overwrite symbolic link $PLASMA_OVERRIDE" >&2
        exit 1
    fi
    mkdir -p "$PLASMA_OVERRIDE_DIR" "$HOME/.config/environment.d"
    printf '[Service]\nEnvironment=QML_XHR_ALLOW_FILE_READ=1\n' > "$PLASMA_OVERRIDE"
    chmod 0644 "$PLASMA_OVERRIDE"
    printf 'QML_XHR_ALLOW_FILE_READ=1\n' > "$HOME/.config/environment.d/linux-system-monitor.conf"
    systemctl --user set-environment QML_XHR_ALLOW_FILE_READ=1 2>/dev/null || true
    systemctl --user daemon-reload
    echo "Enabled local file access for managed Plasma sessions."
}

for bin in python3 kpackagetool6; do
    command -v "$bin" >/dev/null 2>&1 || echo "Warning: '$bin' is not installed or not in PATH."
done
python3 -c "import pynvml" >/dev/null 2>&1 || echo "Note: install python-nvidia-ml-py for GPU stats:  sudo pacman -S python-nvidia-ml-py"

# --- 1. collector onto PATH (symlinked back to the repo) ---
mkdir -p "$BIN_DIR"
chmod +x "$REPO_DIR/bin/sysmon-collect" "$REPO_DIR/bin/sysmon-ecores"
ln -sf "$REPO_DIR/bin/sysmon-collect" "$BIN_DIR/sysmon-collect"
echo "Linked sysmon-collect into $BIN_DIR"

# --- 2. config (gitignored; holds the sampling interval) ---
mkdir -p "$CFG_DIR"
[ -f "$CFG_DIR/config.json" ] || cp "$REPO_DIR/config.example.json" "$CFG_DIR/config.json"

# --- 3. let the widget read the tmpfs snapshot in-process via QML XHR ---
# The environment file covers login sessions; the service override also covers
# every managed mid-session Plasma restart.
configure_plasma_local_file_access

# --- 4. resident collector service, pinned to the E-cores ---
mkdir -p "$HOME/.config/systemd/user"
AFFINITY=""
ECORES=$(python3 -S "$REPO_DIR/bin/sysmon-ecores" 2>/dev/null)
[ -n "$ECORES" ] && AFFINITY="CPUAffinity=$ECORES" && echo "Pinning collector to efficiency cores: $ECORES"
cat > "$HOME/.config/systemd/user/linux-system-monitor.service" <<EOF
[Unit]
Description=Linux-System-Monitor resident collector
After=graphical-session.target

[Service]
Type=simple
ExecStart=/usr/bin/python3 $REPO_DIR/bin/sysmon-collect --serve
Restart=always
RestartSec=5
Nice=19
$AFFINITY

[Install]
WantedBy=default.target
EOF
systemctl --user daemon-reload
systemctl --user enable --now linux-system-monitor.service >/dev/null 2>&1 \
    && echo "Enabled resident collector (linux-system-monitor.service)" \
    || echo "  (could not enable linux-system-monitor.service -- enable it manually)"

# --- 5. install the widget ---
if [ ! -e "$REPO_DIR/shared/common/FileWatcher.qml" ]; then
    echo "  ! shared/common (Linux-Plasma-Shared submodule) is empty." >&2
    echo "    Run: git submodule update --init --recursive" >&2
    exit 1
fi
echo "Installing widget..."
UI_SRC="$PLASMOID_SRC/org.devl0rd.sysmon.panel"
for d in "$PLASMOID_SRC"/org.devl0rd.sysmon*; do
    if [ "$d" != "$UI_SRC" ]; then
        cp "$UI_SRC/contents/ui/"*.qml "$d/contents/ui/"
        cp "$UI_SRC/contents/config/main.xml" "$d/contents/config/"
    fi
    mkdir -p "$d/contents/ui/lib"
    cp "$REPO_DIR/shared/common/"*.qml "$REPO_DIR/shared/common/"*.js "$d/contents/ui/lib/"  # shared (submodule) components
    if kpackagetool6 -t Plasma/Applet -u "$d" >/dev/null 2>&1; then
        echo "  upgraded $(basename "$d")"
    else
        kpackagetool6 -t Plasma/Applet -i "$d" >/dev/null 2>&1 && echo "  installed $(basename "$d")"
    fi
done

echo ""
echo "Done! Add it via right-click panel/desktop -> Add Widgets -> search \"System Monitor\"."

echo "Restarting Plasma…"
systemctl --user restart "$PLASMA_SERVICE"
