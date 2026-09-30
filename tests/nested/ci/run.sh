#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/../../.."
owner=$(stat -c %u:%g .)
trap 'chown -R "$owner" .' EXIT

pacman -Syu --noconfirm --needed git cmake ninja clang llvm wayland-protocols typos nodejs npm shellcheck ruff dbus mesa xorg-xwayland \
    qt6-wayland qt6-declarative python python-dbus python-gobject python-pillow python-xlib plasma-workspace plasma-desktop plasma5support \
    plasma-keyboard breeze libkscreen kcmutils chromium
git clone https://github.com/mgehre/xunused.git /tmp/xunused
git -C /tmp/xunused checkout b81e0ef
if [[ ! -f /usr/include/clang/Driver/Options.h ]]; then
    sed -i 's|#include "clang/Driver/Options.h"|#include "clang/Options/Options.h"|' /tmp/xunused/main.cpp
fi
cmake -S /tmp/xunused -B /tmp/xunused/build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build /tmp/xunused/build
install -Dm755 /tmp/xunused/build/xunused /usr/local/bin/xunused
git config --global --add safe.directory "$PWD"
extras/packaging/dependencies.sh
setcap -r "$(command -v kwin_wayland)"
useradd -m tester
chown -R tester: .
install -d -m 700 -o tester /tmp/runtime-tester
install -d -m 1777 /tmp/.X11-unix
runuser -u tester -- env XDG_RUNTIME_DIR=/tmp/runtime-tester HOME=/home/tester ./tools/check.sh --nested --coverage
