#!/usr/bin/env bash
set -euo pipefail

SOURCE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SOURCE_DIR/extras/packaging/common.sh"

BUILD_DIR="${KONVEYOR_BUILD_DIR:-$SOURCE_DIR/build-release}"
SKIP_DEPS=false
SKIP_PULL=false
WIDGETS=true
AUR=false
[[ ${KONVEYOR_AUR:-} == @(1|true|yes) ]] && AUR=true
MODE=install
OPTIONS_FILE="$HOME/.local/state/konveyor/install-options"
UPDATE_PENDING="$HOME/.local/state/konveyor/update-pending"

usage() {
    cat <<EOF
Usage: ./install.sh [--skip-deps] [--no-pull] [--no-widgets] [--aur]

Builds and installs Konveyor, enables it in KWin, installs the Konveyor widgets and, on
pacman-based systems, keeps a git checkout updated: every system update pulls new commits
and reinstalls, and KWin or Plasma updates rebuild it. Run it again at any time to update.

  --skip-deps   Do not install build and widget dependencies with the system package manager
  --no-pull     Do not update the source checkout with git pull
  --no-widgets  Install only the window manager, without the Konveyor widgets
  --aur         Installed by a package (also KONVEYOR_AUR=true); the package manager handles
                updates, so no update hook is registered
EOF
}

parse_arguments() {
    for argument in "$@"; do
        case "$argument" in
        --skip-deps) SKIP_DEPS=true ;;
        --no-pull) SKIP_PULL=true ;;
        --no-widgets) WIDGETS=false ;;
        --aur) AUR=true ;;
        --system-update) MODE=system-update ;;
        --finish-update) MODE=finish-update ;;
        -h | --help) usage; exit 0 ;;
        *) die "unknown option: $argument" ;;
        esac
    done
}

update_checkout() {
    if ! $SKIP_PULL && git -C "$SOURCE_DIR" rev-parse --abbrev-ref '@{upstream}' >/dev/null 2>&1; then
        say "Updating source checkout"
        git -C "$SOURCE_DIR" pull --ff-only
    fi
    git -C "$SOURCE_DIR" submodule update --init --recursive
}

install_dependencies() {
    $SKIP_DEPS && return
    say "Installing build dependencies"
    "$SOURCE_DIR/extras/packaging/dependencies.sh"
}

build() {
    say "Building (this takes a minute)"
    as_owner cmake -S "$SOURCE_DIR" -B "$BUILD_DIR" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$KONVEYOR_PREFIX" -DKONVEYOR_BUILD_TESTS=OFF >/dev/null
    as_owner cmake --build "$BUILD_DIR"
}

