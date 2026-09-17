#!/usr/bin/env bash
set -euo pipefail

SOURCE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SOURCE_DIR/extras/packaging/common.sh"

BUILD_DIR="${KONVEYOR_BUILD_DIR:-$SOURCE_DIR/build-release}"
SKIP_DEPS=false
SKIP_PULL=false
WIDGETS=true

usage() {
    cat <<EOF
Usage: ./install.sh [--skip-deps] [--no-pull] [--no-widgets]

Builds and installs Konveyor, enables it in KWin, sets up automatic rebuilds after KWin updates,
and installs the Konveyor widgets. Run it again at any time to update an existing install.

  --skip-deps   Do not install build and widget dependencies with the system package manager
  --no-pull     Do not update the source checkout with git pull
  --no-widgets  Install only the window manager, without the Konveyor widgets
EOF
}

parse_arguments() {
    for argument in "$@"; do
        case "$argument" in
        --skip-deps) SKIP_DEPS=true ;;
        --no-pull) SKIP_PULL=true ;;
        --no-widgets) WIDGETS=false ;;
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
    cmake -S "$SOURCE_DIR" -B "$BUILD_DIR" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$KONVEYOR_PREFIX" -DKONVEYOR_BUILD_TESTS=OFF >/dev/null
    cmake --build "$BUILD_DIR"
}

install_files() {
    say "Installing to $KONVEYOR_PREFIX"
    run_root cmake --install "$BUILD_DIR" >/dev/null
    run_root install -Dm644 "$BUILD_DIR/install_manifest.txt" "$KONVEYOR_STATE_DIR/install_manifest.txt"
    printf '%s\n%s\n' "$SOURCE_DIR" "$(id -un)" | run_root tee "$KONVEYOR_STATE_DIR/source" >/dev/null
}

install_versioned_plugin() {
    PLUGIN_ID="konveyor_effect_$(date +%s)"
    run_root install -Dm755 "$BUILD_DIR/bin/kwin/effects/plugins/konveyor_effect.so" "$KONVEYOR_PLUGIN_DIR/${PLUGIN_ID}.so"
    run_root rm -f "$KONVEYOR_PLUGIN_DIR/konveyor_effect.so"
    printf '%s\n' "$PLUGIN_ID" | run_root tee "$KONVEYOR_STATE_DIR/plugin-id" >/dev/null
}

install_rebuild_hook() {
    command -v pacman >/dev/null || return 0
    say "Installing pacman hook to rebuild Konveyor after KWin updates"
    run_root install -Dm644 "$SOURCE_DIR/extras/packaging/konveyor-rebuild.hook" "$KONVEYOR_HOOK"
}

unload_previous_builds() {
    local previous
    for previous in $(konveyor_loaded_plugin_ids | sort -u); do
        [[ $previous == "$PLUGIN_ID" ]] && continue
        konveyor_disable_plugin_id "$previous"
        run_root rm -f "$KONVEYOR_PLUGIN_DIR/${previous}.so"
    done
}

configure_kwin() {
    unload_previous_builds
    say "Enabling Konveyor in KWin"
    for script in "${KONVEYOR_CONFLICTING_SCRIPTS[@]}"; do
        kwinrc_write Plugins "${script}Enabled" false
    done
    kwinrc_write Plugins konveyor_effectEnabled false
    kwinrc_write Plugins "${PLUGIN_ID}Enabled" true
    kwin_dbus /KWin reconfigure
}

activate() {
    kwin_dbus /Effects org.kde.kwin.Effects.loadEffect "$PLUGIN_ID"
    sleep 1
    if qdbus6 org.kde.KWin /Effects org.kde.kwin.Effects.isEffectLoaded "$PLUGIN_ID" 2>/dev/null | grep -q true; then
        say "Konveyor is live now — no logout needed. Press Super+K for the shortcut cheatsheet."
    else
        say "Installed. Log out and back in to start it."
    fi
}

main() {
    parse_arguments "$@"
    [[ $EUID -ne 0 ]] || die "run install.sh as your normal user; it asks for sudo when needed"
    update_checkout
    install_dependencies
    build
    install_files
    install_versioned_plugin
    install_rebuild_hook
    configure_kwin
    activate
    if $WIDGETS; then
        "$SOURCE_DIR/widgets/install.sh"
    fi
    say "Config file: ~/.config/konveyor/config.kdl (created on first start)"
}

main "$@"
