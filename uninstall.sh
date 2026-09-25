#!/usr/bin/env bash
set -euo pipefail

SOURCE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SOURCE_DIR/extras/packaging/common.sh"
source "$SOURCE_DIR/extras/packaging/updates.sh"
WIDGETS_RUNTIME_DIR="${XDG_DATA_HOME:-$HOME/.local/share}/konveyor/widgets"

PURGE=false
WIDGETS=true

usage() {
    cat <<EOF
Usage: ./uninstall.sh [--purge] [--keep-widgets]

Disables Konveyor in KWin, removes every installed file and uninstalls the Konveyor widgets.

  --purge         Also delete your config at ~/.config/konveyor
  --keep-widgets  Leave the Konveyor widgets and their collector service installed
EOF
}

parse_arguments() {
    for argument in "$@"; do
        case "$argument" in
        --purge) PURGE=true ;;
        --keep-widgets) WIDGETS=false ;;
        -h | --help) usage; exit 0 ;;
        *) die "unknown option: $argument" ;;
        esac
    done
}

restore_plasma_panels() {
    gdbus call --session --dest org.kde.plasmashell --object-path /PlasmaShell --method org.kde.PlasmaShell.evaluateScript '
for (const panel of panels()) {
    panel.currentConfigGroup = ["Konveyor"];
    const saved = panel.readConfig("savedLengthMode", "");
    if (saved === "") { continue; }
    if (panel.lengthMode === "fill") { panel.lengthMode = saved; }
    panel.writeConfig("savedLengthMode", "");
}' >/dev/null 2>&1 || true
}

disable_in_kwin() {
    restore_plasma_panels

    say "Disabling Konveyor in KWin"
    local plugin
    for plugin in $(konveyor_loaded_plugin_ids | sort -u); do
        konveyor_disable_plugin_id "$plugin"
        [[ -n $KONVEYOR_PLUGIN_DIR ]] && run_prefix rm -f "$KONVEYOR_PLUGIN_DIR/${plugin}.so"
    done
    kwinrc_delete Plugins konveyor_effectEnabled
    restore_conflicting_scripts
    if $WIDGETS; then
        for plugin in $(process_monitor_telemetry_plugin_ids | sort -u); do
            kwinrc_delete Plugins "${plugin}Enabled"
            kwin_dbus /Effects org.kde.kwin.Effects.unloadEffect "$plugin"
            [[ -n $KONVEYOR_PLUGIN_DIR ]] && run_prefix rm -f "$KONVEYOR_PLUGIN_DIR/${plugin}.so"
        done
    fi
    remove_misplaced_plugins
    kwin_dbus /KWin org.kde.KWin.reconfigure

    say "Restoring the KDE shortcuts Konveyor had taken over"
    "$KONVEYOR_PREFIX/bin/konveyor" restore-shortcuts
}

remove_files() {
    local manifest="$KONVEYOR_STATE_DIR/install_manifest.txt"
    if [[ ! -f $manifest ]]; then
        say "No install manifest found at $manifest, nothing to remove"
        return
    fi
    say "Removing installed files"
    while IFS= read -r file; do
        [[ -n $file ]] && run_prefix rm -f "$file"
    done <"$manifest"
    remove_empty_directories "$manifest"
    unregister_updates
    run_prefix rm -rf "$KONVEYOR_STATE_DIR"
}

package_owns() {
    case "$(package_manager)" in
    pacman) pacman -Qoq "$1" >/dev/null 2>&1 ;;
    dnf | zypper) rpm -qf "$1" >/dev/null 2>&1 ;;
    apt-get) dpkg -S "$1" >/dev/null 2>&1 ;;
    *) return 0 ;;
    esac
}

removable_directory() {
    if $KONVEYOR_ATOMIC; then
        [[ $1/ == */konveyor/* || $1 == "$KONVEYOR_PREFIX"/*/* ]]
    else
        [[ $1 == "$KONVEYOR_PREFIX"/*/* ]] && ! package_owns "$1"
    fi
}

remove_empty_directories() {
    local directory
    while IFS= read -r directory; do
        while removable_directory "$directory" && [[ -d $directory && -z $(ls -A "$directory") ]]; do
            run_prefix rmdir "$directory"
            directory=$(dirname "$directory")
        done
    done < <(sed 's|/[^/]*$||' "$1" | sort -ru)
}

remove_session_path() {
    local current rest
    current=$(systemctl --user show-environment | sed -n "s/^$1=//p")
    [[ ":$current:" == *":$2:"* ]] || return 0
    rest=$(printf '%s' ":$current:" | sed "s|:$2:|:|; s|^:||; s|:$||")
    if [[ -n $rest ]]; then
        systemctl --user set-environment "$1=$rest"
    else
        systemctl --user unset-environment "$1"
    fi
}

remove_atomic_setup() {
    rm -f "$KONVEYOR_SESSION_ENV"
    $KONVEYOR_ATOMIC || return 0
    local name qml
    if [[ -n $KONVEYOR_PLUGIN_DIR ]]; then
        remove_session_path QT_PLUGIN_PATH "${KONVEYOR_PLUGIN_DIR%/kwin/effects/plugins}"
        qml=$(manifest_entry "$KONVEYOR_STATE_DIR/install_manifest.txt" '/org/kde/konveyor/settings/qmldir$')
        remove_session_path QML_IMPORT_PATH "${qml%/org/kde/konveyor/settings/qmldir}"
    fi
    for name in $(podman ps --all --format '{{.Names}}' | grep -E '^konveyor-(fedora-[0-9]+|steamos)$' || true); do
        say "Removing the $name build container"
        podman rm --force --volumes "$name" >/dev/null
    done
    remove_unused_pulled_images
}

remove_update_unit() {
    unregister_updates
    systemctl --user daemon-reload
}

remove_state() {
    rm -rf "${XDG_STATE_HOME:-$HOME/.local/state}/konveyor" "${XDG_STATE_HOME:-$HOME/.local/state}/konveyorstaterc"
    local directory
    for directory in "$(dirname "$KONVEYOR_SESSION_ENV")" "${XDG_CONFIG_HOME:-$HOME/.config}/systemd/user" "${XDG_CONFIG_HOME:-$HOME/.config}/systemd" "$HOME/.local/bin"; do
        [[ -d $directory ]] && rmdir --ignore-fail-on-non-empty "$directory"
    done
    return 0
}

purge_config() {
    $PURGE || return 0
    say "Deleting ~/.config/konveyor"
    rm -rf "${XDG_CONFIG_HOME:-$HOME/.config}/konveyor"
}

main() {
    parse_arguments "$@"
    [[ $EUID -ne 0 ]] || die "run uninstall.sh as your normal user; it asks for sudo when needed"
    KONVEYOR_PLUGIN_DIR=$(konveyor_plugin_dir)
    disable_in_kwin
    if $WIDGETS; then
        local widget_uninstaller="$WIDGETS_RUNTIME_DIR/uninstall.sh"
        [[ -x $widget_uninstaller ]] || widget_uninstaller="$SOURCE_DIR/widgets/uninstall.sh"
        "$widget_uninstaller"
    fi
    remove_atomic_setup
    remove_files
    remove_update_unit
    remove_state
    purge_config
    say "Konveyor is uninstalled. Log out and back in to fully unload it from KWin."
}

main "$@"
