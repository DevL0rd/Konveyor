#!/usr/bin/env bash

KONVEYOR_PREFIX="${KONVEYOR_PREFIX:-/usr}"
KONVEYOR_STATE_DIR="$KONVEYOR_PREFIX/share/konveyor"
KONVEYOR_HOOK="/etc/pacman.d/hooks/konveyor-rebuild.hook"
KONVEYOR_PLUGIN_DIR="$KONVEYOR_PREFIX/lib/qt6/plugins/kwin/effects/plugins"
KONVEYOR_UPDATE_UNIT="konveyor-update.service"
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

as_owner() {
    if [[ $EUID -ne 0 || -z ${KONVEYOR_OWNER:-} ]]; then
        "$@"
        return
    fi
    local uid home
    uid=$(id -u "$KONVEYOR_OWNER")
    home=$(getent passwd "$KONVEYOR_OWNER" | cut -d: -f6)
    runuser -u "$KONVEYOR_OWNER" -- env -i HOME="$home" USER="$KONVEYOR_OWNER" LOGNAME="$KONVEYOR_OWNER" \
        PATH="$home/.local/bin:/usr/local/bin:/usr/bin:/bin" LANG="${LANG:-C.UTF-8}" \
        XDG_RUNTIME_DIR="/run/user/$uid" DBUS_SESSION_BUS_ADDRESS="unix:path=/run/user/$uid/bus" "$@"
}

owner_session_running() {
    [[ -S /run/user/$(id -u "${KONVEYOR_OWNER:-$(id -un)}")/bus ]]
}

notify_owner() {
    as_owner gdbus call --session --dest org.freedesktop.Notifications --object-path /org/freedesktop/Notifications \
        --method org.freedesktop.Notifications.Notify Konveyor 0 system-software-update "$1" "$2" '[]' '{}' 10000 >/dev/null 2>&1 || true
}

kwin_dbus() {
    command -v qdbus6 >/dev/null || return 0
    as_owner qdbus6 org.kde.KWin "$@" >/dev/null 2>&1 || true
}

kwinrc_write() {
    as_owner kwriteconfig6 --file kwinrc --group "$1" --key "$2" "$3"
}

kwinrc_delete() {
    as_owner kwriteconfig6 --file kwinrc --group "$1" --key "$2" --delete
}

konveyor_loaded_plugin_ids() {
    grep -oE '^konveyor_effect[A-Za-z0-9_]*Enabled' "${XDG_CONFIG_HOME:-$HOME/.config}/kwinrc" 2>/dev/null | sed 's/Enabled$//'
    if command -v qdbus6 >/dev/null; then
        qdbus6 org.kde.KWin /Effects org.kde.kwin.Effects.loadedEffects 2>/dev/null | grep -E '^konveyor_effect' || true
    fi
    find "$KONVEYOR_PLUGIN_DIR" -maxdepth 1 -name 'konveyor_effect*.so' -printf '%f\n' 2>/dev/null | sed 's/\.so$//'
}

process_monitor_telemetry_plugin_ids() {
    grep -oE '^process_monitor_telemetry[A-Za-z0-9_]*Enabled' "${XDG_CONFIG_HOME:-$HOME/.config}/kwinrc" 2>/dev/null | sed 's/Enabled$//'
    if command -v qdbus6 >/dev/null; then
        qdbus6 org.kde.KWin /Effects org.kde.kwin.Effects.loadedEffects 2>/dev/null | grep -E '^process_monitor_telemetry' || true
    fi
    find "$KONVEYOR_PLUGIN_DIR" -maxdepth 1 -name 'process_monitor_telemetry*.so' -printf '%f\n' 2>/dev/null | sed 's/\.so$//'
}

konveyor_disable_plugin_id() {
    kwinrc_delete Plugins "${1}Enabled"
    kwin_dbus /Effects org.kde.kwin.Effects.unloadEffect "$1"
}
