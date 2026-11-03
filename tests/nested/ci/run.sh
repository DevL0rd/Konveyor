#!/usr/bin/env bash
set -euo pipefail

mode="${1:?usage: run.sh lint|tidy|xunused|unit|nested|report|tests [INDEX/COUNT]}"
shard="${2:-}"

cd "$(dirname "$0")/../../.."
owner=$(stat -c %u:%g .)
trap 'chown -R "$owner" .' EXIT

if [[ $mode == lint ]]; then
    pacman -Syu --noconfirm --needed git clang typos nodejs npm shellcheck ruff python
    git config --global --add safe.directory "$PWD"
    ./tools/check.sh --lint-only
    exit
fi

pacman -Syu --noconfirm --needed git cmake ninja ccache clang llvm wayland-protocols typos nodejs npm shellcheck ruff dbus mesa xorg-xwayland \
    qt6-wayland qt6-declarative qt6-shadertools libpipewire python python-dbus python-gobject python-pillow python-xlib plasma-workspace plasma-desktop plasma5support \
    pipewire plasma-keyboard breeze libkscreen kcmutils chromium
sed -i "s/^#en_US.UTF-8 UTF-8/en_US.UTF-8 UTF-8/" /etc/locale.gen
locale-gen
if [[ $mode == xunused ]]; then
    git clone https://github.com/mgehre/xunused.git /tmp/xunused
    git -C /tmp/xunused checkout b81e0ef
    if [[ ! -f /usr/include/clang/Driver/Options.h ]]; then
        sed -i 's|#include "clang/Driver/Options.h"|#include "clang/Options/Options.h"|' /tmp/xunused/main.cpp
    fi
    if [[ ! -f /usr/include/clang/Index/USRGeneration.h ]]; then
        sed -i 's|#include "clang/Index/USRGeneration.h"|#include "clang/UnifiedSymbolResolution/USRGeneration.h"|' /tmp/xunused/main.cpp
    fi
    cmake -S /tmp/xunused -B /tmp/xunused/build -G Ninja -DCMAKE_BUILD_TYPE=Release
    cmake --build /tmp/xunused/build
    install -Dm755 /tmp/xunused/build/xunused /usr/local/bin/xunused
fi
git config --global --add safe.directory "$PWD"
extras/packaging/dependencies.sh
setcap -r "$(command -v kwin_wayland)"
useradd -m tester
chown -R tester: .
install -d -m 700 -o tester /tmp/runtime-tester
install -d -m 1777 /tmp/.X11-unix

as_tester() {
    runuser -u tester -- env QT_QUICK_BACKEND=software XDG_RUNTIME_DIR=/tmp/runtime-tester HOME=/home/tester CCACHE_DIR="$PWD/.ccache" CCACHE_MAXSIZE=2G \
        CMAKE_C_COMPILER_LAUNCHER=ccache CMAKE_CXX_COMPILER_LAUNCHER=ccache "$@"
}

case $mode in
tests) as_tester ./tools/check.sh --nested --tests "${KONVEYOR_CI_TESTS:?}" ;;
tidy) as_tester ./tools/check.sh --tidy-only ;;
xunused) as_tester ./tools/check.sh --xunused-only ;;
unit) as_tester ./tools/coverage.sh --only unit --shard "${shard:?}" --no-report ;;
nested) as_tester ./tools/coverage.sh --only nested --shard "${shard:?}" --no-report ;;
report) as_tester ./tools/coverage.sh --nested --report-only --merge coverage-in ;;
*)
    echo "unknown mode $mode" >&2
    exit 2
    ;;
esac
runuser -u tester -- env CCACHE_DIR="$PWD/.ccache" ccache --show-stats
