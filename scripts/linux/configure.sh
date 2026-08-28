#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PACKAGE_DIR="$(cd -- "$SCRIPT_DIR/../.." && pwd)"
GAME_DIR="${MGS4_GAME_DIR:-$HOME/.local/share/Steam/steamapps/common/METAL GEAR SOLID 4/MGS4}"
INI="$GAME_DIR/mgs4_ultrawide.ini"
BACKUP_DIR="$GAME_DIR/.mgs4ultra120-backup"
STATE_FILE="$BACKUP_DIR/steam-options.json"
INSTALL_DIR="$(dirname -- "$GAME_DIR")"
LAUNCHER_DIR="$INSTALL_DIR/Launcher"
LAUNCHER_TARGET="$LAUNCHER_DIR/launcher.exe"
LAUNCHER_BACKUP="$BACKUP_DIR/launcher.exe.preinstall"
WRAPPER_SOURCE="$PACKAGE_DIR/bin/launcher.exe"
KNOWN_EXE_SHA256="9e8df67ea7f41e7f8306ce1a77584707209069b3c75389b3f00445efe459fe41"
MODE="${1:-gui}"

die() { echo "ERROR: $*" >&2; exit 1; }
ini_value() { sed -n "s/^$1=//p" "$INI" | tr -d '\r' | head -n1; }
game_running() {
  pgrep -x 'mgs4.exe' >/dev/null || pgrep -f -- '^-region eu -lan ' >/dev/null
}

[[ -f "$INI" ]] || die "mgs4_ultrawide.ini not found in: $GAME_DIR"
[[ -f "$GAME_DIR/mgs4.exe" ]] || die "mgs4.exe not found in: $GAME_DIR"
game_running && die "Exit the game before changing settings."

apply_values() {
  local width="$1" height="$2" fov="$3" ultrawide="$4" controller_fix="$5" skip_launcher="$6" language="$7" allow_unsupported="$8"
  python3 - "$INI" "$width" "$height" "$fov" "$ultrawide" "$controller_fix" "$skip_launcher" "$language" "$allow_unsupported" <<'PY'
import math
import re
import sys
from pathlib import Path

path = Path(sys.argv[1])
width, height, fov, ultrawide, controller_fix, skip_launcher, language, allow_unsupported = sys.argv[2:]
try:
    width_i, height_i = int(width), int(height)
    fov_f = float(fov.replace(",", "."))
except ValueError as error:
    raise SystemExit(f"Invalid numeric value: {error}")
if not (640 <= width_i <= 16384 and 480 <= height_i <= 16384):
    raise SystemExit("Width/height are outside the allowed range")
if not (0.5 <= fov_f <= 2.0) or not math.isfinite(fov_f):
    raise SystemExit("FOV multiplier must be between 0.5 and 2.0")
if language not in ("en", "sp", "fr", "it", "ge", "jp"):
    raise SystemExit("Unsupported direct-launch language")
values = {
    "Width": str(width_i), "Height": str(height_i),
    "FOVMultiplier": f"{fov_f:.3f}", "Limit": "60",
    "UltrawideEnabled": ultrawide,
    "FPSOverrideEnabled": "0",
    "ControllerProfileFixEnabled": controller_fix,
    "SkipUnityLauncher": skip_launcher,
    "Language": language,
    "AllowUnsupportedExecutable": allow_unsupported,
}
text = path.read_text()
for key, value in values.items():
    pattern = re.compile(rf"(?m)^{re.escape(key)}=.*$")
    if not pattern.search(text):
        raise SystemExit(f"Missing {key} in {path}; reinstall the current patch")
    text = pattern.sub(f"{key}={value}", text, count=1)
temporary = path.with_suffix(".ini.tmp")
temporary.write_text(text)
temporary.replace(path)
PY
}

