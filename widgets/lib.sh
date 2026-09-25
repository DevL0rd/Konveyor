#!/usr/bin/env bash

BIN_DIR="$HOME/.local/bin"
CONFIG_HOME="${XDG_CONFIG_HOME:-$HOME/.config}"
USER_UNITS="$CONFIG_HOME/systemd/user"
SHARED_DIR="$WIDGETS_DIR/shared/common"
PLASMA_SERVICE="plasma-plasmashell.service"
PLASMA_OVERRIDE_DIR="$USER_UNITS/$PLASMA_SERVICE.d"
SERVICE="konveyor-widgets.service"
LEGACY_NAMES=(linux-system-monitor linux-process-mon linux-router-monitor linux-log-monitor linux-plasma-portals)
LEGACY_SERVICES=(linux-system-monitor.service linux-process-mon.service linux-router-monitor.service linux-log-monitor.service portal-friends.service)
GAMES_DESKTOP_ID="org.devl0rd.portal.launcher.games.desktop"
LAUNCHER_SET_UP="${XDG_STATE_HOME:-$HOME/.local/state}/konveyor/launcher-set-up"

link_command() {
    chmod +x "$WIDGETS_DIR/$1"
    ln -sfn "$WIDGETS_DIR/$1" "$BIN_DIR/$(basename "$1")"
}

seed_config() {
    local directory="$CONFIG_HOME/$2"
    mkdir -p "$directory"
    [[ -f $directory/config.json ]] && return 0
    cp "$WIDGETS_DIR/$1/config.example.json" "$directory/config.json"
    say "Created $directory/config.json${3:+ — $3}"
}

read_router_config() {
    python3 -c "import json,sys; print(json.load(open(sys.argv[1])).get(sys.argv[2], sys.argv[3]))" \
        "$CONFIG_HOME/Linux-Router-Monitor/config.json" "$1" "${2:-}"
}

push_router_collector() {
    local host user key remote
    host=$(read_router_config host)
    user=$(read_router_config user)
    key=$(read_router_config ssh_key)
    remote=$(read_router_config remote_script /jffs/lrm-collect.sh)
    key="${key/#\~/$HOME}"
    [[ -n $host && -n $key ]] || return 0
    say "Pushing the Router Monitor collector to $user@$host:$remote"
    if ! ssh -o BatchMode=yes -o ConnectTimeout=8 -o StrictHostKeyChecking=accept-new -i "$key" "$user@$host" \
        "cat > $remote && chmod +x $remote" <"$WIDGETS_DIR/router-monitor/router/collect.sh"; then
        say "Could not reach the router; fix SSH in $CONFIG_HOME/Linux-Router-Monitor/config.json and run the installer again"
    fi
}

configure_file_access() {
    say "Letting Plasma widgets read their collector snapshots"
    [[ ! -L $PLASMA_OVERRIDE_DIR/konveyor-widgets.conf ]] || die "refusing to overwrite symbolic link $PLASMA_OVERRIDE_DIR/konveyor-widgets.conf"
    mkdir -p "$PLASMA_OVERRIDE_DIR" "$CONFIG_HOME/environment.d"
    printf '[Service]\nEnvironment=QML_XHR_ALLOW_FILE_READ=1\n' >"$PLASMA_OVERRIDE_DIR/konveyor-widgets.conf"
    printf 'QML_XHR_ALLOW_FILE_READ=1\n' >"$CONFIG_HOME/environment.d/konveyor-widgets.conf"
    local name
    for name in "${LEGACY_NAMES[@]}"; do
        rm -f "$PLASMA_OVERRIDE_DIR/$name.conf" "$CONFIG_HOME/environment.d/$name.conf" "$CONFIG_HOME/plasma-workspace/env/$name.sh"
    done
    systemctl --user set-environment QML_XHR_ALLOW_FILE_READ=1
}

