#!/usr/bin/env bash
set -euo pipefail

INSTALLER_WIDGETS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SOURCE_WIDGETS_DIR="${KONVEYOR_WIDGETS_SOURCE:-$INSTALLER_WIDGETS_DIR}"
WIDGETS_RUNTIME_DIR="${XDG_DATA_HOME:-$HOME/.local/share}/konveyor/widgets"

say() {
    printf '\033[1;34m==>\033[0m %s\n' "$*"
}

die() {
    printf '\033[1;31merror:\033[0m %s\n' "$*" >&2
    exit 1
}

stage_runtime() {
    [[ -f $SOURCE_WIDGETS_DIR/lib.sh && -f $SOURCE_WIDGETS_DIR/service/konveyor-widgets ]] \
        || die "widget source is incomplete: $SOURCE_WIDGETS_DIR"
    if [[ $SOURCE_WIDGETS_DIR == "$WIDGETS_RUNTIME_DIR" ]]; then
        return
    fi
    local parent temporary previous
    parent=$(dirname "$WIDGETS_RUNTIME_DIR")
    mkdir -p "$parent"
    temporary=$(mktemp -d "$parent/.widgets.XXXXXX")
    previous="$parent/.widgets.previous.$$"
    cp -a "$SOURCE_WIDGETS_DIR/." "$temporary/"
    find "$temporary" -name .git -prune -exec rm -rf {} +
    if [[ -e $WIDGETS_RUNTIME_DIR ]]; then
        mv "$WIDGETS_RUNTIME_DIR" "$previous"
    fi
    mv "$temporary" "$WIDGETS_RUNTIME_DIR"
    rm -rf "$previous"
}

stage_runtime
WIDGETS_DIR="$WIDGETS_RUNTIME_DIR"
source "$WIDGETS_DIR/lib.sh"

RESTART_PLASMA=true

usage() {
    cat <<EOF
Usage: widgets/install.sh [--no-restart]

Installs the Konveyor widgets, their collector service and helper commands.
Run it again at any time to update them.

  --no-restart  Leave Plasma running; the new widgets load on its next start
EOF
}

for argument in "$@"; do
    case "$argument" in
    --no-restart) RESTART_PLASMA=false ;;
    -h | --help) usage; exit 0 ;;
    *) die "unknown option: $argument" ;;
    esac
done

[[ $EUID -ne 0 ]] || die "run the widget installer as your normal user"
[[ -e $SHARED_DIR/FileWatcher.qml ]] || die "widgets/shared/common is empty; run: git submodule update --init --recursive"

check_commands() {
    local missing=()
    for command_name in python3 kpackagetool6 systemctl journalctl busctl gdbus kscreen-doctor jq ssh curl; do
        command -v "$command_name" >/dev/null || missing+=("$command_name")
    done
    if ((${#missing[@]})); then
        die "missing commands for the widgets: ${missing[*]} (run ./install.sh without --skip-deps, or install them)"
    fi
    python3 -c "import pynvml" >/dev/null 2>&1 || say "Note: install python-nvidia-ml-py for NVIDIA GPU stats in System and Process Monitor"
    id -nG | grep -qwE 'systemd-journal|wheel|adm' \
        || say "Note: add yourself to systemd-journal to see every log in System Log: sudo usermod -aG systemd-journal $USER"
}

link_commands() {
    say "Linking widget commands into $BIN_DIR"
    mkdir -p "$BIN_DIR"
    link_command system-monitor/bin/sysmon-collect
    link_command process-monitor/bin/procmon-collect
    link_command router-monitor/bin/routermon-collect
    link_command router-monitor/bin/routermon-ctl
    link_command router-monitor/bin/routermon-config
    link_command router-monitor/bin/routermon-speedtest
    link_command system-log/bin/logmon-collect
    link_command portals/bin/portal-games
    link_command portals/bin/portal-packages
    link_command portals/bin/portal-launcher
    link_command portals/bin/portal-friends
    link_command service/monitor-overlay
    install -m755 "$WIDGETS_DIR/screen-rotate/bin/linux-plasma-screen-rotate" "$BIN_DIR/linux-plasma-screen-rotate"
}

create_configs() {
    seed_config system-monitor "Linux-System-Monitor"
    seed_config process-monitor "Linux-Process-Mon"
    seed_config router-monitor "Linux-Router-Monitor" "set your router host, user and AdGuard Home login there"
    seed_config portals "Plasma-App-Portal" "paste your free Steam Web API key there (https://steamcommunity.com/dev/apikey)"
    chmod 0600 "$CONFIG_HOME/Linux-Router-Monitor/config.json" "$CONFIG_HOME/Plasma-App-Portal/config.json"
}

main() {
    check_commands
    remove_keyboard_toggle
    link_commands
    create_configs
    push_router_collector
    configure_file_access
    install_collector_service
    install_plasmoids
    install_games_shortcut
    "$WIDGETS_DIR/desktop-hider/desktop-containment" install
    if $RESTART_PLASMA; then
        take_over_launcher_and_restart
    else
        python3 "$WIDGETS_DIR/service/overlay-hosts" install "$CONFIG_HOME/plasma-org.kde.plasma.desktop-appletsrc"
        say "Plasma was left running; the widgets load on its next start"
    fi
    say "Konveyor widgets are installed. Add them from Add Widgets; configs live in ~/.config"
}

main