set_launcher_wrapper() {
  local enabled="$1"
  local source_hash target_hash timestamp
  [[ -d "$LAUNCHER_DIR" && -f "$LAUNCHER_TARGET" ]] || die "Unity launcher not found in: $LAUNCHER_DIR"
  mkdir -p -- "$BACKUP_DIR"
  if [[ "$enabled" == 1 ]]; then
    [[ -f "$WRAPPER_SOURCE" ]] || die "Direct-launch wrapper not found in: $PACKAGE_DIR/bin"
    source_hash="$(sha256sum "$WRAPPER_SOURCE" | awk '{print $1}')"
    target_hash="$(sha256sum "$LAUNCHER_TARGET" | awk '{print $1}')"
    [[ "$source_hash" == "$target_hash" ]] && return 0
    if [[ ! -e "$LAUNCHER_BACKUP" ]]; then
      cp -a -- "$LAUNCHER_TARGET" "$LAUNCHER_BACKUP"
    else
      timestamp="$(date -u +%Y%m%dT%H%M%SZ)"
      cp -a -- "$LAUNCHER_BACKUP" "$LAUNCHER_BACKUP.$timestamp"
      cp -a -- "$LAUNCHER_TARGET" "$LAUNCHER_BACKUP"
    fi
    install -m0755 "$WRAPPER_SOURCE" "$LAUNCHER_TARGET"
  elif [[ -e "$LAUNCHER_BACKUP" ]]; then
    [[ -f "$WRAPPER_SOURCE" ]] || die "Direct-launch wrapper is missing; refusing an ambiguous launcher restore."
    source_hash="$(sha256sum "$WRAPPER_SOURCE" | awk '{print $1}')"
    target_hash="$(sha256sum "$LAUNCHER_TARGET" | awk '{print $1}')"
    if [[ "$source_hash" == "$target_hash" ]]; then
      mv -- "$LAUNCHER_BACKUP" "$LAUNCHER_TARGET"
    else
      echo "Launcher changed outside MGS4 Ultra120; leaving it and its backup untouched." >&2
    fi
  fi
}

show_status() {
  local actual
  actual="$(sha256sum "$GAME_DIR/mgs4.exe" | awk '{print $1}')"
  if [[ "$actual" == "$KNOWN_EXE_SHA256" ]]; then
    echo "Supported executable: yes ($actual)"
  else
    echo "Supported executable: NO"
    echo "Expected: $KNOWN_EXE_SHA256"
    echo "Actual:   $actual"
    return 1
  fi
}

current_skip="$(ini_value SkipUnityLauncher)"
current_language="$(ini_value Language)"
current_allow_unsupported="$(ini_value AllowUnsupportedExecutable)"
case "$MODE" in
  stable) apply_values 3440 1440 1.150 1 1 "$current_skip" "$current_language" "$current_allow_unsupported"; exit 0 ;;
  ultrawide-only) apply_values 3440 1440 1.150 1 0 "$current_skip" "$current_language" "$current_allow_unsupported"; exit 0 ;;
  controller-fix-only) apply_values 3440 1440 1.000 0 1 "$current_skip" "$current_language" "$current_allow_unsupported"; exit 0 ;;
  status) show_status; exit $? ;;
  gui) ;;
  *) die "Usage: $0 [gui|stable|ultrawide-only|controller-fix-only|status]" ;;
esac

command -v zenity >/dev/null || die "zenity is required for GUI mode."

