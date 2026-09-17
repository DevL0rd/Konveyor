#!/bin/bash
set -e

REPO_DIR=$(pwd)
if [ ! -f "$REPO_DIR/bin/portal-games" ]; then
    echo "Please run this script from the repository directory."
    exit 1
fi

BIN_DIR="$HOME/.local/bin"
PLASMOID_SRC="$REPO_DIR/plasmoids"
PLASMA_SERVICE="plasma-plasmashell.service"
PLASMA_OVERRIDE_DIR="${XDG_CONFIG_HOME:-$HOME/.config}/systemd/user/$PLASMA_SERVICE.d"
PLASMA_OVERRIDE="$PLASMA_OVERRIDE_DIR/linux-plasma-portals.conf"

configure_plasma_local_file_access() {
    if [ -L "$PLASMA_OVERRIDE" ]; then
        echo "Error: refusing to overwrite symbolic link $PLASMA_OVERRIDE" >&2
        exit 1
    fi
    mkdir -p "$PLASMA_OVERRIDE_DIR" "$HOME/.config/environment.d"
    printf '[Service]\nEnvironment=QML_XHR_ALLOW_FILE_READ=1\n' > "$PLASMA_OVERRIDE"
    chmod 0644 "$PLASMA_OVERRIDE"
    printf 'QML_XHR_ALLOW_FILE_READ=1\n' > "$HOME/.config/environment.d/linux-plasma-portals.conf"
    systemctl --user set-environment QML_XHR_ALLOW_FILE_READ=1 2>/dev/null || true
    systemctl --user daemon-reload
    echo "Enabled local file access for managed Plasma sessions."
}

for bin in python3 kpackagetool6; do
    command -v "$bin" >/dev/null 2>&1 || echo "Warning: '$bin' is not installed or not in PATH."
done

# --- 1. games backend onto PATH (symlinked back to the repo) ---
mkdir -p "$BIN_DIR"
chmod +x "$REPO_DIR/bin/portal-games"
ln -sf "$REPO_DIR/bin/portal-games" "$BIN_DIR/portal-games"
chmod +x "$REPO_DIR/bin/portal-packages"
ln -sf "$REPO_DIR/bin/portal-packages" "$BIN_DIR/portal-packages"
echo "Linked portal-games into $BIN_DIR"

# --- 1b. friends-presence backend + config + resident service ---
chmod +x "$REPO_DIR/bin/portal-friends"
ln -sf "$REPO_DIR/bin/portal-friends" "$BIN_DIR/portal-friends"
echo "Linked portal-friends into $BIN_DIR"

CFG_DIR="$HOME/.config/Plasma-App-Portal"
mkdir -p "$CFG_DIR"
if [ ! -f "$CFG_DIR/config.json" ]; then
    cp "$REPO_DIR/config.example.json" "$CFG_DIR/config.json"
    echo "Created $CFG_DIR/config.json -- paste your free Steam Web API key there"
    echo "  (get one at https://steamcommunity.com/dev/apikey)"
fi

mkdir -p "$HOME/.config/systemd/user"
# pin the resident collector to the E-cores, same as the router/log collectors
AFFINITY=""
ECORES=$(python3 -S "$REPO_DIR/bin/portal-ecores" 2>/dev/null)
[ -n "$ECORES" ] && AFFINITY="CPUAffinity=$ECORES" && echo "Pinning portal-friends to efficiency cores: $ECORES"
cat > "$HOME/.config/systemd/user/portal-friends.service" <<EOF
[Unit]
Description=App Portal friends-presence collector
After=graphical-session.target

[Service]
Type=simple
ExecStart=/usr/bin/python3 $REPO_DIR/bin/portal-friends --serve
Restart=always
RestartSec=10
Nice=19
$AFFINITY

[Install]
WantedBy=default.target
EOF
systemctl --user daemon-reload
systemctl --user enable --now portal-friends.service >/dev/null 2>&1 \
    && echo "Enabled portal-friends.service (friends badge in the Games view)" \
    || echo "  (could not enable portal-friends.service -- enable it manually)"

# --- 1c. let the Friends widget read the tmpfs snapshot in-process via QML XHR ---
# The environment file covers login sessions; the service override also covers
# every managed mid-session Plasma restart.
configure_plasma_local_file_access

# --- 2. install the plasmoid(s) ---
if [ ! -e "$REPO_DIR/shared/common/FileWatcher.qml" ]; then
    echo "  ! shared/common (Linux-Plasma-Shared submodule) is empty." >&2
    echo "    Run: git submodule update --init --recursive" >&2
    exit 1
fi
echo "Installing widget(s)..."
for d in "$PLASMOID_SRC"/org.devl0rd.portal*; do
    rm -rf "$d/contents/ui/lib"
    mkdir -p "$d/contents/ui/lib"
    cp "$REPO_DIR/shared/common/"*.qml "$REPO_DIR/shared/common/"*.js "$d/contents/ui/lib/"
    cp "$REPO_DIR/shared/lib/"*.qml "$REPO_DIR/shared/lib/"*.js "$d/contents/ui/lib/"
    case "$(basename "$d")" in
        org.devl0rd.portal|org.devl0rd.portal.launcher)
            rm -rf "$d/contents/ui/pages"
            cp -r "$REPO_DIR/shared/launcher/." "$d/contents/ui/"
            ;;
    esac
    if kpackagetool6 -t Plasma/Applet -u "$d" >/dev/null 2>&1; then
        echo "  upgraded $(basename "$d")"
    else
        kpackagetool6 -t Plasma/Applet -i "$d" >/dev/null 2>&1 && echo "  installed $(basename "$d")"
    fi
done

echo ""
echo "Done! Add it via right-click panel -> Add Widgets -> search \"App Portal\"."
echo "Portal Launcher: add \"Portal Launcher\" to your panel in place of the application launcher; Meta opens it."
echo "Friends badge: add your Steam Web API key to $HOME/.config/Plasma-App-Portal/config.json"

echo "Restarting Plasma…"
systemctl --user restart "$PLASMA_SERVICE"
