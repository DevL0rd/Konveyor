#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 DevL0rd <dmhzmxn@gmail.com>
# SPDX-License-Identifier: GPL-3.0-or-later

set -euo pipefail

readonly SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
readonly DATA_HOME="${XDG_DATA_HOME:-${HOME}/.local/share}"
readonly PLASMOID_ID="dev.devl0rd.screenrotate"
readonly TARGET="${DATA_HOME}/plasma/plasmoids/${PLASMOID_ID}"

die() {
    printf 'Error: %s\n' "$*" >&2
    exit 1
}

for command_name in kscreen-doctor jq kpackagetool6; do
    command -v "${command_name}" >/dev/null || die "Required command not found: ${command_name}"
done

[[ -d "${SCRIPT_DIR}/plasmoid" ]] || die "Plasmoid source was not found at ${SCRIPT_DIR}/plasmoid"
[[ ! -L "${TARGET}" ]] || die "Refusing to overwrite symbolic link ${TARGET}"

case "${TARGET}" in
    */plasma/plasmoids/"${PLASMOID_ID}") ;;
    *) die "Refusing unexpected install target: ${TARGET}" ;;
esac

if [[ -d "${TARGET}" ]]; then
    kpackagetool6 --type Plasma/Applet --upgrade "${SCRIPT_DIR}/plasmoid" >/dev/null
    printf 'Updated %s\n' "${PLASMOID_ID}"
else
    kpackagetool6 --type Plasma/Applet --install "${SCRIPT_DIR}/plasmoid" >/dev/null
    printf 'Installed %s\n' "${PLASMOID_ID}"
fi

printf 'Add "Screen Rotate" to a panel from the widget list.\n'
