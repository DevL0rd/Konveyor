#!/usr/bin/env bash
set -euo pipefail

version="${1:-}"
[[ -n $version ]] || {
    echo "Usage: ./tools/source-archive.sh <version> [directory]" >&2
    exit 1
}
directory="$(cd "${2:-.}" && pwd)"
cd "$(dirname "$0")/.."

if git submodule status --recursive | grep -q '^[-+U]'; then
    echo "error: the submodules aren't checked out at their recorded commits; run: git submodule update --init --recursive" >&2
    exit 1
fi

archive="$directory/konveyor-${version}.tar.gz"
git ls-files -z --recurse-submodules \
    | tar --null --files-from=- --transform="flags=rh;s|^|konveyor-${version}/|" --sort=name --owner=0 --group=0 --numeric-owner -czf "$archive"
echo "$archive"
