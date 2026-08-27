#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
GAME_DIR="${MGS4_GAME_DIR:-$HOME/.local/share/Steam/steamapps/common/METAL GEAR SOLID 4/MGS4}"
INI="$GAME_DIR/mgs4_ultrawide.ini"
STATE_FILE="$GAME_DIR/.mgs4ultra120-backup/steam-options.json"
KNOWN_EXE_SHA256="9e8df67ea7f41e7f8306ce1a77584707209069b3c75389b3f00445efe459fe41"
MODE="${1:-gui}"

die() { echo "ERROR: $*" >&2; exit 1; }
ini_value() { sed -n "s/^$1=//p" "$INI" | head -n1; }
game_running() {
  pgrep -x 'mgs4.exe' >/dev/null || pgrep -f -- '^-region eu -lan ' >/dev/null
}

[[ -f "$INI" ]] || die "mgs4_ultrawide.ini not found in: $GAME_DIR"
[[ -f "$GAME_DIR/mgs4.exe" ]] || die "mgs4.exe not found in: $GAME_DIR"
game_running && die "Exit the game before changing settings."

apply_values() {
  local width="$1" height="$2" fov="$3" fps="$4" ui="$5" ultrawide="$6" fps_override="$7"
  python3 - "$INI" "$width" "$height" "$fov" "$fps" "$ui" "$ultrawide" "$fps_override" <<'PY'
import math
import re
import sys
from pathlib import Path

path = Path(sys.argv[1])
width, height, fov, fps, ui, ultrawide, fps_override = sys.argv[2:]
try:
    width_i, height_i, fps_i = int(width), int(height), int(fps)
    fov_f = float(fov.replace(",", "."))
except ValueError as error:
    raise SystemExit(f"Invalid numeric value: {error}")
if not (640 <= width_i <= 16384 and 480 <= height_i <= 16384):
    raise SystemExit("Width/height are outside the allowed range")
if not (0.5 <= fov_f <= 2.0) or not math.isfinite(fov_f):
    raise SystemExit("FOV multiplier must be between 0.5 and 2.0")
if fps_i not in (30, 60, 120):
    raise SystemExit("FPS must be 30, 60, or 120")
values = {
    "Width": str(width_i), "Height": str(height_i),
    "FOVMultiplier": f"{fov_f:.3f}", "Limit": str(fps_i),
    "ConstrainUITo16x9": ui, "UltrawideEnabled": ultrawide,
    "FPSOverrideEnabled": fps_override,
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

case "$MODE" in
  stable) apply_values 3440 1440 1.000 60 0 1 1; exit 0 ;;
  ui-safe) apply_values 3440 1440 1.000 60 1 1 1; exit 0 ;;
  120) apply_values 3440 1440 1.000 120 0 1 1; exit 0 ;;
  120-ui) apply_values 3440 1440 1.000 120 1 1 1; exit 0 ;;
  fps-only-120) apply_values 3440 1440 1.000 120 0 0 1; exit 0 ;;
  ultrawide-only) apply_values 3440 1440 1.000 60 0 1 0; exit 0 ;;
  status) show_status; exit $? ;;
  gui) ;;
  *) die "Usage: $0 [gui|stable|ui-safe|120|120-ui|fps-only-120|ultrawide-only|status]" ;;
esac

command -v zenity >/dev/null || die "zenity is required for GUI mode."

width="$(ini_value Width)"; height="$(ini_value Height)"
fov="$(ini_value FOVMultiplier)"; fps="$(ini_value Limit)"
ui="$(ini_value ConstrainUITo16x9)"
ultrawide="$(ini_value UltrawideEnabled)"; fps_override="$(ini_value FPSOverrideEnabled)"
fps_values="$fps"
for value in 60 120 30; do [[ "$fps_values" == *"$value"* ]] || fps_values+="|$value"; done
ui_current='Original UI (stable)'; [[ "$ui" == 1 ]] && ui_current='Centered 16:9 UI (experimental)'
if [[ "$ui" == 1 ]]; then ui_values="$ui_current|Original UI (stable)"
else ui_values="$ui_current|Centered 16:9 UI (experimental)"; fi
if [[ "$ultrawide" == 1 ]]; then ultrawide_values='Enabled|Disabled'
else ultrawide_values='Disabled|Enabled'; fi
if [[ "$fps_override" == 1 ]]; then fps_override_values='Enabled|Disabled'
else fps_override_values='Disabled|Enabled'; fi

result="$(zenity --forms --title='MGS4 Ultra120 configurator' \
  --text='Changes apply on the next game start. Leave text fields empty to retain their values.' \
  --separator='|' \
  --add-entry="Width (current: $width)" --add-entry="Height (current: $height)" \
  --add-entry="FOV multiplier (current: $fov; 1.000 = original)" \
  --add-combo='Ultrawide module' --combo-values="$ultrawide_values" \
  --add-combo='FPS limit' --combo-values="$fps_values" \
  --add-combo='FPS override module' --combo-values="$fps_override_values" \
  --add-combo='UI mode' --combo-values="$ui_values" \
  --add-combo='Linux fullscreen launch' --combo-values='Keep current Steam options|Gamescope fullscreen|Native/no Gamescope' \
  --add-entry='Gamescope refresh rate (default: 60)' \
  --width=780 --height=560)" || exit 0

IFS='|' read -r new_width new_height new_fov new_ultrawide new_fps new_fps_override new_ui steam_mode refresh <<<"$result"
new_width="${new_width:-$width}"; new_height="${new_height:-$height}"
new_fov="${new_fov:-$fov}"; new_fps="${new_fps:-$fps}"; refresh="${refresh:-60}"
[[ "$new_ui" == *Centered* ]] && ui_value=1 || ui_value=0
[[ "$new_ultrawide" == Enabled ]] && ultrawide_value=1 || ultrawide_value=0
[[ "$new_fps_override" == Enabled ]] && fps_override_value=1 || fps_override_value=0

if [[ "$fps_override_value" == 1 && "$new_fps" == 120 ]]; then
  zenity --question --title='Experimental 120 FPS' \
    --text='120 FPS has reproduced a scripted-intro stall with audio continuing. Enable it anyway?' || exit 0
fi
apply_values "$new_width" "$new_height" "$new_fov" "$new_fps" "$ui_value" "$ultrawide_value" "$fps_override_value"

launch_note='Steam launch options were left unchanged.'
if [[ "$steam_mode" != 'Keep current Steam options' ]]; then
  if pgrep -x steam >/dev/null; then
    launch_note='Steam is running, so launch options were NOT changed. Exit Steam and run the configurator again.'
  elif [[ ! -f "$STATE_FILE" ]]; then
    launch_note='Install the patch first; the reversible Steam-options state file is missing.'
  else
    launch_mode=native
    [[ "$steam_mode" == 'Gamescope fullscreen' ]] && launch_mode=gamescope
    python3 "$SCRIPT_DIR/steam_options.py" configure "$STATE_FILE" \
      "$new_width" "$new_height" "$refresh" "$launch_mode"
    launch_note="Steam launch mode updated: $steam_mode."
  fi
fi

status='supported executable'
show_status >/dev/null 2>&1 || status='UNSUPPORTED executable; the DLL will fail safely'
zenity --info --title='MGS4 Ultra120' \
  --text="Saved: ultrawide ${new_ultrawide}, ${new_width}x${new_height}, FOV ${new_fov}; FPS override ${new_fps_override}, ${new_fps}.\n${launch_note}\nStatus: ${status}."
