#!/usr/bin/env bash

KONVEYOR_ATOMIC=false
KONVEYOR_BUILD_ENV=()
if [[ -e /run/ostree-booted ]]; then
    KONVEYOR_ATOMIC=true
    KONVEYOR_PREFIX="${KONVEYOR_PREFIX:-$HOME/.local}"
    KONVEYOR_TOOLBOX="konveyor-fedora-$(. /etc/os-release && printf '%s' "$VERSION_ID")"
    KONVEYOR_BUILD_ENV=(toolbox run --container "$KONVEYOR_TOOLBOX")
fi
KONVEYOR_PREFIX="${KONVEYOR_PREFIX:-/usr}"
KONVEYOR_STATE_DIR="$KONVEYOR_PREFIX/share/konveyor"
KONVEYOR_HOOK="/etc/pacman.d/hooks/konveyor-rebuild.hook"
KONVEYOR_PLUGIN_DIR=""
KONVEYOR_SESSION_ENV="${XDG_CONFIG_HOME:-$HOME/.config}/environment.d/konveyor.conf"
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

run_prefix() {
    if $KONVEYOR_ATOMIC; then
        "$@"
    else
        run_root "$@"
    fi
}

manifest_entry() {
    local entry
    entry=$(grep -m1 -E "$2" "$1") || die "$1 lists no file matching $2"
    printf '%s\n' "$entry"
}

konveyor_plugin_dir() {
    local manifest="$KONVEYOR_STATE_DIR/install_manifest.txt" plugin
    [[ -f $manifest ]] || return 0
    plugin=$(manifest_entry "$manifest" '/kwin/effects/plugins/konveyor_effect\.so$') || exit 1
    dirname "$plugin"
}

remove_misplaced_plugins() {
    local stray="$KONVEYOR_PREFIX/lib/qt6/plugins/kwin/effects/plugins" file
    [[ $stray != "$KONVEYOR_PLUGIN_DIR" && -d $stray ]] || return 0
    for file in "$stray"/konveyor_effect*.so "$stray"/process_monitor_telemetry*.so; do
        [[ -e $file ]] && run_prefix rm -f "$file"
    done
    return 0
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
    as_owner gdbus call --session --dest org.kde.KWin --object-path "$1" --method "$2" "${@:3}" >/dev/null 2>&1 || true
}

kwin_loaded_effects() {
    gdbus call --session --dest org.kde.KWin --object-path /Effects \
        --method org.freedesktop.DBus.Properties.Get org.kde.kwin.Effects loadedEffects 2>/dev/null || true
}

kwinrc_write() {
    as_owner kwriteconfig6 --file kwinrc --group "$1" --key "$2" "$3"
}

kwinrc_delete() {
    as_owner kwriteconfig6 --file kwinrc --group "$1" --key "$2" --delete
}

plugin_ids() {
    grep -oE "^$1[A-Za-z0-9_]*Enabled" "${XDG_CONFIG_HOME:-$HOME/.config}/kwinrc" 2>/dev/null | sed 's/Enabled$//'
    kwin_loaded_effects | grep -oE "\b$1[A-Za-z0-9_]*" || true
    if [[ -n $KONVEYOR_PLUGIN_DIR ]]; then
        find "$KONVEYOR_PLUGIN_DIR" -maxdepth 1 -name "$1*.so" -printf '%f\n' 2>/dev/null | sed 's/\.so$//'
    fi
    return 0
}

konveyor_loaded_plugin_ids() {
    plugin_ids konveyor_effect
}

process_monitor_telemetry_plugin_ids() {
    plugin_ids process_monitor_telemetry
}

konveyor_disable_plugin_id() {
    kwinrc_delete Plugins "${1}Enabled"
    kwin_dbus /Effects org.kde.kwin.Effects.unloadEffect "$1"
}
