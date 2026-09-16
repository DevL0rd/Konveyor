#!/usr/bin/env bash
set -euo pipefail

SOURCE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SOURCE_DIR/extras/packaging/common.sh"

PURGE=false

usage() {
    cat <<EOF
Usage: ./uninstall.sh [--purge]

Disables Konveyor in KWin and removes every installed file.

  --purge  Also delete your config at ~/.config/konveyor
EOF
}

parse_arguments() {
    for argument in "$@"; do
        case "$argument" in
        --purge) PURGE=true ;;
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
    remove_files
    purge_config
    say "Konveyor is uninstalled. Log out and back in to fully unload it from KWin."
}

main "$@"
