#!/usr/bin/env bash
set -euo pipefail

ARCHIVE="${1:?Usage: audit_linux_tar.sh ARCHIVE VERSION}"
VERSION="${2:?Usage: audit_linux_tar.sh ARCHIVE VERSION}"
ARCHIVE="$(realpath -- "$ARCHIVE")"
HASH_FILE="$ARCHIVE.sha256"
AUDIT_DIR="$(mktemp -d -t mgs4ultra120-linux-tar-XXXXXX)"

tar -xzf "$ARCHIVE" -C "$AUDIT_DIR"
ROOT="$AUDIT_DIR/MGS4Ultra120-$VERSION-linux"
[[ -d "$ROOT" ]]

if grep -rIl $'\r' "$ROOT/scripts/linux" | grep -q .; then
  echo "CRLF found in packaged Linux scripts" >&2
  exit 1
fi

"$(dirname -- "$0")/linux_package_smoke.sh" "$ROOT"
file "$ROOT/bin/winmm.dll" "$ROOT/bin/launcher.exe"

(
  cd -- "$(dirname -- "$ARCHIVE")"
  sha256sum -c "$(basename -- "$HASH_FILE")"
)

echo "Extracted Linux audit retained at: $AUDIT_DIR"
