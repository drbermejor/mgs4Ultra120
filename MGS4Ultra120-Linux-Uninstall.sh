#!/usr/bin/env bash
set -euo pipefail

if (( $# > 1 )); then
  echo "Usage: $0 [folder-containing-mgs4.exe]" >&2
  exit 1
fi
if (( $# == 1 )); then
  export MGS4_GAME_DIR="$1"
fi

PACKAGE_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
exec "$PACKAGE_DIR/scripts/linux/uninstall.sh"
