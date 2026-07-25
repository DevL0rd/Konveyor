#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 DevL0rd <dmhzmxn@gmail.com>
# SPDX-License-Identifier: GPL-3.0-or-later

set -euo pipefail

readonly SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
readonly DATA_HOME="${XDG_DATA_HOME:-${HOME}/.local/share}"
readonly SYSTEM_CONTAINMENT="/usr/share/plasma/plasmoids/org.kde.desktopcontainment"
readonly CONTAINMENT_TARGET="${DATA_HOME}/plasma/plasmoids/org.kde.desktopcontainment"
readonly KWIN_TARGET="${DATA_HOME}/kwin/scripts/devl0rd-hide-desktop-widgets"
readonly MANAGED_MARKER=".linux-widget-hider-managed"

die() {
    printf 'Error: %s\n' "$*" >&2
    exit 1
}

for command_name in patch kwriteconfig6 qdbus6 systemctl; do
    command -v "${command_name}" >/dev/null || die "Required command not found: ${command_name}"
done

[[ -d "${SYSTEM_CONTAINMENT}" ]] || die "Plasma desktop containment was not found at ${SYSTEM_CONTAINMENT}"

case "${CONTAINMENT_TARGET}" in
    */plasma/plasmoids/org.kde.desktopcontainment) ;;
    *) die "Refusing unexpected containment target: ${CONTAINMENT_TARGET}" ;;
esac

case "${KWIN_TARGET}" in
    */kwin/scripts/devl0rd-hide-desktop-widgets) ;;
    *) die "Refusing unexpected KWin target: ${KWIN_TARGET}" ;;
esac

if [[ -e "${CONTAINMENT_TARGET}" && ! -f "${CONTAINMENT_TARGET}/${MANAGED_MARKER}" ]]; then
    die "An unmanaged local desktop-containment override already exists at ${CONTAINMENT_TARGET}"
fi

if [[ -e "${KWIN_TARGET}" && ! -f "${KWIN_TARGET}/${MANAGED_MARKER}" ]]; then
    die "An unmanaged KWin script already exists at ${KWIN_TARGET}"
fi

readonly STAGING_ROOT="$(mktemp -d "${TMPDIR:-/tmp}/linux-widget-hider.XXXXXXXX")"
cleanup() {
    rm -rf -- "${STAGING_ROOT}"
}
trap cleanup EXIT

readonly STAGED_CONTAINMENT="${STAGING_ROOT}/org.kde.desktopcontainment"
readonly STAGED_KWIN="${STAGING_ROOT}/devl0rd-hide-desktop-widgets"

cp -a -- "${SYSTEM_CONTAINMENT}" "${STAGED_CONTAINMENT}"
patch --batch --forward -d "${STAGED_CONTAINMENT}" -p1 < "${SCRIPT_DIR}/patches/desktopcontainment.patch"
touch "${STAGED_CONTAINMENT}/${MANAGED_MARKER}"

cp -a -- "${SCRIPT_DIR}/kwin-script" "${STAGED_KWIN}"
touch "${STAGED_KWIN}/${MANAGED_MARKER}"

mkdir -p -- "$(dirname -- "${CONTAINMENT_TARGET}")" "$(dirname -- "${KWIN_TARGET}")"

if [[ -e "${CONTAINMENT_TARGET}" ]]; then
    rm -rf -- "${CONTAINMENT_TARGET}"
fi
if [[ -e "${KWIN_TARGET}" ]]; then
    rm -rf -- "${KWIN_TARGET}"
fi

mv -- "${STAGED_CONTAINMENT}" "${CONTAINMENT_TARGET}"
mv -- "${STAGED_KWIN}" "${KWIN_TARGET}"

kwriteconfig6 --file kwinrc --group Plugins --key devl0rd-hide-desktop-widgetsEnabled --type bool true

qdbus6 org.kde.KWin /Scripting org.kde.kwin.Scripting.unloadScript devl0rd-hide-desktop-widgets >/dev/null 2>&1 || true
qdbus6 org.kde.KWin /Scripting org.kde.kwin.Scripting.loadScript \
    "${KWIN_TARGET}/contents/code/main.js" devl0rd-hide-desktop-widgets >/dev/null
qdbus6 org.kde.KWin /Scripting org.kde.kwin.Scripting.start

systemctl --user restart plasma-plasmashell.service

printf 'Linux Widget Hider installed successfully.\n'
