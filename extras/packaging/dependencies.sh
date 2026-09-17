#!/usr/bin/env bash
set -euo pipefail

source "$(dirname "${BASH_SOURCE[0]}")/common.sh"

ARCH_PACKAGES=(base-devel cmake ninja git extra-cmake-modules qt6-base qt6-declarative kwin kglobalaccel kconfig kcolorscheme knotifications kcoreaddons ki18n libxkbcommon python-dbus kirigami layer-shell-qt kcmutils kirigami-addons kdeclarative kservice kdecoration kwindowsystem vulkan-headers vulkan-icd-loader wayland libepoxy libdrm python jq curl openssh glib2 libkscreen qt6-tools kpackage)
FEDORA_PACKAGES=(cmake ninja-build git gcc-c++ extra-cmake-modules qt6-qtbase-devel qt6-qtbase-private-devel qt6-qtdeclarative-devel kwin-devel kf6-kglobalaccel-devel kf6-kconfig-devel kf6-kcolorscheme-devel kf6-knotifications-devel kf6-kcoreaddons-devel kf6-ki18n-devel libxkbcommon-devel python3-dbus kf6-kirigami layer-shell-qt kf6-kcmutils-devel kf6-kirigami-addons kf6-kdeclarative kf6-kservice-devel vulkan-headers vulkan-loader-devel python3 jq curl openssh-clients)
SUSE_PACKAGES=(cmake ninja git gcc-c++ kf6-extra-cmake-modules qt6-base-devel qt6-base-private-devel qt6-declarative-devel kwin6-devel kf6-kglobalaccel-devel kf6-kconfig-devel kf6-kcolorscheme-devel kf6-knotifications-devel kf6-kcoreaddons-devel kf6-ki18n-devel libxkbcommon-devel python3-dbus-python kf6-kirigami layer-shell-qt6 kf6-kcmutils-devel kf6-kcmutils-imports kirigami-addons6 kf6-kdeclarative-imports kf6-kservice-devel vulkan-devel python3 jq curl openssh-clients)
DEBIAN_PACKAGES=(cmake ninja-build git g++ extra-cmake-modules qt6-base-dev qt6-base-private-dev qt6-declarative-dev kwin-dev libkf6globalaccel-dev libkf6config-dev libkf6colorscheme-dev libkf6notifications-dev libkf6coreaddons-dev libkf6i18n-dev libxkbcommon-dev python3-dbus qml6-module-org-kde-kirigami qml6-module-org-kde-layershell libkf6kcmutils-dev qml6-module-org-kde-kcmutils qml6-module-org-kde-kirigamiaddons-formcard qml6-module-org-kde-kquickcontrols libkf6service-dev libvulkan-dev python3 jq curl openssh-client)

if command -v pacman >/dev/null; then
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
