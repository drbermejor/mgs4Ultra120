#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_DIR="$(cd -- "$SCRIPT_DIR/.." && pwd)"
FIXTURE="$(mktemp -d -t mgs4ultra120-linux-config-XXXXXX)"
cleanup() {
  if [[ "$FIXTURE" == /tmp/mgs4ultra120-linux-config-* ]]; then
    rm -rf -- "$FIXTURE"
  fi
}
trap cleanup EXIT

touch "$FIXTURE/mgs4.exe"
cp "$REPO_DIR/config/mgs4_ultrawide.ini" "$FIXTURE/mgs4_ultrawide.ini"
mkdir -p "$FIXTURE/scripts"
cp "$REPO_DIR/config/MGSFPSUnlock.ini" "$FIXTURE/scripts/MGSFPSUnlock.ini"

MGS4_GAME_DIR="$FIXTURE" "$REPO_DIR/scripts/linux/configure.sh" stable
grep -q '^UltrawideEnabled=1$' "$FIXTURE/mgs4_ultrawide.ini"
grep -q '^FPSOverrideEnabled=0$' "$FIXTURE/mgs4_ultrawide.ini"
grep -q '^Limit=60$' "$FIXTURE/mgs4_ultrawide.ini"
grep -q '^ControllerProfileFixEnabled=1$' "$FIXTURE/mgs4_ultrawide.ini"
grep -q '^TargetFrameRate = 120$' "$FIXTURE/scripts/MGSFPSUnlock.ini"

MGS4_GAME_DIR="$FIXTURE" \
  "$REPO_DIR/scripts/linux/configure.sh" controller-fix-only
grep -q '^UltrawideEnabled=0$' "$FIXTURE/mgs4_ultrawide.ini"
grep -q '^ControllerProfileFixEnabled=1$' "$FIXTURE/mgs4_ultrawide.ini"

rm -f "$FIXTURE/scripts/MGSFPSUnlock.ini"
MGS4_GAME_DIR="$FIXTURE" "$REPO_DIR/scripts/linux/configure.sh" stable
grep -q '^UltrawideEnabled=1$' "$FIXTURE/mgs4_ultrawide.ini"

echo "Linux non-GUI configurator smoke test passed."
