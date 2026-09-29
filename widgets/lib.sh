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
KONTROL_PANEL_DIR="$WIDGETS_DIR/portals/kontrol-panel"
KONTROL_PANEL_UNIT="konveyor-kontrol-panel.service"
KONTROL_PANEL_BUS="org.devl0rd.KontrolPanel"
KONTROL_PANEL_DBUS_DIR="${XDG_DATA_HOME:-$HOME/.local/share}/dbus-1/services"
KONTROL_PANEL_CONFIG="$CONFIG_HOME/konveyor/kontrolpanelrc"
KONTROL_PANEL_DESKTOP="${XDG_DATA_HOME:-$HOME/.local/share}/applications/$KONTROL_PANEL_BUS.desktop"
PLASMA_LAUNCHER_ACTION=(plasmashell "activate application launcher" plasmashell "Activate Application Launcher")
KONTROL_PANEL_KEYS=(16777250 150994992)
LAUNCHER_KEYS_STATE="${XDG_STATE_HOME:-$HOME/.local/state}/konveyor/launcher-keys"

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

stage_lib_into() {
    local lib="$1"
    shift
    rm -rf "${lib:?}"
    mkdir -p "$lib"
    cp "$SHARED_DIR/"*.qml "$SHARED_DIR/"*.js "$lib/"
    cp "$WIDGETS_DIR/shared/MonitorOverlay.qml" "$lib/"
    local extra
    for extra in "$@"; do
        cp -r "$extra/." "$lib/"
    done
}

stage_lib() {
    local plasmoid="$1"
    shift
    stage_lib_into "$plasmoid/contents/ui/lib" "$@"
}

stage_kontrol_panel() {
    cp -r "$WIDGETS_DIR/portals/shared/launcher/." "$KONTROL_PANEL_DIR/"
    stage_lib_into "$KONTROL_PANEL_DIR/lib" "$WIDGETS_DIR/portals/shared/lib"
}

install_kontrol_panel_service() {
    local binary
    binary=$(PATH="$HOME/.local/bin:$PATH" command -v konveyor-kontrol-panel) \
        || die "konveyor-kontrol-panel is missing; install Konveyor with ./install.sh first"
    say "Installing the Kontrol Panel service"
    stage_kontrol_panel
    mkdir -p "$USER_UNITS" "$KONTROL_PANEL_DBUS_DIR"
    cat >"$USER_UNITS/$KONTROL_PANEL_UNIT" <<EOF
[Unit]
Description=Konveyor Kontrol Panel
PartOf=graphical-session.target
After=graphical-session.target

[Service]
Type=dbus
BusName=$KONTROL_PANEL_BUS
ExecStart=$binary $KONTROL_PANEL_DIR
Environment=QML_XHR_ALLOW_FILE_READ=1
Restart=on-failure
RestartSec=2

[Install]
WantedBy=graphical-session.target
EOF
    cat >"$KONTROL_PANEL_DBUS_DIR/$KONTROL_PANEL_BUS.service" <<EOF
[D-BUS Service]
Name=$KONTROL_PANEL_BUS
Exec=$binary $KONTROL_PANEL_DIR
SystemdService=$KONTROL_PANEL_UNIT
EOF
    mkdir -p "$(dirname "$KONTROL_PANEL_DESKTOP")"
    cat >"$KONTROL_PANEL_DESKTOP" <<EOF
[Desktop Entry]
Type=Application
Name=Kontrol Panel
Comment=Apps, games, files, friends and system actions
Exec=$BIN_DIR/portal-launcher toggle
Icon=start-here-kde-plasma-symbolic
NoDisplay=true
StartupNotify=false
EOF
    kbuildsycoca6 >/dev/null 2>&1 || true
    busctl --user call org.freedesktop.DBus /org/freedesktop/DBus org.freedesktop.DBus ReloadConfig >/dev/null
    systemctl --user daemon-reload
    systemctl --user enable "$KONTROL_PANEL_UNIT" >/dev/null 2>&1
    systemctl --user restart "$KONTROL_PANEL_UNIT"
}

