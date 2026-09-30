#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."
BUILD_DIR="${KONVEYOR_COVERAGE_BUILD_DIR:-build-coverage}"
OUTPUT="$(realpath -m "$BUILD_DIR")/coverage"
THRESHOLDS=tests/coverage/thresholds.json
LABELS=(unit)
[[ "${1:-}" == "--nested" ]] && LABELS+=(nested)
FLAGS="-fprofile-instr-generate -fcoverage-mapping"

step() {
    printf '\n==> %s\n' "$1"
}

step "configure the coverage build (clang source-based coverage)"
cmake -S . -B "$BUILD_DIR" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
    -DCMAKE_C_FLAGS="$FLAGS" -DCMAKE_CXX_FLAGS="$FLAGS" -DCMAKE_EXE_LINKER_FLAGS="$FLAGS" \
    -DCMAKE_SHARED_LINKER_FLAGS="$FLAGS" -DCMAKE_MODULE_LINKER_FLAGS="$FLAGS" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON >/dev/null

step "build"
cmake --build "$BUILD_DIR"

rm -rf "$OUTPUT"
mkdir -p "$OUTPUT/profiles"
python3 tools/coverage/pycoverage.py prepare --data "$OUTPUT/python"

for label in "${LABELS[@]}"; do
    step "$label tests with coverage"
    jobs=$(nproc)
    [[ $label == nested ]] && jobs=2
    LLVM_PROFILE_FILE="$OUTPUT/profiles/%p-%m.profraw" \
        KONVEYOR_PYCOVERAGE_DIR="$OUTPUT/python" \
        PYTHONPATH="$PWD/tools/coverage/site${PYTHONPATH:+:$PYTHONPATH}" \
        ctest --test-dir "$BUILD_DIR" --output-on-failure -L "^${label}\$" -j "$jobs"
done

step "merge the C++ profiles"
llvm-profdata merge -sparse "$OUTPUT"/profiles/*.profraw -o "$OUTPUT/merged.profdata"
objects=()
while IFS= read -r binary; do
    if llvm-readelf --section-headers "$binary" 2>/dev/null | grep -q __llvm_covmap; then
        objects+=(-object "$binary")
    fi
done < <(find "$BUILD_DIR/bin" "$BUILD_DIR/tests" -type f \( -perm -u+x -o -name '*.so' \) | sort)
ignored='(^|/)(tests|build[^/]*|usr)/|_autogen/|\.moc$|moc_'
llvm-cov export -summary-only -instr-profile "$OUTPUT/merged.profdata" -ignore-filename-regex "$ignored" "${objects[@]:1}" \
    >"$OUTPUT/cpp.json"
llvm-cov show -format=html -show-branches=count -output-dir "$OUTPUT/html" -instr-profile "$OUTPUT/merged.profdata" \
    -ignore-filename-regex "$ignored" "${objects[@]:1}"

step "coverage report"
status=0
python3 tools/coverage/cxxreport.py --export "$OUTPUT/cpp.json" --thresholds "$THRESHOLDS" --markdown "$OUTPUT/cpp.md" || status=1
python3 tools/coverage/pycoverage.py report --data "$OUTPUT/python" --thresholds "$THRESHOLDS" --markdown "$OUTPUT/python.md" \
    --json "$OUTPUT/python.json" || status=1
cat "$OUTPUT/cpp.md" "$OUTPUT/python.md" >"$OUTPUT/summary.md"
printf '\nThe HTML report is in %s/html/index.html\n' "$OUTPUT"
exit "$status"
