#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/../../.."
owner=$(stat -c %u:%g .)
trap 'chown -R "$owner" .' EXIT

pacman -Syu --noconfirm --needed git cmake ninja wayland-protocols dbus mesa xorg-xwayland qt6-wayland qt6-declarative \
    python python-dbus python-gobject python-pillow python-xlib plasma-workspace plasma5support plasma-keyboard breeze libkscreen kcmutils chromium
git config --global --add safe.directory "$PWD"
extras/packaging/dependencies.sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
setcap -r "$(command -v kwin_wayland)"
useradd -m tester
chown -R tester: .
install -d -m 700 -o tester /tmp/runtime-tester
install -d -m 1777 /tmp/.X11-unix
runuser -u tester -- env XDG_RUNTIME_DIR=/tmp/runtime-tester HOME=/home/tester \
    ctest --test-dir build -L nested --output-on-failure -j2
