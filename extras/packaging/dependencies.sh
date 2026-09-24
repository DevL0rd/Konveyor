#!/usr/bin/env bash
set -euo pipefail

source "$(dirname "${BASH_SOURCE[0]}")/common.sh"

ARCH_PACKAGES=(base-devel cmake ninja git extra-cmake-modules qt6-base qt6-declarative kwin kglobalaccel kconfig kcolorscheme knotifications kcoreaddons ki18n libxkbcommon python-dbus kirigami layer-shell-qt kcmutils kirigami-addons kdeclarative kservice kdecoration kwindowsystem vulkan-headers vulkan-icd-loader wayland libepoxy libdrm python jq curl openssh glib2 libkscreen kpackage)
FEDORA_PACKAGES=(cmake ninja-build git gcc-c++ extra-cmake-modules qt6-qtbase-devel qt6-qtbase-private-devel qt6-qtdeclarative-devel qt6-qt5compat kwin-devel kdecoration-devel kf6-kglobalaccel-devel kf6-kconfig-devel kf6-kcolorscheme-devel kf6-knotifications-devel kf6-kcoreaddons-devel kf6-ki18n-devel kf6-kwindowsystem-devel libxkbcommon-devel libepoxy-devel libdrm-devel wayland-devel xcb-util-wm-devel python3-dbus kf6-kirigami layer-shell-qt kf6-kcmutils-devel kf6-kirigami-addons kf6-kdeclarative kf6-kiconthemes kf6-kservice-devel kf6-kpackage libkscreen vulkan-headers vulkan-loader-devel glib2 libdnf5-plugin-actions python3 jq curl openssh-clients)
SUSE_PACKAGES=(cmake ninja git gcc-c++ kf6-extra-cmake-modules qt6-base-devel qt6-base-private-devel qt6-declarative-devel qt6-declarative-imports qt6-qt5compat-imports kwin6-devel kdecoration6-devel kf6-kglobalaccel-devel kf6-kconfig-devel kf6-kcolorscheme-devel kf6-knotifications-devel kf6-kcoreaddons-devel kf6-kcoreaddons-imports kf6-ki18n-devel kf6-kwindowsystem-devel libxkbcommon-devel libepoxy-devel libdrm-devel wayland-devel libxcb-devel xcb-util-wm-devel python3-dbus-python kf6-kirigami-imports layer-shell-qt6-imports kf6-kcmutils-devel kf6-kcmutils-imports kirigami-addons6 kf6-kdeclarative-imports kf6-kiconthemes-imports kf6-kservice-devel kf6-kpackage libkscreen6-plugin vulkan-devel glib2-tools python3 jq curl openssh-clients)
DEBIAN_PACKAGES=(cmake ninja-build git g++ extra-cmake-modules qt6-base-dev qt6-base-private-dev qt6-declarative-dev kwin-dev libkdecorations3-dev libkf6globalaccel-dev libkf6config-dev libkf6config-bin libkf6colorscheme-dev libkf6notifications-dev libkf6coreaddons-dev libkf6i18n-dev libkf6windowsystem-dev libxkbcommon-dev libepoxy-dev libdrm-dev libwayland-dev libxcb-icccm4-dev libxcb-composite0-dev libxcb-randr0-dev libxcb-res0-dev libxcb-shm0-dev libxcb-sync-dev python3-dbus qml6-module-org-kde-kirigami qml6-module-org-kde-layershell libkf6kcmutils-dev qml6-module-org-kde-kcmutils qml6-module-org-kde-kirigamiaddons-formcard qml6-module-org-kde-kirigamiaddons-components qml6-module-org-kde-kquickcontrols qml6-module-org-kde-iconthemes qml6-module-org-kde-coreaddons qml6-module-qt5compat-graphicaleffects qml6-module-qtquick-shapes qml6-module-qtquick-dialogs qml6-module-qt-labs-folderlistmodel libkf6service-dev libkf6service-bin kpackagetool6 libkscreen-bin libvulkan-dev libglib2.0-bin python3 jq curl openssh-client)
TOOLBOX_MATCHED_PACKAGES='^(kwin|kdecoration|qt6-|kf6-)'