install_collector_service() {
    say "Installing $SERVICE (every widget collector in one process)"
    local unit
    for unit in "${LEGACY_SERVICES[@]}"; do
        if [[ -f $USER_UNITS/$unit ]]; then
            systemctl --user disable --now "$unit" >/dev/null 2>&1 || true
            rm -f "$USER_UNITS/$unit"
        fi
    done
    local affinity
    affinity=$(python3 -S "$WIDGETS_DIR/service/konveyor-widgets-ecores" 2>/dev/null || true)
    mkdir -p "$USER_UNITS"
    cat >"$USER_UNITS/$SERVICE" <<EOF
[Unit]
Description=Konveyor widget collectors
After=graphical-session.target

[Service]
Type=simple
ExecStart=/usr/bin/python3 $WIDGETS_DIR/service/konveyor-widgets
Restart=always
RestartSec=3
Nice=19
${affinity:+CPUAffinity=$affinity}

[Install]
WantedBy=default.target
EOF
    systemctl --user daemon-reload
    systemctl --user enable "$SERVICE" >/dev/null 2>&1
    systemctl --user restart "$SERVICE"
}

stage_lib() {
    local plasmoid="$1"
    shift
    rm -rf "$plasmoid/contents/ui/lib"
    mkdir -p "$plasmoid/contents/ui/lib"
    cp "$SHARED_DIR/"*.qml "$SHARED_DIR/"*.js "$plasmoid/contents/ui/lib/"
    cp "$WIDGETS_DIR/shared/MonitorOverlay.qml" "$plasmoid/contents/ui/lib/"
    local extra
    for extra in "$@"; do
        cp -r "$extra/." "$plasmoid/contents/ui/lib/"
    done
}

install_plasmoid() {
    local id
    id=$(python3 -c "import json,sys; print(json.load(open(sys.argv[1]))['KPlugin']['Id'])" "$1/metadata.json")
    if kpackagetool6 -t Plasma/Applet -u "$1" >/dev/null 2>&1; then
        say "  upgraded $id"
    else
        kpackagetool6 -t Plasma/Applet -i "$1" >/dev/null
        say "  installed $id"
    fi
}

copy_panel_ui() {
    local source="$1" target="$2"
    shift 2
    mkdir -p "$target/contents/ui" "$target/contents/config"
    cp "$source/contents/ui/"*.qml "$target/contents/ui/"
    local pattern
    for pattern in "$@"; do
        cp -r "$source/contents/ui/"$pattern "$target/contents/ui/"
    done
    cp "$source/contents/config/main.xml" "$source/contents/config/config.qml" "$target/contents/config/"
}

install_plasmoids() {
    say "Installing the widgets"
    local root="$WIDGETS_DIR" plasmoid
    for plasmoid in "$root"/system-monitor/plasmoids/org.devl0rd.sysmon*; do
        [[ $plasmoid == */org.devl0rd.sysmon.panel ]] || copy_panel_ui "$root/system-monitor/plasmoids/org.devl0rd.sysmon.panel" "$plasmoid"
        stage_lib "$plasmoid"
        install_plasmoid "$plasmoid"
    done
    for plasmoid in "$root"/process-monitor/plasmoids/org.devl0rd.procmon*; do
        [[ $plasmoid == */org.devl0rd.procmon.panel ]] || copy_panel_ui "$root/process-monitor/plasmoids/org.devl0rd.procmon.panel" "$plasmoid" "*.mjs"
        stage_lib "$plasmoid"
        install_plasmoid "$plasmoid"
    done
    for plasmoid in "$root"/router-monitor/plasmoids/org.devl0rd.routermon.*; do
        if [[ $plasmoid != */org.devl0rd.routermon.panel ]]; then
            rm -rf "$plasmoid/contents/ui/tabs"
            copy_panel_ui "$root/router-monitor/plasmoids/org.devl0rd.routermon.panel" "$plasmoid" tabs
        fi
        stage_lib "$plasmoid" "$root/router-monitor/shared/lib"
        install_plasmoid "$plasmoid"
    done
    for plasmoid in "$root"/system-log/plasmoids/org.devl0rd.logmon.*; do
        stage_lib "$plasmoid" "$root/system-log/shared/lib"
        install_plasmoid "$plasmoid"
    done
    for plasmoid in "$root"/portals/plasmoids/org.devl0rd.portal*; do
        stage_lib "$plasmoid" "$root/portals/shared/lib"
        case "$(basename "$plasmoid")" in
        org.devl0rd.portal | org.devl0rd.portal.launcher)
            rm -rf "$plasmoid/contents/ui/pages" "$plasmoid/contents/ui/shortcuts" "$plasmoid/contents/ui/settings"
            cp -r "$root/portals/shared/launcher/." "$plasmoid/contents/ui/"
            ;;
        esac
        install_plasmoid "$plasmoid"
    done
    install_plasmoid "$root/screen-rotate/plasmoid"
}

