#!/usr/bin/env bash
set -euo pipefail

mode=${1:?usage: run.sh nested|coverage}
cd "$(dirname "$0")/../../.."
owner=$(stat -c %u:%g .)
trap 'chown -R "$owner" .' EXIT

tools=()
[[ $mode == coverage ]] && tools=(clang llvm)
pacman -Syu --noconfirm --needed git cmake ninja wayland-protocols dbus mesa xorg-xwayland qt6-wayland qt6-declarative \
    python python-dbus python-gobject python-pillow python-xlib plasma-workspace plasma-desktop plasma5support plasma-keyboard breeze libkscreen \
    kcmutils chromium "${tools[@]}"
git config --global --add safe.directory "$PWD"
extras/packaging/dependencies.sh
setcap -r "$(command -v kwin_wayland)"
useradd -m tester
chown -R tester: .
install -d -m 700 -o tester /tmp/runtime-tester
install -d -m 1777 /tmp/.X11-unix
as_tester() {
    runuser -u tester -- env XDG_RUNTIME_DIR=/tmp/runtime-tester HOME=/home/tester "$@"
}
case $mode in
nested)
    as_tester cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
    as_tester cmake --build build
    as_tester ctest --test-dir build -L nested --output-on-failure -j2
    ;;
coverage)
    as_tester ./tools/coverage.sh --nested
    ;;
*)
    echo "unknown mode: $mode" >&2
    exit 2
    ;;
esac
