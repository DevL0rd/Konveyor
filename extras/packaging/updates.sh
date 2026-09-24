#!/usr/bin/env bash

KONVEYOR_UPDATER_DIR="/usr/lib/konveyor"
KONVEYOR_USER_UPDATER_DIR="$HOME/.local/lib/konveyor"
KONVEYOR_LOGIN_UNIT="konveyor-login-update.service"
declare -A KONVEYOR_UPDATE_HOOKS=(
    [pacman]="konveyor-rebuild.hook /usr/share/libalpm/hooks/konveyor-rebuild.hook 644"
    [dnf]="konveyor-rebuild.actions /etc/dnf/libdnf5-plugins/actions.d/konveyor-rebuild.actions 644"
    [zypper]="konveyor-rebuild.zypp /usr/lib/zypp/plugins/commit/konveyor-rebuild 755"
    [apt-get]="konveyor-rebuild.apt /etc/apt/apt.conf.d/99konveyor-rebuild 644"
)
KONVEYOR_LEGACY_HOOK="/etc/pacman.d/hooks/konveyor-rebuild.hook"

package_manager() {
    local manager
    for manager in pacman dnf zypper apt-get; do
        if command -v "$manager" >/dev/null; then
            printf '%s\n' "$manager"
            return 0
        fi
    done
    return 1
}

record_fingerprint() {
    system_fingerprint | run_prefix tee "$KONVEYOR_BUILT_FOR" >/dev/null
}

copy_updater() {
    local directory="$1"
    shift
    "$@" install -Dm755 "$SOURCE_DIR/install.sh" "$directory/install"
    "$@" install -Dm644 "$SOURCE_DIR/extras/packaging/common.sh" "$directory/common.sh"
    "$@" install -Dm644 "$SOURCE_DIR/extras/packaging/updates.sh" "$directory/updates.sh"
    "$@" install -Dm755 "$SOURCE_DIR/extras/packaging/konveyor-rebuild" "$directory/konveyor-rebuild"
}

enable_user_unit() {
    local units="${XDG_CONFIG_HOME:-$HOME/.config}/systemd/user"
    mkdir -p "$units"
    sed "s|@SOURCE_DIR@|$SOURCE_DIR|g" "$SOURCE_DIR/extras/packaging/$1.in" >"$units/$1"
    systemctl --user daemon-reload
    systemctl --user enable "$1" >/dev/null 2>&1
}

disable_user_unit() {
    local unit="${XDG_CONFIG_HOME:-$HOME/.config}/systemd/user/$1"
    [[ -e $unit ]] || return 0
    systemctl --user disable "$1" >/dev/null 2>&1 || true
    rm -f "$unit"
}

register_updates() {
    if $AUR || ! as_owner git -C "$SOURCE_DIR" rev-parse --git-dir >/dev/null 2>&1; then
        unregister_updates
        return 0
    fi
    say "Registering Konveyor with system updates"
    printf '%s\n%s\n' "$SOURCE_DIR" "${KONVEYOR_OWNER:-$(id -un)}" | run_prefix tee "$KONVEYOR_STATE_DIR/source" >/dev/null
    if $KONVEYOR_ATOMIC; then
        copy_updater "$KONVEYOR_USER_UPDATER_DIR"
        enable_user_unit "$KONVEYOR_LOGIN_UNIT"
        return 0
    fi
    local manager source target mode
    manager=$(package_manager) || die "no supported package manager to hook Konveyor's updates into"
    read -r source target mode <<<"${KONVEYOR_UPDATE_HOOKS[$manager]}"
    copy_updater "$KONVEYOR_UPDATER_DIR" run_root
    run_root install -Dm"$mode" "$SOURCE_DIR/extras/packaging/$source" "$target"
    if [[ $manager == pacman ]] && grep -qa 'NetworkAccess' /usr/lib/libalpm.so.*; then
        run_root sed -i '/^Exec = /a NetworkAccess = allowed' "$target"
    fi
    [[ -e $KONVEYOR_LEGACY_HOOK ]] && run_root rm -f "$KONVEYOR_LEGACY_HOOK"
    $SYSTEM_UPDATE_ROOT && return 0
    enable_user_unit "$KONVEYOR_UPDATE_UNIT"
}

unregister_updates() {
    local entry source target mode
    for entry in "${KONVEYOR_UPDATE_HOOKS[@]}" "- $KONVEYOR_LEGACY_HOOK -"; do
        read -r source target mode <<<"$entry"
        [[ -e $target ]] && run_root rm -f "$target"
    done
    [[ -d $KONVEYOR_UPDATER_DIR ]] && run_root rm -rf "$KONVEYOR_UPDATER_DIR"
    [[ -e $KONVEYOR_STATE_DIR/source ]] && run_prefix rm -f "$KONVEYOR_STATE_DIR/source"
    rm -rf "$KONVEYOR_USER_UPDATER_DIR"
    disable_user_unit "$KONVEYOR_UPDATE_UNIT"
    disable_user_unit "$KONVEYOR_LOGIN_UNIT"
    return 0
}

source_git() {
    as_owner env "${KONVEYOR_GIT_ENV[@]}" git -C "$SOURCE_DIR" "$@"
}

pull_checkout() {
    source_git rev-parse --abbrev-ref '@{upstream}' >/dev/null 2>&1 || return 1
    if ! source_git fetch --quiet; then
        say "Could not fetch updates for $SOURCE_DIR"
        return 1
    fi
    [[ $(source_git rev-list --count 'HEAD..@{upstream}') -gt 0 ]] || return 1
    [[ -z $(source_git status --porcelain --untracked-files=no) ]] || return 1
    [[ $(source_git rev-list --count '@{upstream}..HEAD') -eq 0 ]] || return 1
    source_git merge --ff-only --quiet '@{upstream}' && source_git submodule update --init --recursive --quiet
}

login_update() {
    if ! pull_checkout && [[ $(system_fingerprint) == "$(cat "$KONVEYOR_BUILT_FOR" 2>/dev/null)" ]]; then
        return 0
    fi
    grep -qx "widgets=false" "$OPTIONS_FILE" 2>/dev/null && WIDGETS=false
    notify_owner "Updating Konveyor" "The system changed, so Konveyor is rebuilding for it. This takes a few minutes."
    install_dependencies
    build
    install_files
    install_versioned_plugin
    record_fingerprint
    register_updates
    remove_previous_plugin_files
    finish_update
}
