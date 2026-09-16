#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

version="${1:-}"
[[ -n $version ]] || {
    echo "Usage: ./tools/release.sh <version>   (for example 0.2.0)" >&2
    exit 1
}
[[ $version =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]] || {
    echo "error: version must look like 1.2.3" >&2
    exit 1
}
[[ -z $(git status --porcelain) ]] || {
    echo "error: the working tree has uncommitted changes" >&2
    exit 1
}

sed -i -E "s/^(project\(konveyor VERSION )[0-9]+\.[0-9]+\.[0-9]+/\1${version}/" CMakeLists.txt
./tools/check.sh

git add CMakeLists.txt
git commit -m "Release ${version}"
git tag -a "v${version}" -m "Konveyor ${version}"

echo "Tagged v${version}. Push it when you are ready:"
echo "  git push origin main \"v${version}\""
