#!/usr/bin/env bash

KONVEYOR_PREFIX="${KONVEYOR_PREFIX:-/usr}"
KONVEYOR_STATE_DIR="$KONVEYOR_PREFIX/share/konveyor"
KONVEYOR_HOOK="/etc/pacman.d/hooks/konveyor-rebuild.hook"
KONVEYOR_PLUGIN_DIR="$KONVEYOR_PREFIX/lib/qt6/plugins/kwin/effects/plugins"
KONVEYOR_CONFLICTING_SCRIPTS=(karousel krohnkite kzones polonium bismuth devl0rd-hide-desktop-widgets)

say() {
    printf '\033[1;34m==>\033[0m %s\n' "$*"
}

die() {
    printf '\033[1;31merror:\033[0m %s\n' "$*" >&2
    exit 1
}

run_root() {
    if [[ $EUID -eq 0 ]]; then
        "$@"
    else
        sudo "$@"
    fi
}

kwin_dbus() {
    command -v qdbus6 >/dev/null || return 0
    qdbus6 org.kde.KWin "$@" >/dev/null 2>&1 || true
}

kwinrc_write() {
    kwriteconfig6 --file kwinrc --group "$1" --key "$2" "$3"
}

kwinrc_delete() {
    kwriteconfig6 --file kwinrc --group "$1" --key "$2" --delete
}

konveyor_loaded_plugin_ids() {
    grep -oE '^konveyor_effect[A-Za-z0-9_]*Enabled' "${XDG_CONFIG_HOME:-$HOME/.config}/kwinrc" 2>/dev/null | sed 's/Enabled$//'
    if command -v qdbus6 >/dev/null; then
        qdbus6 org.kde.KWin /Effects org.kde.kwin.Effects.loadedEffects 2>/dev/null | grep -E '^konveyor_effect' || true
    fi
    find "$KONVEYOR_PLUGIN_DIR" -maxdepth 1 -name 'konveyor_effect*.so' -printf '%f\n' 2>/dev/null | sed 's/\.so$//'
}

konveyor_disable_plugin_id() {
    kwinrc_delete Plugins "${1}Enabled"
    kwin_dbus /Effects org.kde.kwin.Effects.unloadEffect "$1"
}
