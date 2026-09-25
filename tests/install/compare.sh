#!/usr/bin/env bash
set -uo pipefail

cd /opt/install-test/snapshots
KEPT='\./\.config/(konveyor|Linux-System-Monitor|Linux-Process-Mon|Linux-Router-Monitor|Plasma-App-Portal)(/|$)'
NOISE='\./\.local/state/(UserFeedback\.|kglobalshortcutsstaterc)|\./\.local/share/flatpak(/|$)'
failed=0
for snapshot in system etc home-files home-folders session; do
    changes=$(diff before/$snapshot after/$snapshot | sed -n 's/^\([<>]\) /\1 /p' | grep -vE "^[<>] ([0-9a-f]{64}  )?($KEPT|$NOISE)" || true)
    if [[ -n $changes ]]; then
        printf 'Uninstalling changed %s:\n%s\n' "$snapshot" "$changes"
        failed=1
    fi
done
((failed == 0)) && echo "Uninstalling left the system as it was"
exit "$failed"
