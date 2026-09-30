#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."
BUILD_DIR="${KONVEYOR_BUILD_DIR:-build}"
NESTED=false
COVERAGE=false
for argument in "$@"; do
    case "$argument" in
        --nested) NESTED=true ;;
        --coverage) COVERAGE=true ;;
        *)
            echo "usage: tools/check.sh [--nested] [--coverage]" >&2
            exit 2
            ;;
    esac
done

step() {
    printf '\n==> %s\n' "$1"
}

step "file length (at most 400 lines per file)"
mapfile -t length_checked < <(git ls-files --cached --others --exclude-standard -- '*.cpp' '*.h' '*.py' '*.qml' '*.sh' '*.kdl' 'src/cheatsheet/konveyor-cheatsheet.in' ':!widgets/')
too_long=$(wc -l "${length_checked[@]}" | awk '$2 != "total" && $1 > 400 { print $1, $2 }')
if [[ -n $too_long ]]; then
    echo "$too_long"
    exit 1
fi

scripts() {
    local file first
    while IFS= read -r file; do
        [[ -f $file && ! -L $file ]] || continue
        case $file in
        *.sh) [[ $1 == shell ]] && printf '%s\n' "$file" ;;
        *.py) [[ $1 == python ]] && printf '%s\n' "$file" ;;
        *)
            IFS= read -r first <"$file" || true
            case $first in
            '#!'*python*) [[ $1 == python ]] && printf '%s\n' "$file" ;;
            '#!'*sh*) [[ $1 == shell ]] && printf '%s\n' "$file" ;;
            esac
            ;;
        esac
    done < <(git ls-files --cached --others --exclude-standard)
    return 0
}

step "shellcheck"
SHARED_STATE=(install.sh uninstall.sh extras/packaging/common.sh extras/packaging/updates.sh extras/packaging/konveyor-rebuild
    extras/packaging/dependencies.sh widgets/lib.sh widgets/install.sh widgets/uninstall.sh)
EVAL_CHECKS=(tests/install/verify.sh)
mapfile -t shell_files < <(scripts shell | grep -vxF -f <(printf '%s\n' "${SHARED_STATE[@]}" "${EVAL_CHECKS[@]}"))
shellcheck "${shell_files[@]}"
shellcheck --exclude=SC2034 "${SHARED_STATE[@]}"
shellcheck --exclude=SC2329 "${EVAL_CHECKS[@]}"

step "ruff"
mapfile -t python_files < <(scripts python | grep -E '^(widgets|tools|tests)/')
ruff check --no-cache "${python_files[@]}"

mapfile -t cpp_files < <(git ls-files --cached --others --exclude-standard -- 'src/*.cpp' 'src/*.h' 'tests/*.cpp' 'tests/*.h')

step "clang-format"
clang-format --dry-run --Werror "${cpp_files[@]}"

step "typos"
typos

step "duplicate code (jscpd)"
npx --yes jscpd@5 src tests tools extras widgets install.sh uninstall.sh

if $COVERAGE; then
    CLANG_BUILD_DIR="${KONVEYOR_COVERAGE_BUILD_DIR:-build-coverage}"
    step "build and test with coverage"
    if $NESTED; then
        tools/coverage.sh --nested
    else
        tools/coverage.sh
    fi
else
    step "configure"
    cmake -S . -B "$BUILD_DIR" -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_EXPORT_COMPILE_COMMANDS=ON >/dev/null

    step "build"
    cmake --build "$BUILD_DIR"

    step "unit tests"
    ctest --test-dir "$BUILD_DIR" --output-on-failure -L unit -j "$(nproc)"

    if $NESTED; then
        step "nested KWin tests"
        ctest --test-dir "$BUILD_DIR" --output-on-failure -L nested -j 2
    fi

    step "clang build (compile database for clang-tidy)"
    CLANG_BUILD_DIR="${BUILD_DIR}-clang"
    cmake -S . -B "$CLANG_BUILD_DIR" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_EXPORT_COMPILE_COMMANDS=ON >/dev/null
    cmake --build "$CLANG_BUILD_DIR"
fi

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

printf '\nAll checks passed.\n'
