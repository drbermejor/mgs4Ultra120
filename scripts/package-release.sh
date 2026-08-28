#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_DIR="$(cd -- "$SCRIPT_DIR/.." && pwd)"
VERSION="${1:-}"
PLATFORM="${2:-all}"
SOURCE_DATE_EPOCH="${SOURCE_DATE_EPOCH:-946684800}"
[[ "$VERSION" =~ ^v[0-9]+\.[0-9]+\.[0-9]+-alpha\.[0-9]+$ ]] || {
  echo "Usage: $0 vX.Y.Z-alpha.N [windows|linux|all]" >&2
  exit 1
}
[[ "$PLATFORM" == windows || "$PLATFORM" == linux || "$PLATFORM" == all ]] || {
  echo "Platform must be windows, linux, or all." >&2
  exit 1
}
BIN_DIR="${MGS4ULTRA120_BIN_DIR:-$REPO_DIR/build-mingw/bin}"
LEGACY_DLL="$BIN_DIR/winmm.dll"
ASI_PLUGIN="$BIN_DIR/MGS4Ultra120.asi"
ASI_LOADER="${MGS4ULTRA120_ASI_LOADER:-$REPO_DIR/build-third-party/ultimate-asi-loader/winmm.dll}"
WRAPPER="$BIN_DIR/launcher.exe"
[[ -f "$WRAPPER" ]] || { echo "Build the direct-launch wrapper first: $WRAPPER" >&2; exit 1; }
if [[ "$PLATFORM" == windows || "$PLATFORM" == all ]]; then
  [[ -f "$ASI_PLUGIN" ]] || { echo "Build the ASI plugin first: $ASI_PLUGIN" >&2; exit 1; }
  [[ -f "$ASI_LOADER" ]] || { echo "Fetch the pinned ASI loader first: $ASI_LOADER" >&2; exit 1; }
fi
if [[ "$PLATFORM" == linux || "$PLATFORM" == all ]]; then
  [[ -f "$LEGACY_DLL" ]] || { echo "Build the legacy Proton DLL first: $LEGACY_DLL" >&2; exit 1; }
fi

DIST="$REPO_DIR/dist"
STAGE="$(mktemp -d)"
trap 'rm -rf -- "$STAGE"' EXIT
COMMON=(README.md CHANGELOG.md LICENSE THIRD_PARTY_NOTICES.md SECURITY.md)

make_tree() {
  local platform="$1"
  local root="$STAGE/MGS4Ultra120-$VERSION-$platform"
  mkdir -p -- "$root/bin" "$root/config" "$root/docs" "$root/scripts/$platform"
  if [[ "$platform" == windows ]]; then
    install -m0644 "$ASI_LOADER" "$root/bin/winmm.dll"
    install -m0644 "$ASI_PLUGIN" "$root/bin/MGS4Ultra120.asi"
    mkdir -p -- "$root/Manual-Install/scripts"
    install -m0644 "$ASI_LOADER" "$root/Manual-Install/winmm.dll"
    install -m0644 "$ASI_PLUGIN" "$root/Manual-Install/scripts/MGS4Ultra120.asi"
    install -m0644 "$REPO_DIR/config/windows/mgs4_ultrawide.ini" \
      "$root/Manual-Install/mgs4_ultrawide.ini"
    install -m0644 "$REPO_DIR/installer/windows/MANUAL_INSTALL.txt" \
      "$root/Manual-Install/README.txt"
    mkdir -p -- "$root/third_party/ultimate_asi_loader"
    install -m0644 "$REPO_DIR/third_party/ultimate_asi_loader/README.md" \
      "$root/third_party/ultimate_asi_loader/README.md"
    install -m0644 "$REPO_DIR/third_party/ultimate_asi_loader/LICENSE.txt" \
      "$root/third_party/ultimate_asi_loader/LICENSE.txt"
  else
    install -m0644 "$LEGACY_DLL" "$root/bin/winmm.dll"
  fi
  install -m0755 "$WRAPPER" "$root/bin/launcher.exe"
  local config_source="$REPO_DIR/config/mgs4_ultrawide.ini"
  if [[ "$platform" == windows ]]; then
    config_source="$REPO_DIR/config/windows/mgs4_ultrawide.ini"
  fi
  install -m0644 "$config_source" "$root/config/mgs4_ultrawide.ini"
  for name in "${COMMON[@]}"; do install -m0644 "$REPO_DIR/$name" "$root/$name"; done
  if [[ "$platform" == windows ]]; then
    install -m0644 "$REPO_DIR/MGS4Ultra120-Setup.cmd" "$root/MGS4Ultra120-Setup.cmd"
  fi
  cp -a -- "$REPO_DIR/docs/." "$root/docs/"
  for script in "$REPO_DIR/scripts/$platform/"*; do
    [[ -f "$script" ]] && cp -a -- "$script" "$root/scripts/$platform/"
  done
  printf '%s\n' "$root"
}

