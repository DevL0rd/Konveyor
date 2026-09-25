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
check "the Kontrol Panel opened on Shortcuts" "! grep -q openPageOnStart=shortcuts ~/.config/plasma-org.kde.plasma.desktop-appletsrc"
check "System Settings lists the Konveyor module" "kcmshell6 --list | grep -q kcm_konveyor"
errors=$(journalctl --user -u plasma-plasmashell --no-pager -o cat | grep -E 'org/kde/konveyor|org\.devl0rd' | grep -iE 'error|not installed|not a type|unavailable' | sort -u)
check "Plasma logged no Konveyor QML errors" "[[ -z \"\$errors\" ]]"
[[ -n $errors ]] && echo "$errors"
exit "$failed"
