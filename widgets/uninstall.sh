#!/usr/bin/env bash
set -euo pipefail

SCRIPT_WIDGETS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WIDGETS_RUNTIME_DIR="${XDG_DATA_HOME:-$HOME/.local/share}/konveyor/widgets"
WIDGETS_DIR="$SCRIPT_WIDGETS_DIR"
[[ -f $WIDGETS_RUNTIME_DIR/lib.sh ]] && WIDGETS_DIR="$WIDGETS_RUNTIME_DIR"

say() {
    printf '\033[1;34m==>\033[0m %s\n' "$*"
}

die() {
    printf '\033[1;31merror:\033[0m %s\n' "$*" >&2
    exit 1
}

source "$WIDGETS_DIR/lib.sh"

PLASMOIDS=(
    org.devl0rd.sysmon org.devl0rd.sysmon.panel org.devl0rd.sysmon.overlay
    org.devl0rd.procmon org.devl0rd.procmon.panel org.devl0rd.procmon.overlay
    org.devl0rd.routermon.system org.devl0rd.routermon.network org.devl0rd.routermon.wifi org.devl0rd.routermon.dns
    org.devl0rd.routermon.clients org.devl0rd.routermon.speedtest org.devl0rd.routermon.panel org.devl0rd.routermon.overlay
    org.devl0rd.logmon.journal
    org.devl0rd.portal org.devl0rd.portal.friends org.devl0rd.portal.launcher
    dev.devl0rd.screenrotate
)
COMMANDS=(sysmon-collect procmon-collect routermon-collect routermon-ctl routermon-config routermon-speedtest logmon-collect
    portal-games portal-packages portal-launcher portal-friends monitor-overlay linux-plasma-screen-rotate)
RUNTIME_DIRS=(Linux-System-Monitor Linux-Process-Mon Linux-Router-Monitor Linux-Log-Monitor Plasma-App-Portal Konveyor-Monitor-Overlay)

[[ $EUID -ne 0 ]] || die "run the widget uninstaller as your normal user"

remove_router_collector() {
    [[ -f $CONFIG_HOME/Linux-Router-Monitor/config.json ]] || return 0
    local host user key remote
    host=$(read_router_config host)
    user=$(read_router_config user)
    key=$(read_router_config ssh_key)
    remote=$(read_router_config remote_script /jffs/lrm-collect.sh)
    key="${key/#\~/$HOME}"
    [[ -n $host && -n $key ]] || return 0
    ssh -o BatchMode=yes -o ConnectTimeout=6 -i "$key" "$user@$host" "rm -f $remote" 2>/dev/null \
        && say "Removed the Router Monitor collector from $host" || true
}

remove_games_shortcut() {
    busctl --user call org.kde.kglobalaccel /kglobalaccel org.kde.KGlobalAccel unregister ss "$GAMES_DESKTOP_ID" _launch >/dev/null 2>&1 || true
    rm -f "${XDG_DATA_HOME:-$HOME/.local/share}/applications/$GAMES_DESKTOP_ID"
    kbuildsycoca6 >/dev/null 2>&1 || true
}

main() {
    say "Stopping $SERVICE"
    systemctl --user disable --now "$SERVICE" >/dev/null 2>&1 || true
    rm -f "$USER_UNITS/$SERVICE"
    systemctl --user daemon-reload

    remove_router_collector
    remove_games_shortcut

    say "Removing widget commands and snapshots"
    local item
    for item in "${COMMANDS[@]}"; do
        rm -f "$BIN_DIR/$item"
    done
    for item in "${RUNTIME_DIRS[@]}"; do
        rm -rf "${XDG_RUNTIME_DIR:-/tmp}/$item"
    done
    rm -f "$PLASMA_OVERRIDE_DIR/konveyor-widgets.conf" "$CONFIG_HOME/environment.d/konveyor-widgets.conf"
    [[ -d $PLASMA_OVERRIDE_DIR ]] && rmdir --ignore-fail-on-non-empty "$PLASMA_OVERRIDE_DIR"

    say "Restoring the application launcher and removing the widgets"
    systemctl --user stop "$PLASMA_SERVICE" 2>/dev/null || true
    python3 "$WIDGETS_DIR/service/overlay-hosts" uninstall "$CONFIG_HOME/plasma-org.kde.plasma.desktop-appletsrc"
    python3 "$WIDGETS_DIR/service/panel-launcher" uninstall "$CONFIG_HOME/plasma-org.kde.plasma.desktop-appletsrc"
    rm -f "$LAUNCHER_SET_UP"
    "$WIDGETS_DIR/desktop-hider/desktop-containment" uninstall
    for item in "${PLASMOIDS[@]}"; do
        kpackagetool6 -t Plasma/Applet -r "$item" >/dev/null 2>&1 && say "  removed $item" || true
    done
    systemctl --user reset-failed "$PLASMA_SERVICE" 2>/dev/null || true
    systemctl --user start "$PLASMA_SERVICE" 2>/dev/null || true

    rm -rf "$WIDGETS_RUNTIME_DIR"
    systemctl --user unset-environment QML_XHR_ALLOW_FILE_READ
    for item in "$(dirname "$WIDGETS_RUNTIME_DIR")" "$BIN_DIR" "${XDG_DATA_HOME:-$HOME/.local/share}/applications" "${XDG_DATA_HOME:-$HOME/.local/share}/plasma/plasmoids" "${XDG_DATA_HOME:-$HOME/.local/share}/plasma"; do
        [[ -d $item ]] && rmdir --ignore-fail-on-non-empty "$item"
    done
    say "Widget configs in ~/.config (router and Steam credentials) were kept"
}

main
