#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 DevL0rd <dmhzmxn@gmail.com>
# SPDX-License-Identifier: GPL-3.0-or-later

set -euo pipefail

readonly DATA_HOME="${XDG_DATA_HOME:-${HOME}/.local/share}"
readonly PLASMOID_ID="dev.devl0rd.keyboardtoggle"
readonly TARGET="${DATA_HOME}/plasma/plasmoids/${PLASMOID_ID}"
readonly HELPER_TARGET="${HOME}/.local/bin/linux-plasma-keyboard-toggle"

die() {
    printf 'Error: %s\n' "$*" >&2
    exit 1
}

command -v kpackagetool6 >/dev/null || die "Required command not found: kpackagetool6"

if [[ ! -e "${TARGET}" ]]; then
    printf '%s is already uninstalled.\n' "${PLASMOID_ID}"
    exit 0
fi

case "${TARGET}" in
    */plasma/plasmoids/"${PLASMOID_ID}") ;;
    *) die "Refusing unexpected uninstall target: ${TARGET}" ;;
esac

if [[ -f "${HELPER_TARGET}" && ! -L "${HELPER_TARGET}" ]]; then
    rm -f -- "${HELPER_TARGET}"
    printf 'Removed helper %s\n' "${HELPER_TARGET}"
fi

kpackagetool6 --type Plasma/Applet --remove "${PLASMOID_ID}" >/dev/null
printf 'Removed %s\n' "${PLASMOID_ID}"
printf 'Remove the widget from your panel if it is still shown.\n'
