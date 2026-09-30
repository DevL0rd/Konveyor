#!/usr/bin/env bash
set -uo pipefail

failed=0
check() {
    if eval "$2" >/dev/null 2>&1; then
        echo "ok: $1"
    else
        echo "FAILED: $1"
        failed=1
    fi
}

holds_launcher_keys() {
    local keys
    keys=$(busctl --user call org.kde.kglobalaccel /kglobalaccel org.kde.KGlobalAccel shortcut as 4 "$1" "$2" "$1" "$2")
    [[ " $keys " == *" 16777250 "* && " $keys " == *" 150994992 "* ]]
}

if [[ ${1:-} == uninstalled ]]; then
    check "Meta and Alt+F1 are back on Plasma's application launcher" "holds_launcher_keys plasmashell 'activate application launcher'"
    check "the Kontrol Panel no longer holds Meta and Alt+F1" "! holds_launcher_keys konveyor-kontrol-panel toggle"
    exit "$failed"
fi

plugin=$(cat /usr/share/konveyor/plugin-id)
telemetry=$(cat /usr/share/konveyor/telemetry-plugin-id)
check "KWin loaded $plugin" "gdbus call --session --dest org.kde.KWin --object-path /Effects --method org.kde.kwin.Effects.isEffectLoaded $plugin | grep -q true"
check "KWin loaded $telemetry" "gdbus call --session --dest org.kde.KWin --object-path /Effects --method org.kde.kwin.Effects.isEffectLoaded $telemetry | grep -q true"
check "org.kde.Konveyor answers" "gdbus call --session --dest org.kde.Konveyor --object-path /Konveyor --method org.kde.Konveyor.Windows"
check "the cheatsheet lists shortcuts" "konveyor-cheatsheet --json | python3 -c 'import json, sys; assert json.load(sys.stdin)'"
check "the system update hook is registered" "test -f /usr/share/libalpm/hooks/konveyor-rebuild.hook"
check "konveyor-widgets.service runs" "systemctl --user is-active konveyor-widgets.service"
check "Plasma runs" "systemctl --user is-active plasma-plasmashell.service"
for collector in Linux-System-Monitor Linux-Process-Mon Linux-Log-Monitor; do
    check "$collector writes fresh snapshots" "find $XDG_RUNTIME_DIR/$collector -type f -newermt '-30 seconds' | grep -q ."
done
check "the Kontrol Panel replaced the app menu" "grep -q plugin=org.devl0rd.portal.launcher ~/.config/plasma-org.kde.plasma.desktop-appletsrc"
check "the Kontrol Panel service runs" "systemctl --user is-active konveyor-kontrol-panel.service"
check "the Kontrol Panel answers on the session bus" "busctl --user call org.devl0rd.KontrolPanel /KontrolPanel org.devl0rd.KontrolPanel IsOpen"
check "the Kontrol Panel opened on Shortcuts" "test -f ~/.config/konveyor/kontrolpanelrc && ! grep -q openPageOnStart=shortcuts ~/.config/konveyor/kontrolpanelrc"
check "Meta and Alt+F1 open the Kontrol Panel" "holds_launcher_keys konveyor-kontrol-panel toggle"
check "Plasma's application launcher gave up Meta and Alt+F1" "! holds_launcher_keys plasmashell 'activate application launcher'"
check "System Settings lists the Konveyor module" "kcmshell6 --list | grep -q kcm_konveyor"
errors=$(journalctl --user -u plasma-plasmashell --no-pager -o cat | grep -E 'org/kde/konveyor|org\.devl0rd' | grep -iE 'error|not installed|not a type|unavailable' | sort -u)
check "Plasma logged no Konveyor QML errors" "[[ -z \"\$errors\" ]]"
[[ -n $errors ]] && echo "$errors"
exit "$failed"