matched_packages() {
    "$@" rpm -qa --qf '%{NAME} %{VERSION}-%{RELEASE}\n' | tr -d '\r' | awk -v pattern="$TOOLBOX_MATCHED_PACKAGES" '$1 ~ pattern' | sort -u
}

remove_stale_toolboxes() {
    local name
    for name in $(podman ps --all --format '{{.Names}}' | grep -E '^konveyor-fedora-[0-9]+$' || true); do
        [[ $name == "$KONVEYOR_BUILD_BOX" ]] || toolbox rm --force "$name" >/dev/null
    done
}

prepare_toolbox() {
    local release locks mismatched
    release=$(. /etc/os-release && [[ " $ID ${ID_LIKE:-} " == *" fedora "* ]] && printf '%s' "$VERSION_ID") \
        || die "on image-based systems Konveyor builds in a Fedora toolbox, and this system isn't based on Fedora"
    command -v toolbox >/dev/null || die "toolbox is missing; Fedora Atomic desktops ship it, so install it with: rpm-ostree install toolbox"
    remove_stale_toolboxes
    remove_unused_pulled_images
    if ! podman container exists "$KONVEYOR_BUILD_BOX"; then
        say "Creating the $KONVEYOR_BUILD_BOX toolbox to build Konveyor in"
        remember_pulled_image "registry.fedoraproject.org/fedora-toolbox:$release"
        toolbox --assumeyes create --distro fedora --release "$release" "$KONVEYOR_BUILD_BOX"
    fi
    say "Matching the toolbox to this system's KWin, Qt and KDE Frameworks"
    mapfile -t locks < <(matched_packages | tr ' ' '-')
    "${KONVEYOR_BUILD_ENV[@]}" sudo dnf install --assumeyes --quiet fedora-repos-archive
    "${KONVEYOR_BUILD_ENV[@]}" sudo dnf versionlock clear >/dev/null
    "${KONVEYOR_BUILD_ENV[@]}" sudo dnf versionlock add "${locks[@]}" >/dev/null
    "${KONVEYOR_BUILD_ENV[@]}" sudo dnf install --assumeyes "${FEDORA_PACKAGES[@]}"
    "${KONVEYOR_BUILD_ENV[@]}" sudo dnf distro-sync --assumeyes
    mismatched=$(join <(matched_packages) <(matched_packages "${KONVEYOR_BUILD_ENV[@]}") | awk '$2 != $3 { printf "%s (system %s, toolbox %s) ", $1, $2, $3 }')
    [[ -z $mismatched ]] || die "Fedora's repositories don't carry the exact versions this system runs: ${mismatched}Update the system, reboot and run ./install.sh again"
}

STEAMOS_MIRROR="https://steamdeck-packages.steamos.cloud/archlinux-mirror"

steamos_matched_packages() {
    "$@" bash -c 'pacman -Q | awk '"'"'$1 ~ /^(kwin|kdecoration|qt6-)/'"'"'; pacman -Q $(pacman -Qgq kf6)' | tr -d '\r' | sort -u
}

steamos_pacman_conf() {
    local repo
    printf '[options]\nArchitecture = auto\nSigLevel = %s\nLocalFileSigLevel = Optional\n' "$1"
    for repo in "${@:2}"; do
        printf '[%s]\nServer = %s/$repo/os/$arch\n' "$repo" "$STEAMOS_MIRROR"
    done
}

steamos_package_url() {
    local repo arch
    repo=$("${KONVEYOR_BUILD_ENV[@]}" pacman -Si "$1" | tr -d '\r' | awk -F' *: *' '$1 == "Repository" { print $2; exit }')
    arch=$("${KONVEYOR_BUILD_ENV[@]}" pacman -Si "$1" | tr -d '\r' | awk -F' *: *' '$1 == "Architecture" { print $2; exit }')
    printf '%s/%s/os/x86_64/%s-%s-%s.pkg.tar.zst\n' "$STEAMOS_MIRROR" "$repo" "$1" "$2" "$arch"
}

