#!/usr/bin/env bash
set -euo pipefail

WIDGETS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$WIDGETS_DIR/../extras/packaging/common.sh"
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
    for command_name in python3 kpackagetool6 qdbus6 systemctl journalctl busctl gdbus kscreen-doctor jq ssh curl; do
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
        say "Plasma was left running; the widgets load on its next start"
    fi
    say "Konveyor widgets are installed. Add them from Add Widgets; configs live in ~/.config"
}

main
