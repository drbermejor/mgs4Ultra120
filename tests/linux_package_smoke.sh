#!/usr/bin/env bash
set -euo pipefail

PACKAGE_DIR="${1:?Usage: linux_package_smoke.sh EXTRACTED_PACKAGE_DIR [OFFICIAL_MGSFPSUNLOCK_ZIP]}"
FPS_ARCHIVE="${2:-}"
[[ -x "$PACKAGE_DIR/scripts/linux/install.sh" ]]
[[ -x "$PACKAGE_DIR/scripts/linux/configure.sh" ]]
[[ -x "$PACKAGE_DIR/scripts/linux/uninstall.sh" ]]
[[ -x "$PACKAGE_DIR/MGS4Ultra120-Linux-Setup.sh" ]]
[[ -x "$PACKAGE_DIR/MGS4Ultra120-Linux-Configure.sh" ]]
[[ -x "$PACKAGE_DIR/MGS4Ultra120-Linux-Uninstall.sh" ]]
[[ -f "$PACKAGE_DIR/bin/winmm.dll" ]]
[[ -f "$PACKAGE_DIR/bin/MGS4Ultra120.asi" ]]

FIXTURE="$(mktemp -d -t mgs4ultra120-linux-package-XXXXXX)"
cleanup() {
  if [[ "$FIXTURE" == /tmp/mgs4ultra120-linux-package-* ]]; then
    rm -rf -- "$FIXTURE"
  fi
}
trap cleanup EXIT

GAME_DIR="$FIXTURE/steamapps/common/METAL GEAR SOLID 4/MGS4"
LAUNCHER_DIR="$FIXTURE/steamapps/common/METAL GEAR SOLID 4/Launcher"
CONFIG="$FIXTURE/localconfig.vdf"
LOCAL_PACKAGE="$FIXTURE/local-package"
XDG_DATA="$FIXTURE/xdg-data"
DESKTOP_DIR="$FIXTURE/Desktop"
mkdir -p -- "$GAME_DIR" "$LAUNCHER_DIR" "$DESKTOP_DIR"
touch "$GAME_DIR/mgs4.exe"
printf 'original launcher fixture' >"$LAUNCHER_DIR/launcher.exe"
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

install_environment=(
  STEAM_LOCALCONFIG="$CONFIG"
  MGS4_GAME_DIR="$GAME_DIR"
  MGS4_CONFIGURE_AFTER_INSTALL=0
  MGS4_PACKAGE_INSTALL_DIR="$LOCAL_PACKAGE"
  XDG_DATA_HOME="$XDG_DATA"
  MGS4_DESKTOP_DIR="$DESKTOP_DIR"
)
if [[ -n "$FPS_ARCHIVE" ]]; then
  install_environment+=(MGS4_FPS_UNLOCK_ARCHIVE="$FPS_ARCHIVE")
else
  install_environment+=(MGS4_INSTALL_FPS_UNLOCK=0)
fi
env "${install_environment[@]}" "$PACKAGE_DIR/MGS4Ultra120-Linux-Setup.sh"
[[ -x "$LOCAL_PACKAGE/MGS4Ultra120-Linux-Configure.sh" ]]
[[ -f "$GAME_DIR/winmm.dll" ]]
[[ -f "$GAME_DIR/mgs4_ultrawide.ini" ]]
[[ -f "$GAME_DIR/scripts/MGS4Ultra120.asi" ]]
cmp -s "$PACKAGE_DIR/bin/launcher.exe" "$LAUNCHER_DIR/launcher.exe"
if [[ -n "$FPS_ARCHIVE" ]]; then
  [[ -f "$GAME_DIR/scripts/MGSFPSUnlock.asi" ]]
  [[ -f "$GAME_DIR/scripts/MGSFPSUnlock.ini" ]]
  grep -q '^TargetFrameRate = 120$' "$GAME_DIR/scripts/MGSFPSUnlock.ini"
else
  [[ ! -e "$GAME_DIR/scripts/MGSFPSUnlock.asi" ]]
  [[ ! -e "$GAME_DIR/scripts/MGSFPSUnlock.ini" ]]
fi
grep -q 'WINEDLLOVERRIDES=\\"winmm=n,b\\" PROTON_LOG=1 %command%' "$CONFIG"
[[ -x "$XDG_DATA/applications/mgs4ultra120-configure.desktop" ]]
[[ -x "$DESKTOP_DIR/MGS4 Ultra120 Configurator.desktop" ]]
grep -q "Exec=\"$LOCAL_PACKAGE/MGS4Ultra120-Linux-Configure.sh\"" \
  "$DESKTOP_DIR/MGS4 Ultra120 Configurator.desktop"

MGS4_GAME_DIR="$GAME_DIR" "$LOCAL_PACKAGE/scripts/linux/configure.sh" stable
grep -q '^FPSOverrideEnabled=0$' "$GAME_DIR/mgs4_ultrawide.ini"
grep -q '^FOVMultiplier=1.050$' "$GAME_DIR/mgs4_ultrawide.ini"

STEAM_LOCALCONFIG="$CONFIG" MGS4_GAME_DIR="$GAME_DIR" \
  XDG_DATA_HOME="$XDG_DATA" MGS4_DESKTOP_DIR="$DESKTOP_DIR" \
  "$LOCAL_PACKAGE/scripts/linux/uninstall.sh"
[[ ! -e "$GAME_DIR/winmm.dll" ]]
[[ ! -e "$GAME_DIR/mgs4_ultrawide.ini" ]]
[[ ! -e "$GAME_DIR/scripts/MGS4Ultra120.asi" ]]
[[ ! -e "$GAME_DIR/scripts/MGSFPSUnlock.ini" ]]
[[ ! -e "$GAME_DIR/scripts/MGSFPSUnlock.asi" ]]
grep -q 'original launcher fixture' "$LAUNCHER_DIR/launcher.exe"
grep -q '"LaunchOptions"[[:space:]]*"PROTON_LOG=1 %command%"' "$CONFIG"
[[ ! -e "$XDG_DATA/applications/mgs4ultra120-configure.desktop" ]]
[[ ! -e "$DESKTOP_DIR/MGS4 Ultra120 Configurator.desktop" ]]

echo "Linux package install/configure/uninstall smoke test passed."
