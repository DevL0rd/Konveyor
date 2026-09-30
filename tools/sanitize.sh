#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."
BUILD_DIR="${KONVEYOR_SANITIZE_BUILD_DIR:-build-sanitize}"
LABEL="${1:-layout}"
FLAGS="-fsanitize=address,undefined -fno-sanitize-recover=undefined -fno-omit-frame-pointer -D_GLIBCXX_ASSERTIONS"

step() {
    printf '\n==> %s\n' "$1"
}

step "configure the sanitizer build (AddressSanitizer, UndefinedBehaviorSanitizer, libstdc++ assertions)"
cmake -S . -B "$BUILD_DIR" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DKONVEYOR_BUILD_EFFECT=OFF -DCMAKE_CXX_FLAGS="$FLAGS" \
    -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined" >/dev/null

mapfile -t targets < <(ctest --test-dir "$BUILD_DIR" -N -L "$LABEL" | sed -n 's/^ *Test *#[0-9]*: //p')
if [[ ${#targets[@]} -eq 0 ]]; then
    echo "no tests carry the label $LABEL"
    exit 1
fi

step "build the $LABEL tests"
cmake --build "$BUILD_DIR" --target "${targets[@]}"

step "$LABEL tests under the sanitizers"
ctest --test-dir "$BUILD_DIR" --output-on-failure --parallel "$(nproc)" -L "$LABEL"
