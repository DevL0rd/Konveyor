#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."
output="${1:-docs/screenshot.png}"
client="$PWD/tests/nested/clients/client.qml"
shot="$PWD/tests/nested/harness/screenshot.py"

session=$(mktemp -d)
cat >"$session/run.sh" <<INNER
export QT_QPA_PLATFORM=wayland
qml6 $client -- Terminal 600 400 &
sleep 2
qml6 $client -- Editor 900 600 &
sleep 2
qml6 $client -- Browser 700 500 &
sleep 4
python3 $shot "$PWD/$output"
INNER

python3 tests/nested/harness/nested.py "$session/run.sh" --timeout 90
rm -rf "$session"
echo "wrote $output"