remove_kontrol_panel_service() {
    systemctl --user disable --now "$KONTROL_PANEL_UNIT" >/dev/null 2>&1 || true
    rm -f "${USER_UNITS:?}/${KONTROL_PANEL_UNIT:?}" "${KONTROL_PANEL_DBUS_DIR:?}/${KONTROL_PANEL_BUS:?}.service" "${KONTROL_PANEL_DESKTOP:?}"
    rmdir --ignore-fail-on-non-empty "$KONTROL_PANEL_DBUS_DIR" "$(dirname "$KONTROL_PANEL_DBUS_DIR")" 2>/dev/null || true
    kbuildsycoca6 >/dev/null 2>&1 || true
    busctl --user call org.freedesktop.DBus /org/freedesktop/DBus org.freedesktop.DBus ReloadConfig >/dev/null 2>&1 || true
    systemctl --user daemon-reload
}

kglobalaccel() {
    busctl --user call org.kde.kglobalaccel /kglobalaccel org.kde.KGlobalAccel "$@"
}

take_launcher_keys() {
    [[ -e $LAUNCHER_KEYS_STATE ]] && return 0
    local reply codes kept=() code
    reply=$(kglobalaccel shortcut as 4 "${PLASMA_LAUNCHER_ACTION[@]}")
    mkdir -p "$(dirname "$LAUNCHER_KEYS_STATE")"
    printf '%s\n' "$reply" >"$LAUNCHER_KEYS_STATE"
    read -r -a codes <<<"${reply#ai }"
    for code in "${codes[@]:1}"; do
        [[ " ${KONTROL_PANEL_KEYS[*]} " == *" $code "* ]] || kept+=("$code")
    done
    kglobalaccel setForeignShortcut asai 4 "${PLASMA_LAUNCHER_ACTION[@]}" "${#kept[@]}" "${kept[@]}" >/dev/null
    say "Meta and Alt+F1 open the Kontrol Panel"
}

restore_launcher_keys() {
    kglobalaccel unregister ss konveyor-kontrol-panel toggle >/dev/null 2>&1 || true
    [[ -f $LAUNCHER_KEYS_STATE ]] || return 0
    local codes
    read -r -a codes <<<"$(sed 's/^ai //' "$LAUNCHER_KEYS_STATE")"
    kglobalaccel setForeignShortcut asai 4 "${PLASMA_LAUNCHER_ACTION[@]}" "${codes[@]}" >/dev/null 2>&1 || true
    rm -f "${LAUNCHER_KEYS_STATE:?}"
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
        case "$(basename "$plasmoid")" in
        org.devl0rd.portal)
            stage_lib "$plasmoid" "$root/portals/shared/lib"
            rm -rf "$plasmoid/contents/ui/pages" "$plasmoid/contents/ui/shortcuts" "$plasmoid/contents/ui/settings"
            cp -r "$root/portals/shared/launcher/." "$plasmoid/contents/ui/"
            ;;
        org.devl0rd.portal.launcher)
            find "$plasmoid/contents/ui" -mindepth 1 -maxdepth 1 ! -name main.qml ! -name configButton.qml -exec rm -rf {} +
            ;;
        *)
            stage_lib "$plasmoid" "$root/portals/shared/lib"
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
        python3 "$WIDGETS_DIR/service/panel-launcher" install "$CONFIG_HOME/plasma-org.kde.plasma.desktop-appletsrc"
        mkdir -p "$(dirname "$LAUNCHER_SET_UP")"
        touch "$LAUNCHER_SET_UP"
        kwriteconfig6 --file "$KONTROL_PANEL_CONFIG" --group General --key openPageOnStart shortcuts
        systemctl --user restart "$KONTROL_PANEL_UNIT"
    fi
    systemctl --user reset-failed "$PLASMA_SERVICE" 2>/dev/null || true
    systemctl --user start "$PLASMA_SERVICE"
}