mkdir -p -- "$DIST"

make_windows_zip() {
  local root="$1"
  local asset="$2"
  find "$root" -exec touch -h -d "@$SOURCE_DATE_EPOCH" {} +
  rm -f -- "$asset" "$asset.sha256"
  if command -v zip >/dev/null 2>&1; then
    (cd "$STAGE" && find "$(basename "$root")" -print | LC_ALL=C sort |
      TZ=UTC zip -X -q "$asset" -@)
  elif command -v powershell.exe >/dev/null 2>&1 &&
       { command -v cygpath >/dev/null 2>&1 ||
         command -v wslpath >/dev/null 2>&1; }; then
    local root_native asset_native
    if command -v cygpath >/dev/null 2>&1; then
      root_native="$(cygpath -w "$root")"
      asset_native="$(cygpath -w "$asset")"
    else
      root_native="$(wslpath -w "$root")"
      asset_native="$(wslpath -w "$asset")"
    fi
    powershell.exe -NoLogo -NoProfile -Command \
      "Compress-Archive -LiteralPath '$root_native' -DestinationPath '$asset_native' -CompressionLevel Optimal"
  else
    echo "Creating a Windows ZIP requires zip or Windows PowerShell interop." >&2
    exit 1
  fi
  (cd "$DIST" && sha256sum "$(basename "$asset")" >"$(basename "$asset").sha256")
  printf 'Created:\n%s\n' "$asset"
}

if [[ "$PLATFORM" == windows || "$PLATFORM" == all ]]; then
  windows_root="$(make_tree windows)"

  manual_root="$STAGE/MGS4Ultra120-$VERSION-windows-manual"
  mkdir -p -- "$manual_root"
  cp -a -- "$windows_root/Manual-Install/." "$manual_root/"

  portable_root="$STAGE/MGS4Ultra120-$VERSION-windows-portable"
  mkdir -p -- "$portable_root/scripts" "$portable_root/third_party"
  install -m0644 "$windows_root/MGS4Ultra120-Setup.cmd" \
    "$portable_root/MGS4Ultra120-Setup.cmd"
  for name in "${COMMON[@]}"; do
    install -m0644 "$windows_root/$name" "$portable_root/$name"
  done
  cp -a -- "$windows_root/bin" "$portable_root/bin"
  cp -a -- "$windows_root/config" "$portable_root/config"
  cp -a -- "$windows_root/scripts/windows" "$portable_root/scripts/windows"
  cp -a -- "$windows_root/third_party/ultimate_asi_loader" \
    "$portable_root/third_party/ultimate_asi_loader"

  make_windows_zip "$manual_root" \
    "$DIST/MGS4Ultra120-$VERSION-windows-manual.zip"
  make_windows_zip "$portable_root" \
    "$DIST/MGS4Ultra120-$VERSION-windows-portable.zip"
  make_windows_zip "$windows_root" \
    "$DIST/MGS4Ultra120-$VERSION-windows-complete.zip"
fi

if [[ "$PLATFORM" == linux || "$PLATFORM" == all ]]; then
  linux_root="$(make_tree linux)"
  # A package created from a Windows checkout must still contain executable
  # Unix scripts with LF line endings. Git's global core.autocrlf setting must
  # never leak CRLF shebangs into the Linux tarball.
  sed -i 's/\r$//' "$linux_root/scripts/linux/"*.sh \
    "$linux_root/scripts/linux/"*.py
  chmod +x "$linux_root/scripts/linux/"*.sh "$linux_root/scripts/linux/"*.py
  find "$linux_root" -exec touch -h -d "@$SOURCE_DATE_EPOCH" {} +
  linux_asset="$DIST/MGS4Ultra120-$VERSION-linux.tar.gz"
  rm -f -- "$linux_asset" "$linux_asset.sha256"
  tar --sort=name --mtime="@$SOURCE_DATE_EPOCH" --owner=0 --group=0 --numeric-owner \
    --pax-option=delete=atime,delete=ctime -C "$STAGE" -czf "$linux_asset" \
    "$(basename "$linux_root")"
  (cd "$DIST" && sha256sum "$(basename "$linux_asset")" >"$(basename "$linux_asset").sha256")
  printf 'Created:\n%s\n' "$linux_asset"
fi