remove_stale_files() {
    local previous="$KONVEYOR_STATE_DIR/install_manifest.txt" current="$BUILD_DIR/install_manifest.txt" file
    [[ -f $previous ]] || return 0
    while IFS= read -r file; do
        [[ $file == "$KONVEYOR_PREFIX"/* && $file != *..* && ( -f $file || -L $file ) ]] || continue
        run_root rm -f "$file"
    done < <(comm -23 <(sort -u "$previous") <(sort -u "$current"))
}

install_files() {
    say "Installing to $KONVEYOR_PREFIX"
    run_root cmake --install "$BUILD_DIR" >/dev/null
    remove_stale_files
    run_root install -Dm644 "$BUILD_DIR/install_manifest.txt" "$KONVEYOR_STATE_DIR/install_manifest.txt"
}

install_versioned_plugin() {
    PLUGIN_ID="konveyor_effect_$(date +%s)"
    run_root install -Dm755 "$BUILD_DIR/bin/kwin/effects/plugins/konveyor_effect.so" "$KONVEYOR_PLUGIN_DIR/${PLUGIN_ID}.so"
    run_root rm -f "$KONVEYOR_PLUGIN_DIR/konveyor_effect.so"
    printf '%s\n' "$PLUGIN_ID" | run_root tee "$KONVEYOR_STATE_DIR/plugin-id" >/dev/null
}

register_updates() {
    if $AUR || ! command -v pacman >/dev/null || ! git -C "$SOURCE_DIR" rev-parse --git-dir >/dev/null 2>&1; then
        unregister_updates
        return 0
    fi
    say "Registering Konveyor with system updates"
    run_root install -Dm644 "$SOURCE_DIR/extras/packaging/konveyor-rebuild.hook" "$KONVEYOR_HOOK"
    printf '%s\n%s\n' "$SOURCE_DIR" "${KONVEYOR_OWNER:-$(id -un)}" | run_root tee "$KONVEYOR_STATE_DIR/source" >/dev/null
    $SYSTEM_UPDATE_ROOT && return 0
    local units="${XDG_CONFIG_HOME:-$HOME/.config}/systemd/user"
    mkdir -p "$units"
    sed "s|@SOURCE_DIR@|$SOURCE_DIR|g" "$SOURCE_DIR/extras/packaging/konveyor-update.service.in" >"$units/$KONVEYOR_UPDATE_UNIT"
    systemctl --user daemon-reload
    systemctl --user enable "$KONVEYOR_UPDATE_UNIT" >/dev/null 2>&1
}

unregister_updates() {
    [[ -e $KONVEYOR_HOOK || -e $KONVEYOR_STATE_DIR/source ]] && run_root rm -f "$KONVEYOR_HOOK" "$KONVEYOR_STATE_DIR/source"
    local unit="${XDG_CONFIG_HOME:-$HOME/.config}/systemd/user/$KONVEYOR_UPDATE_UNIT"
    if [[ -e $unit ]]; then
        systemctl --user disable "$KONVEYOR_UPDATE_UNIT" >/dev/null 2>&1 || true
        rm -f "$unit"
    fi
    return 0
}

remove_previous_plugin_files() {
    local file
    for file in "$KONVEYOR_PLUGIN_DIR"/konveyor_effect*.so; do
        [[ -e $file && $(basename "$file" .so) != "$PLUGIN_ID" ]] && run_root rm -f "$file"
    done
    return 0
}

configure_kwin() {
    local previous
    for previous in $(konveyor_loaded_plugin_ids | sort -u); do
        [[ $previous == "$PLUGIN_ID" ]] || konveyor_disable_plugin_id "$previous"
    done
    say "Enabling Konveyor in KWin"
    for script in "${KONVEYOR_CONFLICTING_SCRIPTS[@]}"; do
        kwinrc_write Plugins "${script}Enabled" false
    done
    kwinrc_write Plugins konveyor_effectEnabled false
    kwinrc_write Plugins "${PLUGIN_ID}Enabled" true
    if $WIDGETS; then
        say "Enabling Process Monitor frame telemetry in KWin"
        kwinrc_write Plugins process_monitor_telemetryEnabled true
    else
        kwinrc_delete Plugins process_monitor_telemetryEnabled
        kwin_dbus /Effects org.kde.kwin.Effects.unloadEffect process_monitor_telemetry
    fi
    kwin_dbus /KWin reconfigure
}

activate() {
    kwin_dbus /Effects org.kde.kwin.Effects.loadEffect "$PLUGIN_ID"
    $WIDGETS && kwin_dbus /Effects org.kde.kwin.Effects.loadEffect process_monitor_telemetry
    sleep 1
    if qdbus6 org.kde.KWin /Effects org.kde.kwin.Effects.isEffectLoaded "$PLUGIN_ID" 2>/dev/null | grep -q true; then
        say "Konveyor is live now — no logout needed. Press Super+K for the shortcut cheatsheet."
    else
        say "Installed. Log out and back in to start it."
    fi
    if $WIDGETS && qdbus6 org.kde.KWin /Effects org.kde.kwin.Effects.isEffectLoaded process_monitor_telemetry 2>/dev/null | grep -q true; then
        say "Process Monitor frame telemetry is live now"
    fi
}

finish_update() {
    PLUGIN_ID=$(<"$KONVEYOR_STATE_DIR/plugin-id")
    if grep -qx "widgets=false" "$OPTIONS_FILE" 2>/dev/null; then
        WIDGETS=false
    fi
    configure_kwin
    activate
    if $WIDGETS; then
        "$SOURCE_DIR/widgets/install.sh" --no-restart
    fi
    rm -f "$UPDATE_PENDING"
    notify_owner "Konveyor updated" "Konveyor $(git -C "$SOURCE_DIR" describe --always --tags 2>/dev/null) is installed. Restart Plasma or log out and back in to load the updated widgets."
}

system_update() {
    [[ $EUID -eq 0 && -n ${KONVEYOR_OWNER:-} ]] || die "--system-update runs from the pacman hook"
    SYSTEM_UPDATE_ROOT=true
    as_owner git -C "$SOURCE_DIR" submodule update --init --recursive --quiet
    build
    install_files
    install_versioned_plugin
    register_updates
    remove_previous_plugin_files
    if owner_session_running; then
        as_owner "$SOURCE_DIR/install.sh" --finish-update
    else
        local pending
        pending="$(getent passwd "$KONVEYOR_OWNER" | cut -d: -f6)/.local/state/konveyor/update-pending"
        as_owner mkdir -p "$(dirname "$pending")"
        as_owner touch "$pending"
        say "Konveyor was built; its session steps run at your next login"
    fi
}

main() {
    parse_arguments "$@"
    SYSTEM_UPDATE_ROOT=false
    case "$MODE" in
    system-update) system_update; return ;;
    finish-update) finish_update; return ;;
    esac
    [[ $EUID -ne 0 ]] || die "run install.sh as your normal user; it asks for sudo when needed"
    update_checkout
    install_dependencies
    build
    install_files
    install_versioned_plugin
    register_updates
    remove_previous_plugin_files
    configure_kwin
    activate
    mkdir -p "$(dirname "$OPTIONS_FILE")"
    printf 'widgets=%s\n' "$WIDGETS" >"$OPTIONS_FILE"
    if $WIDGETS; then
        "$SOURCE_DIR/widgets/install.sh"
        say "Settings: press Meta+K and open Settings in the Kontrol Panel"
    else
        say "Settings live in the Kontrol Panel, which comes with the widgets; without them, edit ~/.config/konveyor/config.kdl"
    fi
    say "Config file: ~/.config/konveyor/config.kdl (created on first start)"
}

main "$@"
