#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 DevL0rd <dmhzmxn@gmail.com>
# SPDX-License-Identifier: GPL-3.0-or-later

set -euo pipefail

readonly DATA_HOME="${XDG_DATA_HOME:-${HOME}/.local/share}"
readonly CONTAINMENT_TARGET="${DATA_HOME}/plasma/plasmoids/org.kde.desktopcontainment"
readonly KWIN_TARGET="${DATA_HOME}/kwin/scripts/devl0rd-hide-desktop-widgets"
readonly MANAGED_MARKER=".linux-widget-hider-managed"

case "${CONTAINMENT_TARGET}" in
    */plasma/plasmoids/org.kde.desktopcontainment) ;;
    *) printf 'Error: refusing unexpected containment target: %s\n' "${CONTAINMENT_TARGET}" >&2; exit 1 ;;
esac

case "${KWIN_TARGET}" in
    */kwin/scripts/devl0rd-hide-desktop-widgets) ;;
    *) printf 'Error: refusing unexpected KWin target: %s\n' "${KWIN_TARGET}" >&2; exit 1 ;;
esac

qdbus6 org.kde.plasmashell /PlasmaShell org.kde.PlasmaShell.evaluateScript \
    'for (const desktop of desktops()) { desktop.currentConfigGroup = ["General"]; desktop.writeConfig("hideDesktopWidgets", false); }' \
    >/dev/null 2>&1 || true

qdbus6 org.kde.KWin /Scripting org.kde.kwin.Scripting.unloadScript devl0rd-hide-desktop-widgets \
    >/dev/null 2>&1 || true
kwriteconfig6 --file kwinrc --group Plugins --key devl0rd-hide-desktop-widgetsEnabled --delete ''

if [[ -f "${KWIN_TARGET}/${MANAGED_MARKER}" ]]; then
    rm -rf -- "${KWIN_TARGET}"
elif [[ -e "${KWIN_TARGET}" ]]; then
    printf 'Left unmanaged KWin script untouched: %s\n' "${KWIN_TARGET}" >&2
fi

if [[ -f "${CONTAINMENT_TARGET}/${MANAGED_MARKER}" ]]; then
    rm -rf -- "${CONTAINMENT_TARGET}"
elif [[ -e "${CONTAINMENT_TARGET}" ]]; then
    printf 'Left unmanaged desktop containment untouched: %s\n' "${CONTAINMENT_TARGET}" >&2
fi

systemctl --user restart plasma-plasmashell.service

printf 'Linux Widget Hider uninstalled successfully.\n'