install_games_shortcut() {
    local apps="${XDG_DATA_HOME:-$HOME/.local/share}/applications"
    mkdir -p "$apps"
    cat >"$apps/$GAMES_DESKTOP_ID" <<DESKTOP
[Desktop Entry]
Type=Application
Name=Kontrol Panel: Games
Comment=Open the Kontrol Panel on its Games page
Exec=$BIN_DIR/portal-launcher games
Icon=input-gamepad-symbolic
NoDisplay=true
StartupNotify=false
X-KDE-Shortcuts=Meta+G
DESKTOP
    kbuildsycoca6 >/dev/null 2>&1 || true
    local meta_g=268435527 grid_keys
    grid_keys=$(busctl --user call org.kde.kglobalaccel /kglobalaccel org.kde.KGlobalAccel shortcut as 4 kwin "Grid View" KWin "Toggle Grid View" 2>/dev/null || true)
    if [[ " $grid_keys " == *" $meta_g "* ]]; then
        busctl --user call org.kde.kglobalaccel /kglobalaccel org.kde.KGlobalAccel setForeignShortcut asai 4 kwin "Grid View" KWin "Toggle Grid View" 0 >/dev/null
        say "Freed Meta+G from KWin's Grid View"
    fi
    busctl --user call org.kde.kglobalaccel /kglobalaccel org.kde.KGlobalAccel doRegister as 4 "$GAMES_DESKTOP_ID" _launch "Kontrol Panel: Games" "Kontrol Panel: Games" >/dev/null
    busctl --user call org.kde.kglobalaccel /kglobalaccel org.kde.KGlobalAccel setShortcut asaiu 4 "$GAMES_DESKTOP_ID" _launch "Kontrol Panel: Games" "Kontrol Panel: Games" 1 "$meta_g" 2 >/dev/null
    say "Meta+G opens the Kontrol Panel on Games"
}

remove_keyboard_toggle() {
    if kpackagetool6 -t Plasma/Applet -r dev.devl0rd.keyboardtoggle >/dev/null 2>&1; then
        say "Removed the Keyboard Toggle widget; it now lives in KBoard"
    fi
    rm -f "$BIN_DIR/linux-plasma-keyboard-toggle"
    rm -rf "${XDG_STATE_HOME:-$HOME/.local/state}/linux-plasma-keyboard-toggle"
}

take_over_launcher_and_restart() {
    say "Restarting Plasma"
    systemctl --user stop "$PLASMA_SERVICE"
    python3 "$WIDGETS_DIR/service/overlay-hosts" install "$CONFIG_HOME/plasma-org.kde.plasma.desktop-appletsrc"
    if [[ ! -e $LAUNCHER_SET_UP ]]; then
        python3 "$WIDGETS_DIR/service/panel-launcher" install --open-page shortcuts "$CONFIG_HOME/plasma-org.kde.plasma.desktop-appletsrc"
        mkdir -p "$(dirname "$LAUNCHER_SET_UP")"
        touch "$LAUNCHER_SET_UP"
    fi
    systemctl --user reset-failed "$PLASMA_SERVICE" 2>/dev/null || true
    systemctl --user start "$PLASMA_SERVICE"
}
