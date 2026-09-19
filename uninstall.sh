#!/usr/bin/env bash
set -euo pipefail

SOURCE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SOURCE_DIR/extras/packaging/common.sh"

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
    command -v qdbus6 >/dev/null || return 0
    qdbus6 org.kde.plasmashell /PlasmaShell org.kde.PlasmaShell.evaluateScript '
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
        run_root rm -f "$KONVEYOR_PLUGIN_DIR/${plugin}.so"
    done
    kwinrc_delete Plugins konveyor_effectEnabled
    if $WIDGETS; then
        for plugin in $(process_monitor_telemetry_plugin_ids | sort -u); do
            kwinrc_delete Plugins "${plugin}Enabled"
            kwin_dbus /Effects org.kde.kwin.Effects.unloadEffect "$plugin"
            run_root rm -f "$KONVEYOR_PLUGIN_DIR/${plugin}.so"
        done
    fi
    kwin_dbus /KWin reconfigure

    say "Restoring the KDE shortcuts Konveyor had taken over"
    konveyor restore-shortcuts
}

remove_files() {
    local manifest="$KONVEYOR_STATE_DIR/install_manifest.txt"
    if [[ ! -f $manifest ]]; then
        say "No install manifest found at $manifest, nothing to remove"
        return
    fi
    say "Removing installed files"
    while IFS= read -r file; do
        [[ -n $file ]] && run_root rm -f "$file"
    done <"$manifest"
    run_root rm -f "$KONVEYOR_HOOK"
    run_root rm -rf "$KONVEYOR_STATE_DIR"
}

remove_update_unit() {
    local unit="${XDG_CONFIG_HOME:-$HOME/.config}/systemd/user/$KONVEYOR_UPDATE_UNIT"
    if [[ -e $unit ]]; then
        systemctl --user disable "$KONVEYOR_UPDATE_UNIT" >/dev/null 2>&1 || true
        rm -f "$unit"
        systemctl --user daemon-reload
    fi
    rm -f "$HOME/.local/state/konveyor/update-pending" "$HOME/.local/state/konveyor/install-options"
}

purge_config() {
    rm -f "${XDG_STATE_HOME:-$HOME/.local/state}/konveyorstaterc"
    $PURGE || return 0
    say "Deleting ~/.config/konveyor"
    rm -rf "${XDG_CONFIG_HOME:-$HOME/.config}/konveyor"
}

main() {
    parse_arguments "$@"
    [[ $EUID -ne 0 ]] || die "run uninstall.sh as your normal user; it asks for sudo when needed"
    disable_in_kwin
    if $WIDGETS; then
        "$SOURCE_DIR/widgets/uninstall.sh"
    fi
    remove_files
    remove_update_unit
    purge_config
    say "Konveyor is uninstalled. Log out and back in to fully unload it from KWin."
}

main "$@"
