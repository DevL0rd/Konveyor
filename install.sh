#!/usr/bin/env bash
set -euo pipefail

INSTALL_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SOURCE_DIR="${KONVEYOR_SOURCE_DIR:-$INSTALL_DIR}"
INSTALL_SUPPORT="${KONVEYOR_INSTALL_SUPPORT:-$SOURCE_DIR/extras/packaging}"
source "$INSTALL_SUPPORT/common.sh"
source "$INSTALL_SUPPORT/updates.sh"

BUILD_DIR="${KONVEYOR_BUILD_DIR:-$SOURCE_DIR/build-release}"
SKIP_DEPS=false
SKIP_PULL=false
WIDGETS=true
AUR=false
[[ ${KONVEYOR_AUR:-} == @(1|true|yes) ]] && AUR=true
MODE=install
OPTIONS_FILE="$HOME/.local/state/konveyor/install-options"
UPDATE_PENDING="$HOME/.local/state/konveyor/update-pending"
WIDGETS_RUNTIME_DIR="${XDG_DATA_HOME:-$HOME/.local/share}/konveyor/widgets"

usage() {
    cat <<EOF
Usage: ./install.sh [--skip-deps] [--no-pull] [--no-widgets] [--aur]

Builds and installs Konveyor, enables it in KWin, installs the Konveyor widgets and keeps a git
checkout updated: every system update pulls new commits and reinstalls, and KWin, Qt or Plasma
updates rebuild it. Run it again at any time to update.

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
        --login-update) MODE=login-update ;;
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
    local system_paths=OFF
    [[ $KONVEYOR_PREFIX == /usr ]] && system_paths=ON
    as_owner "${KONVEYOR_BUILD_ENV[@]}" cmake -S "$SOURCE_DIR" -B "$BUILD_DIR" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$KONVEYOR_PREFIX" \
        -DKDE_INSTALL_USE_QT_SYS_PATHS="$system_paths" -DKONVEYOR_BUILD_TESTS=OFF >/dev/null
    local clean=()
    [[ $(system_fingerprint) == "$(cat "$KONVEYOR_BUILT_FOR" 2>/dev/null)" ]] || clean=(--clean-first)
    as_owner "${KONVEYOR_BUILD_ENV[@]}" cmake --build "$BUILD_DIR" "${clean[@]}"
}

