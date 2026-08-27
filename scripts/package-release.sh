#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_DIR="$(cd -- "$SCRIPT_DIR/.." && pwd)"
VERSION="${1:-}"
[[ "$VERSION" =~ ^v[0-9]+\.[0-9]+\.[0-9]+-alpha\.[0-9]+$ ]] || {
  echo "Usage: $0 vX.Y.Z-alpha.N" >&2
  exit 1
}
DLL="$REPO_DIR/build-mingw/bin/winmm.dll"
[[ -f "$DLL" ]] || { echo "Build the release DLL first: $DLL" >&2; exit 1; }

DIST="$REPO_DIR/dist"
STAGE="$(mktemp -d)"
trap 'rm -rf -- "$STAGE"' EXIT
COMMON=(README.md CHANGELOG.md LICENSE THIRD_PARTY_NOTICES.md SECURITY.md)

make_tree() {
  local platform="$1"
  local root="$STAGE/MGS4Ultra120-$VERSION-$platform"
  mkdir -p -- "$root/bin" "$root/config" "$root/docs" "$root/scripts/$platform"
  install -m0644 "$DLL" "$root/bin/winmm.dll"
  install -m0644 "$REPO_DIR/config/mgs4_ultrawide.ini" "$root/config/mgs4_ultrawide.ini"
  for name in "${COMMON[@]}"; do install -m0644 "$REPO_DIR/$name" "$root/$name"; done
  cp -a -- "$REPO_DIR/docs/." "$root/docs/"
  for script in "$REPO_DIR/scripts/$platform/"*; do
    [[ -f "$script" ]] && cp -a -- "$script" "$root/scripts/$platform/"
  done
  printf '%s\n' "$root"
}

mkdir -p -- "$DIST"
windows_root="$(make_tree windows)"
linux_root="$(make_tree linux)"
chmod +x "$linux_root/scripts/linux/"*.sh "$linux_root/scripts/linux/"*.py

windows_asset="$DIST/MGS4Ultra120-$VERSION-windows.zip"
linux_asset="$DIST/MGS4Ultra120-$VERSION-linux.tar.gz"
rm -f -- "$windows_asset" "$linux_asset" "$windows_asset.sha256" "$linux_asset.sha256"
(cd "$STAGE" && zip -q -r "$windows_asset" "$(basename "$windows_root")")
tar -C "$STAGE" -czf "$linux_asset" "$(basename "$linux_root")"
(cd "$DIST" && sha256sum "$(basename "$windows_asset")" >"$(basename "$windows_asset").sha256")
(cd "$DIST" && sha256sum "$(basename "$linux_asset")" >"$(basename "$linux_asset").sha256")
printf 'Created:\n%s\n%s\n' "$windows_asset" "$linux_asset"