steamos_mismatches() {
    join <(steamos_matched_packages) <(steamos_matched_packages "${KONVEYOR_BUILD_ENV[@]}") | awk '$2 != $3 { print $1, $2, $3 }'
}

prepare_steamos_box() {
    local repos pins name version rest mismatched
    [[ $(printf '%s\n' 6.7 "$(kwin_version)" | sort -V | head -1) == 6.7 ]] \
        || die "this SteamOS build has KWin $(kwin_version), and Konveyor needs Plasma 6.7, which SteamOS 3.9 brings. Switch to the Preview update channel in Settings > System, update, and run ./install.sh again"
    command -v distrobox >/dev/null || die "distrobox is missing; SteamOS 3.5 and newer ship it"
    mapfile -t repos < <(sed -n 's/^\[\(.*\)\]$/\1/p' /etc/pacman.conf | grep -vx options)
    if ! podman container exists "$KONVEYOR_BUILD_BOX"; then
        say "Creating the $KONVEYOR_BUILD_BOX box to build Konveyor in"
        remember_pulled_image docker.io/library/archlinux:latest
        distrobox create --yes --no-entry --name "$KONVEYOR_BUILD_BOX" --image docker.io/library/archlinux:latest
    fi
    say "Matching the box to this SteamOS build's KWin, Qt and KDE Frameworks"
    steamos_pacman_conf "Required DatabaseOptional" "${repos[@]}" | "${KONVEYOR_BUILD_ENV[@]}" sudo tee /etc/pacman.conf >/dev/null
    steamos_pacman_conf Never "${repos[@]}" | "${KONVEYOR_BUILD_ENV[@]}" sudo tee /etc/pacman-keyring.conf >/dev/null
    "${KONVEYOR_BUILD_ENV[@]}" sudo pacman -Sy --noconfirm --config /etc/pacman-keyring.conf holo-keyring
    "${KONVEYOR_BUILD_ENV[@]}" sudo pacman-key --init
    "${KONVEYOR_BUILD_ENV[@]}" sudo pacman-key --populate holo
    "${KONVEYOR_BUILD_ENV[@]}" sudo pacman -Syuu --noconfirm --ignore filesystem
    "${KONVEYOR_BUILD_ENV[@]}" sudo pacman -S --needed --noconfirm "${ARCH_PACKAGES[@]}"
    pins=()
    while read -r name version rest; do
        pins+=("$(steamos_package_url "$name" "$version")")
    done < <(steamos_mismatches)
    if ((${#pins[@]})); then
        "${KONVEYOR_BUILD_ENV[@]}" sudo pacman -U --noconfirm "${pins[@]}"
    fi
    mismatched=$(steamos_mismatches | awk '{ printf "%s (system %s, box %s) ", $1, $2, $3 }')
    [[ -z $mismatched ]] || die "Valve's repositories don't carry the exact versions this SteamOS build runs: ${mismatched}Update SteamOS and run ./install.sh again"
}

if [[ $KONVEYOR_BUILD_BOX == konveyor-steamos ]]; then
    prepare_steamos_box
elif $KONVEYOR_ATOMIC; then
    prepare_toolbox
elif command -v pacman >/dev/null; then
    run_root pacman -S --needed --noconfirm "${ARCH_PACKAGES[@]}"
elif command -v dnf >/dev/null; then
    run_root dnf install -y "${FEDORA_PACKAGES[@]}"
elif command -v zypper >/dev/null; then
    run_root zypper --non-interactive install "${SUSE_PACKAGES[@]}"
elif command -v apt-get >/dev/null; then
    run_root apt-get install -y "${DEBIAN_PACKAGES[@]}"
else
    die "unsupported package manager: install the KWin, KDE Frameworks 6 and Qt 6 development packages, then run ./install.sh --skip-deps"
fi