remove_stale_files() {
    local previous="$KONVEYOR_STATE_DIR/install_manifest.txt" current="$BUILD_DIR/install_manifest.txt" file
    [[ -f $previous ]] || return 0
    while IFS= read -r file; do
        [[ $file == "$KONVEYOR_PREFIX"/* && $file != *..* && ( -f $file || -L $file ) ]] || continue
        run_prefix rm -f "$file"
    done < <(comm -23 <(sort -u "$previous") <(sort -u "$current"))
}

install_files() {
    say "Installing to $KONVEYOR_PREFIX"
    run_prefix "${KONVEYOR_BUILD_ENV[@]}" cmake --install "$BUILD_DIR" >/dev/null
    remove_stale_files
    run_prefix install -Dm644 "$BUILD_DIR/install_manifest.txt" "$KONVEYOR_STATE_DIR/install_manifest.txt"
    KONVEYOR_PLUGIN_DIR=$(konveyor_plugin_dir)
}

install_versioned_plugin() {
    PLUGIN_ID="konveyor_effect_$(date +%s)"
    run_prefix install -Dm755 "$BUILD_DIR/bin/kwin/effects/plugins/konveyor_effect.so" "$KONVEYOR_PLUGIN_DIR/${PLUGIN_ID}.so"
    run_prefix rm -f "$KONVEYOR_PLUGIN_DIR/konveyor_effect.so"
    printf '%s\n' "$PLUGIN_ID" | run_prefix tee "$KONVEYOR_STATE_DIR/plugin-id" >/dev/null
    if $WIDGETS; then
        TELEMETRY_PLUGIN_ID="process_monitor_telemetry_$(date +%s)"
        run_prefix install -Dm755 "$BUILD_DIR/bin/kwin/effects/plugins/process_monitor_telemetry.so" "$KONVEYOR_PLUGIN_DIR/${TELEMETRY_PLUGIN_ID}.so"
        printf '%s\n' "$TELEMETRY_PLUGIN_ID" | run_prefix tee "$KONVEYOR_STATE_DIR/telemetry-plugin-id" >/dev/null
    fi
    run_prefix rm -f "$KONVEYOR_PLUGIN_DIR/process_monitor_telemetry.so"
}

remove_previous_plugin_files() {
    local file
    for file in "$KONVEYOR_PLUGIN_DIR"/konveyor_effect*.so; do
        [[ -e $file && $(basename "$file" .so) != "$PLUGIN_ID" ]] && run_prefix rm -f "$file"
    done
    for file in "$KONVEYOR_PLUGIN_DIR"/process_monitor_telemetry*.so; do
        [[ -e $file && $(basename "$file" .so) != "${TELEMETRY_PLUGIN_ID:-}" ]] && run_prefix rm -f "$file"
    done
    remove_misplaced_plugins
}

configure_kwin() {
    local previous
    for previous in $(konveyor_loaded_plugin_ids | sort -u); do
        [[ $previous == "$PLUGIN_ID" ]] || konveyor_disable_plugin_id "$previous"
    done
    say "Enabling Konveyor in KWin"
    disable_conflicting_scripts
    kwinrc_write Plugins konveyor_effectEnabled false
    kwinrc_write Plugins "${PLUGIN_ID}Enabled" true
    if $WIDGETS; then
        say "Enabling Process Monitor frame telemetry in KWin"
        for previous in $(process_monitor_telemetry_plugin_ids | sort -u); do
            [[ $previous == "$TELEMETRY_PLUGIN_ID" ]] || {
                kwinrc_delete Plugins "${previous}Enabled"
                kwin_dbus /Effects org.kde.kwin.Effects.unloadEffect "$previous"
            }
        done
        kwinrc_write Plugins process_monitor_telemetryEnabled false
        kwinrc_write Plugins "${TELEMETRY_PLUGIN_ID}Enabled" true
    else
        for previous in $(process_monitor_telemetry_plugin_ids | sort -u); do
            kwinrc_delete Plugins "${previous}Enabled"
            kwin_dbus /Effects org.kde.kwin.Effects.unloadEffect "$previous"
        done
    fi
    kwin_dbus /KWin org.kde.KWin.reconfigure
}

add_session_path() {
    local current
    current=$(systemctl --user show-environment | sed -n "s/^$1=//p")
    [[ ":$current:" == *":$2:"* ]] || systemctl --user set-environment "$1=$2${current:+:$current}"
}

configure_session_paths() {
    $KONVEYOR_ATOMIC || return 0
    local plugins qml
    plugins="${KONVEYOR_PLUGIN_DIR%/kwin/effects/plugins}"
    qml=$(manifest_entry "$KONVEYOR_STATE_DIR/install_manifest.txt" '/org/kde/konveyor/settings/qmldir$')
    qml="${qml%/org/kde/konveyor/settings/qmldir}"
    say "Adding Konveyor to the session's Qt plugin and QML paths"
    mkdir -p "$(dirname "$KONVEYOR_SESSION_ENV")"
    printf 'QT_PLUGIN_PATH=%s${QT_PLUGIN_PATH:+:$QT_PLUGIN_PATH}\nQML_IMPORT_PATH=%s${QML_IMPORT_PATH:+:$QML_IMPORT_PATH}\n' \
        "$plugins" "$qml" >"$KONVEYOR_SESSION_ENV"
    add_session_path QT_PLUGIN_PATH "$plugins"
    add_session_path QML_IMPORT_PATH "$qml"
}

effect_loaded() {
    gdbus call --session --dest org.kde.KWin --object-path /Effects --method org.kde.kwin.Effects.isEffectLoaded "$1" 2>/dev/null | grep -q true
}

activate() {
    kwin_dbus /Effects org.kde.kwin.Effects.loadEffect "$PLUGIN_ID"
    $WIDGETS && kwin_dbus /Effects org.kde.kwin.Effects.loadEffect "$TELEMETRY_PLUGIN_ID"
    sleep 1
    if effect_loaded "$PLUGIN_ID"; then
        say "Konveyor is live now — no logout needed. Press Super+K for the shortcut cheatsheet."
    else
        say "Installed. Log out and back in to start it."
    fi
    if $WIDGETS && effect_loaded "$TELEMETRY_PLUGIN_ID"; then
        say "Process Monitor frame telemetry is live now"
    fi
}

finish_update() {
    KONVEYOR_PLUGIN_DIR=$(konveyor_plugin_dir)
    PLUGIN_ID=$(<"$KONVEYOR_STATE_DIR/plugin-id")
    if grep -qx "widgets=false" "$OPTIONS_FILE" 2>/dev/null; then
        WIDGETS=false
    fi
    if $WIDGETS; then
        TELEMETRY_PLUGIN_ID=$(<"$KONVEYOR_STATE_DIR/telemetry-plugin-id")
    fi
    configure_kwin
    configure_session_paths
    activate
    if $WIDGETS; then
        local widget_installer="$WIDGETS_RUNTIME_DIR/install.sh"
        [[ -x $widget_installer ]] || widget_installer="$SOURCE_DIR/widgets/install.sh"
        KONVEYOR_WIDGETS_SOURCE="$SOURCE_DIR/widgets" "$widget_installer" --no-restart
    fi
    rm -f "$UPDATE_PENDING"
    notify_owner "Konveyor updated" "Konveyor $(git -C "$SOURCE_DIR" describe --always --tags 2>/dev/null) is installed. Restart Plasma or log out and back in to load the updated widgets."
}

system_update() {
    [[ $EUID -eq 0 && -n ${KONVEYOR_OWNER:-} ]] || die "--system-update runs from the system update hook"
    SYSTEM_UPDATE_ROOT=true
    source_git submodule update --init --recursive --quiet
    build
    install_files
    install_versioned_plugin
    record_fingerprint
    register_updates
    remove_previous_plugin_files
    if owner_session_running; then
        as_owner env KONVEYOR_SOURCE_DIR="$SOURCE_DIR" KONVEYOR_INSTALL_SUPPORT="$KONVEYOR_UPDATER_DIR" \
            "$KONVEYOR_UPDATER_DIR/install" --finish-update
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
    login-update) login_update; return ;;
    esac
    [[ $EUID -ne 0 ]] || die "run install.sh as your normal user; it asks for sudo when needed"
    update_checkout
    install_dependencies
    build
    install_files
    install_versioned_plugin
    record_fingerprint
    register_updates
    remove_previous_plugin_files
    configure_kwin
    configure_session_paths
    activate
    mkdir -p "$(dirname "$OPTIONS_FILE")"
    printf 'widgets=%s\n' "$WIDGETS" >"$OPTIONS_FILE"
    if $WIDGETS; then
        "$SOURCE_DIR/widgets/install.sh"
        say "Settings: press Meta+K and open Settings in the Kontrol Panel, or open System Settings > Window Management > Konveyor"
    else
        say "Settings: open System Settings > Window Management > Konveyor"
    fi
    say "Config file: ~/.config/konveyor/config.kdl (created on first start)"
}

main "$@"
