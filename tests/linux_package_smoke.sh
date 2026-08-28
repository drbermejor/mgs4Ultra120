#!/usr/bin/env bash
set -euo pipefail

PACKAGE_DIR="${1:?Usage: linux_package_smoke.sh EXTRACTED_PACKAGE_DIR}"
[[ -x "$PACKAGE_DIR/scripts/linux/install.sh" ]]
[[ -x "$PACKAGE_DIR/scripts/linux/configure.sh" ]]
[[ -x "$PACKAGE_DIR/scripts/linux/uninstall.sh" ]]
[[ -f "$PACKAGE_DIR/bin/winmm.dll" ]]

FIXTURE="$(mktemp -d -t mgs4ultra120-linux-package-XXXXXX)"
cleanup() {
  if [[ "$FIXTURE" == /tmp/mgs4ultra120-linux-package-* ]]; then
    rm -rf -- "$FIXTURE"
  fi
}
trap cleanup EXIT

GAME_DIR="$FIXTURE/steamapps/common/METAL GEAR SOLID 4/MGS4"
CONFIG="$FIXTURE/localconfig.vdf"
mkdir -p -- "$GAME_DIR"
touch "$GAME_DIR/mgs4.exe"
cat >"$CONFIG" <<'VDF'
"UserLocalConfigStore"
{
  "Software"
  {
    "Valve"
    {
      "Steam"
      {
        "apps"
        {
          "2492670"
          {
            "LaunchOptions"  "PROTON_LOG=1 %command%"
          }
        }
      }
    }
  }
}
VDF

STEAM_LOCALCONFIG="$CONFIG" MGS4_GAME_DIR="$GAME_DIR" \
  "$PACKAGE_DIR/scripts/linux/install.sh"
[[ -f "$GAME_DIR/winmm.dll" ]]
[[ -f "$GAME_DIR/mgs4_ultrawide.ini" ]]
grep -q 'WINEDLLOVERRIDES=\\"winmm=n,b\\" PROTON_LOG=1 %command%' "$CONFIG"

MGS4_GAME_DIR="$GAME_DIR" "$PACKAGE_DIR/scripts/linux/configure.sh" stable
grep -q '^FPSOverrideEnabled=0$' "$GAME_DIR/mgs4_ultrawide.ini"
grep -q '^FOVMultiplier=1.150$' "$GAME_DIR/mgs4_ultrawide.ini"

STEAM_LOCALCONFIG="$CONFIG" MGS4_GAME_DIR="$GAME_DIR" \
  "$PACKAGE_DIR/scripts/linux/uninstall.sh"
[[ ! -e "$GAME_DIR/winmm.dll" ]]
[[ ! -e "$GAME_DIR/mgs4_ultrawide.ini" ]]
grep -q '"LaunchOptions"[[:space:]]*"PROTON_LOG=1 %command%"' "$CONFIG"

echo "Linux package install/configure/uninstall smoke test passed."