width="$(ini_value Width)"; height="$(ini_value Height)"
fov="$(ini_value FOVMultiplier)"
ultrawide="$(ini_value UltrawideEnabled)"
controller_fix="$(ini_value ControllerProfileFixEnabled)"
skip_launcher="$(ini_value SkipUnityLauncher)"; language="$(ini_value Language)"
allow_unsupported="$(ini_value AllowUnsupportedExecutable)"
if [[ "$ultrawide" == 1 ]]; then ultrawide_values='Enabled|Disabled'
else ultrawide_values='Disabled|Enabled'; fi
if [[ "$controller_fix" == 1 ]]; then controller_values='Enabled (recommended)|Disabled'
else controller_values='Disabled|Enabled (recommended)'; fi
if [[ "$skip_launcher" == 1 ]]; then launcher_values='Skip Unity launcher|Use original Unity launcher'
else launcher_values='Use original Unity launcher|Skip Unity launcher'; fi
if [[ "$allow_unsupported" == 1 ]]; then unsupported_values='Attempt unsupported executable (unsafe)|Block unsupported executable'
else unsupported_values='Block unsupported executable|Attempt unsupported executable (unsafe)'; fi
language_values="$language"
for value in en sp fr it ge jp; do [[ "|$language_values|" == *"|$value|"* ]] || language_values+="|$value"; done
result="$(zenity --forms --title='MGS4 Ultra120 configurator' \
  --text='Changes apply on the next game start. Leave text fields empty to retain their values.' \
  --separator='|' \
  --add-entry="Width (current: $width)" --add-entry="Height (current: $height)" \
  --add-entry="FOV (current: $fov; 1.150 recommended; 1.000 original framing)" \
  --add-combo='Ultrawide module' --combo-values="$ultrawide_values" \
  --add-combo='Controller profile fix' --combo-values="$controller_values" \
  --add-combo='Steam launch path' --combo-values="$launcher_values" \
  --add-combo='Direct-launch language' --combo-values="$language_values" \
  --add-combo='Unknown executable policy' --combo-values="$unsupported_values" \
  --add-combo='Linux fullscreen launch' --combo-values='Keep current Steam options|Gamescope fullscreen|Native/no Gamescope' \
  --add-entry='Gamescope refresh rate (default: 60)' \
  --width=820 --height=640)" || exit 0

IFS='|' read -r new_width new_height new_fov new_ultrawide new_controller new_launcher new_language new_unsupported steam_mode refresh <<<"$result"
new_width="${new_width:-$width}"; new_height="${new_height:-$height}"
new_fov="${new_fov:-$fov}"; refresh="${refresh:-60}"
[[ "$new_ultrawide" == Enabled ]] && ultrawide_value=1 || ultrawide_value=0
[[ "$new_controller" == Enabled* ]] && controller_value=1 || controller_value=0
[[ "$new_launcher" == 'Skip Unity launcher' ]] && launcher_value=1 || launcher_value=0
[[ "$new_unsupported" == Attempt* ]] && unsupported_value=1 || unsupported_value=0

if [[ "$unsupported_value" == 1 ]]; then
  zenity --question --title='Unsupported executable override' \
    --text='Known code/data offsets will be attempted on an unverified executable. This may crash the game or corrupt its process state. Continue under your responsibility?' || exit 0
fi

set_launcher_wrapper "$launcher_value"
apply_values "$new_width" "$new_height" "$new_fov" "$ultrawide_value" "$controller_value" "$launcher_value" "$new_language" "$unsupported_value"

launch_note='Steam launch options were left unchanged.'
if [[ "$steam_mode" != 'Keep current Steam options' ]]; then
  if pgrep -x steam >/dev/null; then
    launch_note='Steam is running, so launch options were NOT changed. Exit Steam and run the configurator again.'
  elif [[ ! -f "$STATE_FILE" ]]; then
    launch_note='Install the patch first; the reversible Steam-options state file is missing.'
  else
    launch_mode=native
    [[ "$steam_mode" == 'Gamescope fullscreen' ]] && launch_mode=gamescope
    if [[ "$launch_mode" == gamescope ]] && ! command -v gamescope >/dev/null 2>&1; then
      launch_note='Gamescope was not found, so Steam launch options were NOT changed. Install Gamescope first or select Native/no Gamescope.'
    else
      python3 "$SCRIPT_DIR/steam_options.py" configure "$STATE_FILE" \
        "$new_width" "$new_height" "$refresh" "$launch_mode"
      launch_note="Steam launch mode updated: $steam_mode."
    fi
  fi
fi

status='supported executable'
show_status >/dev/null 2>&1 || status='UNSUPPORTED executable; the DLL will fail safely'
zenity --info --title='MGS4 Ultra120' \
  --text="Saved: ultrawide ${new_ultrawide}, ${new_width}x${new_height}, FOV ${new_fov}; controller fix ${new_controller}; ${new_launcher}.\n${launch_note}\nStatus: ${status}."
