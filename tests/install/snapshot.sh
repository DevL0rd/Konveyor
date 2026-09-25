#!/usr/bin/env bash
set -euo pipefail

out="/opt/install-test/snapshots/$1"
mkdir -p "$out"
find /usr /etc -xdev -printf '%p %y\n' | sort >"$out/system"
find /etc -xdev -type f ! -name machine-id -exec sha256sum {} + 2>/dev/null | sort -k2 >"$out/etc"
cd /home/tester
find . -xdev \( -path ./Konveyor -o -path ./.cache \) -prune -o -type f -print0 | xargs -0 sha256sum | sort -k2 >"$out/home-files"
find . -xdev \( -path ./Konveyor -o -path ./.cache \) -prune -o -type d -print | sort >"$out/home-folders"
runuser -u tester -- env XDG_RUNTIME_DIR=/run/user/1000 DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/1000/bus \
    sh -c 'systemctl --user show-environment; systemctl --user list-unit-files --no-legend --state=enabled' | sort >"$out/session"
