#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."
BUILD_DIR="${KONVEYOR_BUILD_DIR:-build}"

step() {
    printf '\n==> %s\n' "$1"
}

step "file length (at most 400 lines per file)"
mapfile -t length_checked < <(git ls-files --cached --others --exclude-standard -- '*.cpp' '*.h' '*.py' '*.qml' '*.sh' '*.kdl' 'src/cheatsheet/konveyor-cheatsheet')
too_long=$(wc -l "${length_checked[@]}" | awk '$2 != "total" && $1 > 400 { print $1, $2 }')
if [[ -n $too_long ]]; then
    echo "$too_long"
    exit 1
fi

step "configure"
cmake -S . -B "$BUILD_DIR" -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_EXPORT_COMPILE_COMMANDS=ON >/dev/null

step "build"
cmake --build "$BUILD_DIR"

step "unit tests"
ctest --test-dir "$BUILD_DIR" --output-on-failure -L unit

mapfile -t cpp_files < <(git ls-files --cached --others --exclude-standard -- 'src/*.cpp' 'src/*.h' 'tests/*.cpp' 'tests/*.h')

step "clang-format"
clang-format --dry-run --Werror "${cpp_files[@]}"

step "clang build (compile database for clang-tidy)"
CLANG_BUILD_DIR="${BUILD_DIR}-clang"
cmake -S . -B "$CLANG_BUILD_DIR" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_EXPORT_COMPILE_COMMANDS=ON >/dev/null
cmake --build "$CLANG_BUILD_DIR"

step "clang-tidy (complexity, bugprone, performance)"
mapfile -t tidy_files < <(printf '%s\n' "${cpp_files[@]}" | grep -E '^src/.*\.cpp$')
run-clang-tidy -quiet -p "$CLANG_BUILD_DIR" "${tidy_files[@]}"

step "unused functions (xunused)"
command -v xunused >/dev/null || { echo "xunused is not installed: https://github.com/mgehre/xunused"; exit 1; }
mapfile -t all_sources < <(git ls-files --cached --others --exclude-standard -- 'src/*.cpp' 'tests/*.cpp')
unused=$(xunused -p "$CLANG_BUILD_DIR" --extra-arg=-resource-dir="$(clang -print-resource-dir)" "${all_sources[@]}" 2>&1 | grep -E "warning: Function|Failed to run" || true)
if [[ -n $unused ]]; then
    echo "$unused"
    exit 1
fi

step "typos"
typos

step "duplicate code (jscpd)"
npx --yes jscpd@5 src tests tools extras

if [[ "${1:-}" == "--nested" ]]; then
    step "nested KWin tests"
    ctest --test-dir "$BUILD_DIR" --output-on-failure -L nested
fi

printf '\nAll checks passed.\n'
