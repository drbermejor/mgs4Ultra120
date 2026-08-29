#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PACKAGE_DIR="$(cd -- "$SCRIPT_DIR/../.." && pwd)"
GAME_DIR_RESOLVER="$SCRIPT_DIR/game_dir.py"
[[ -f "$GAME_DIR_RESOLVER" ]] || {
  echo "Linux game-directory resolver is missing: $GAME_DIR_RESOLVER" >&2
  exit 1
}
GAME_DIR="$(python3 "$GAME_DIR_RESOLVER" resolve --interactive)"
export MGS4_GAME_DIR="$GAME_DIR"
INI="$GAME_DIR/mgs4_ultrawide.ini"
HUD_INI="$GAME_DIR/mgs4_centered_hud_16x9.ini"
FPS_INI="$GAME_DIR/scripts/MGSFPSUnlock.ini"
BACKUP_DIR="$GAME_DIR/.mgs4ultra120-backup"
STATE_FILE="$BACKUP_DIR/steam-options.json"
INSTALL_DIR="$(dirname -- "$GAME_DIR")"
LAUNCHER_DIR="$INSTALL_DIR/Launcher"
LAUNCHER_TARGET="$LAUNCHER_DIR/launcher.exe"
LAUNCHER_BACKUP="$BACKUP_DIR/launcher.exe.preinstall"
WRAPPER_SOURCE="$PACKAGE_DIR/bin/launcher.exe"
KNOWN_EXE_SHA256="9e8df67ea7f41e7f8306ce1a77584707209069b3c75389b3f00445efe459fe41"
MODE="${1:-gui}"
VERSION="$(tr -d '\r\n' <"$PACKAGE_DIR/VERSION")"
[[ -n "$VERSION" ]] || { echo "ERROR: Package VERSION is empty." >&2; exit 1; }

die() { echo "ERROR: $*" >&2; exit 1; }
ini_value() { sed -n "s/^$1=//p" "$INI" | tr -d '\r' | head -n1; }
game_running() {
  pgrep -x 'mgs4.exe' >/dev/null || pgrep -f -- '^-region eu -lan ' >/dev/null
}

[[ -f "$INI" ]] || die "mgs4_ultrawide.ini not found in: $GAME_DIR"
[[ -f "$HUD_INI" ]] || die "mgs4_centered_hud_16x9.ini not found in: $GAME_DIR; reinstall the current patch"
[[ -f "$GAME_DIR/mgs4.exe" ]] || die "mgs4.exe not found in: $GAME_DIR"
game_running && die "Exit the game before changing settings."

apply_values() {
  local width="$1" height="$2" fov="$3" native_fov="$4" cinematic_fov="$5" cinematic_multiplier="$6" ultrawide="$7" controller_fix="$8" skip_launcher="$9" language="${10}" allow_unsupported="${11}" supersampling="${12}" render_scale="${13}" hud_enabled="${14}" fps_target="${15}"
  python3 - "$INI" "$FPS_INI" "$HUD_INI" "$width" "$height" "$fov" "$native_fov" "$cinematic_fov" "$cinematic_multiplier" "$ultrawide" "$controller_fix" "$skip_launcher" "$language" "$allow_unsupported" "$supersampling" "$render_scale" "$hud_enabled" "$fps_target" <<'PY'
import math
import re
import sys
from pathlib import Path

path = Path(sys.argv[1])
fps_path = Path(sys.argv[2])
hud_path = Path(sys.argv[3])
width, height, fov, native_fov, cinematic_fov, cinematic_multiplier, ultrawide, controller_fix, skip_launcher, language, allow_unsupported, supersampling, render_scale, hud_enabled, fps_target = sys.argv[4:]
try:
    width_i, height_i = int(width), int(height)
    fov_f = float(fov.replace(",", "."))
    render_scale_f = float(render_scale.replace(",", "."))
except ValueError as error:
    raise SystemExit(f"Invalid numeric value: {error}")
if not (640 <= width_i <= 16384 and 480 <= height_i <= 16384):
    raise SystemExit("Width/height are outside the allowed range")
if fov_f < 0.5 or not math.isfinite(fov_f):
    raise SystemExit("FOV multiplier must be finite and at least 0.5")
if not (1.0 <= render_scale_f <= 8.0) or not math.isfinite(render_scale_f):
    raise SystemExit("Supersampling scale must be between 1.0 and 8.0")
if supersampling not in ("0", "1"):
    raise SystemExit("Invalid supersampling state")
if native_fov not in ("0", "1"):
    raise SystemExit("Invalid experimental native FOV state")
if cinematic_fov not in ("0", "1"):
    raise SystemExit("Invalid experimental cinematic FOV state")
if cinematic_fov == "1" and native_fov != "1":
    raise SystemExit("Experimental cinematic FOV requires native FOV correction")
if hud_enabled not in ("0", "1"):
    raise SystemExit("Invalid experimental centered-HUD state")
cinematic_multiplier = cinematic_multiplier.strip().lower().replace(",", ".")
if cinematic_multiplier != "inherit":
    try:
        cinematic_multiplier_f = float(cinematic_multiplier)
    except ValueError as error:
        raise SystemExit(f"Invalid cinematic FOV multiplier: {error}")
    if cinematic_multiplier_f < 0.5 or not math.isfinite(cinematic_multiplier_f):
        raise SystemExit("Cinematic FOV multiplier must be 'inherit' or a finite value of at least 0.5")
    cinematic_multiplier = f"{cinematic_multiplier_f:.3f}"
if fps_target not in ("30", "60", "120"):
    raise SystemExit("FPS target must be 30, 60 or 120")
if language == "ge":
    language = "gr"
if language not in ("en", "sp", "fr", "it", "gr", "jp", "pt"):
    raise SystemExit("Unsupported direct-launch language")
values = {
    "Width": str(width_i), "Height": str(height_i),
    "FOVMultiplier": f"{fov_f:.3f}", "Limit": "60",
    "NativeCameraFOV": native_fov,
    "ExperimentalCinematicFOV": cinematic_fov,
    "CinematicFOVMultiplier": cinematic_multiplier,
    "UltrawideEnabled": ultrawide,
    "FPSOverrideEnabled": "0",
    "ControllerProfileFixEnabled": controller_fix,
    "SkipUnityLauncher": skip_launcher,
    "Language": language,
    "AllowUnsupportedExecutable": allow_unsupported,
    "SupersamplingEnabled": supersampling,
    "RenderScale": f"{render_scale_f:.3f}",
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
hud_text = hud_path.read_text()
for key, value in (("Enabled", hud_enabled), ("Width", str(width_i)),
                   ("Height", str(height_i))):
    pattern = re.compile(rf"(?m)^{re.escape(key)}=.*$")
    if not pattern.search(hud_text):
        raise SystemExit(f"Missing {key} in {hud_path}; reinstall the current patch")
    hud_text = pattern.sub(f"{key}={value}", hud_text, count=1)
hud_temporary = hud_path.with_suffix(".ini.tmp")
hud_temporary.write_text(hud_text)
hud_temporary.replace(hud_path)
if fps_path.is_file():
    fps_text = fps_path.read_text()
    fps_pattern = re.compile(r"(?m)^\s*TargetFrameRate\s*=.*$")
    if not fps_pattern.search(fps_text):
        raise SystemExit(f"Missing TargetFrameRate in {fps_path}")
    fps_text = fps_pattern.sub(f"TargetFrameRate = {fps_target}", fps_text, count=1)
    fps_temporary = fps_path.with_suffix(".ini.tmp")
    fps_temporary.write_text(fps_text)
    fps_temporary.replace(fps_path)
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

detect_primary_mode() {
  command -v xrandr >/dev/null 2>&1 || return 1
  local output dimensions refresh
  output="$(xrandr --current 2>/dev/null)" || return 1
  dimensions="$(awk '
    / connected primary / {
      if (match($0, /[0-9]+x[0-9]+\+[0-9]+\+[0-9]+/)) {
        value=substr($0, RSTART, RLENGTH); split(value, parts, /[x+]/)
        print parts[1], parts[2]; exit
      }
    }' <<<"$output")"
  [[ -n "$dimensions" ]] || return 1
  refresh="$(awk '
    / connected primary / { active=1; next }
    active && /^[^[:space:]]/ { active=0 }
    active && /\*/ { value=$2; gsub(/[^0-9.].*$/, "", value); printf "%.0f", value; exit }
  ' <<<"$output")"
  [[ -n "$refresh" ]] || refresh=60
  printf '%s %s\n' "$dimensions" "$refresh"
}

current_skip="$(ini_value SkipUnityLauncher)"
current_language="$(ini_value Language)"
current_allow_unsupported="$(ini_value AllowUnsupportedExecutable)"
current_supersampling="$(ini_value SupersamplingEnabled)"
current_render_scale="$(ini_value RenderScale)"
current_hud="$(sed -n 's/^Enabled=//p' "$HUD_INI" | tr -d '\r' | head -n1)"
fps_installed=0
[[ -f "$GAME_DIR/scripts/MGSFPSUnlock.asi" && -f "$FPS_INI" ]] && fps_installed=1
current_fps_target=""
if [[ -f "$FPS_INI" ]]; then
  current_fps_target="$(sed -n 's/^[[:space:]]*TargetFrameRate[[:space:]]*=[[:space:]]*//p' "$FPS_INI" | tr -d '\r' | head -n1)"
fi
current_fps_target="${current_fps_target:-60}"
detected_width=""; detected_height=""; detected_refresh=60
if detected_mode="$(detect_primary_mode)"; then
  read -r detected_width detected_height detected_refresh <<<"$detected_mode"
fi
case "$MODE" in
  detected)
    [[ -n "$detected_width" && -n "$detected_height" ]] || die "Could not detect the primary physical display mode."
    apply_values "$detected_width" "$detected_height" 1.000 1 0 inherit 1 1 "$current_skip" "$current_language" "$current_allow_unsupported" 0 "$current_render_scale" 0 "$current_fps_target"
    exit 0
    ;;
  stable) apply_values 3440 1440 1.200 1 0 inherit 1 1 "$current_skip" "$current_language" "$current_allow_unsupported" 0 "$current_render_scale" 0 "$current_fps_target"; exit 0 ;;
  16x9-supersampling) apply_values 1920 1080 1.000 0 0 inherit 0 1 "$current_skip" "$current_language" "$current_allow_unsupported" 1 2.000 0 "$current_fps_target"; exit 0 ;;
  ultrawide-only) apply_values 3440 1440 1.200 1 0 inherit 1 0 "$current_skip" "$current_language" "$current_allow_unsupported" 0 "$current_render_scale" 0 "$current_fps_target"; exit 0 ;;
  controller-fix-only) apply_values 3440 1440 1.000 0 0 inherit 0 1 "$current_skip" "$current_language" "$current_allow_unsupported" 0 "$current_render_scale" 0 "$current_fps_target"; exit 0 ;;
  status) show_status; exit $? ;;
  gui) ;;
  *) die "Usage: $0 [gui|detected|stable|16x9-supersampling|ultrawide-only|controller-fix-only|status]" ;;
esac

command -v zenity >/dev/null || die "zenity is required for GUI mode."

width="$(ini_value Width)"; height="$(ini_value Height)"
fov="$(ini_value FOVMultiplier)"
native_fov="$(ini_value NativeCameraFOV)"
cinematic_fov="$(ini_value ExperimentalCinematicFOV)"
cinematic_multiplier="$(ini_value CinematicFOVMultiplier)"
ultrawide="$(ini_value UltrawideEnabled)"
controller_fix="$(ini_value ControllerProfileFixEnabled)"
skip_launcher="$(ini_value SkipUnityLauncher)"; language="$(ini_value Language)"
allow_unsupported="$(ini_value AllowUnsupportedExecutable)"
supersampling="$current_supersampling"; render_scale="$current_render_scale"
fps_target="$current_fps_target"
if [[ "$ultrawide" == 1 ]]; then ultrawide_values='Enabled|Disabled'
else ultrawide_values='Disabled|Enabled'; fi
if [[ "$native_fov" == 1 ]]; then native_fov_values='Enabled (experimental)|Disabled (original vertical FOV)'
else native_fov_values='Disabled (original vertical FOV)|Enabled (experimental)'; fi
if [[ "$cinematic_fov" == 1 ]]; then cinematic_fov_values='Enabled (experimental)|Disabled'
else cinematic_fov_values='Disabled|Enabled (experimental)'; fi
if [[ "$current_hud" == 1 ]]; then hud_values='Enabled (experimental, DX12)|Disabled'
else hud_values='Disabled|Enabled (experimental, DX12)'; fi
if [[ "$controller_fix" == 1 ]]; then controller_values='Enabled (recommended)|Disabled'
else controller_values='Disabled|Enabled (recommended)'; fi
if [[ "$supersampling" == 1 ]]; then supersampling_values='Enabled (experimental)|Disabled'
else supersampling_values='Disabled|Enabled (experimental)'; fi
fps_values="$fps_target"
for value in 120 60 30; do [[ "|$fps_values|" == *"|$value|"* ]] || fps_values+="|$value"; done
if [[ "$fps_installed" == 1 ]]; then
  fps_label='Corrected FPS target (MGSFPSUnlock installed)'
else
  fps_label='FPS target (component not installed; re-run setup to add it)'
fi
if [[ "$skip_launcher" == 1 ]]; then launcher_values='Skip Unity launcher|Use original Unity launcher'
else launcher_values='Use original Unity launcher|Skip Unity launcher'; fi
if [[ "$allow_unsupported" == 1 ]]; then unsupported_values='Attempt unsupported executable (unsafe)|Block unsupported executable'
else unsupported_values='Block unsupported executable|Attempt unsupported executable (unsafe)'; fi
language_values="$language"
[[ "$language_values" == ge ]] && language_values=gr
for value in en sp fr it gr jp pt; do [[ "|$language_values|" == *"|$value|"* ]] || language_values+="|$value"; done
result="$(zenity --forms --title="MGS4 Ultra120 $VERSION configurator" \
  --text='Changes apply on the next game start. Leave text fields empty to retain their values.' \
  --separator='|' \
  --add-entry="Width (current: $width)" --add-entry="Height (current: $height)" \
  --add-entry="FOV (current: $fov; 1.200 tested 21:9 recommendation)" \
  --add-combo='Native FOV correction' --combo-values="$native_fov_values" \
  --add-combo='Real-time cinematic FOV' --combo-values="$cinematic_fov_values" \
  --add-entry="Cinematic FOV (current: $cinematic_multiplier; use inherit or a number)" \
  --add-combo='Centered 16:9 HUD' --combo-values="$hud_values" \
  --add-combo='Ultrawide module' --combo-values="$ultrawide_values" \
  --add-combo='Supersampling' --combo-values="$supersampling_values" \
  --add-entry="Render scale (current: $render_scale; 1.15 keeps 3440 output below 4096 internal)" \
  --add-combo="$fps_label" --combo-values="$fps_values" \
  --add-combo='Controller profile fix' --combo-values="$controller_values" \
  --add-combo='Steam launch path' --combo-values="$launcher_values" \
  --add-combo='Game language' --combo-values="$language_values" \
  --add-combo='Unknown executable policy' --combo-values="$unsupported_values" \
  --add-combo='Linux fullscreen launch' --combo-values='Keep current Steam options|Gamescope fullscreen|Native/no Gamescope' \
  --add-entry="Gamescope refresh rate (detected/default: $detected_refresh)" \
  --width=860 --height=900)" || exit 0

IFS='|' read -r new_width new_height new_fov new_native_fov new_cinematic_fov new_cinematic_multiplier new_hud new_ultrawide new_supersampling new_render_scale new_fps new_controller new_launcher new_language new_unsupported steam_mode refresh <<<"$result"
new_width="${new_width:-$width}"; new_height="${new_height:-$height}"
new_fov="${new_fov:-$fov}"; new_render_scale="${new_render_scale:-$render_scale}"
new_cinematic_multiplier="${new_cinematic_multiplier:-$cinematic_multiplier}"
new_fps="${new_fps:-$fps_target}"; refresh="${refresh:-$detected_refresh}"
[[ "$new_ultrawide" == Enabled ]] && ultrawide_value=1 || ultrawide_value=0
[[ "$new_native_fov" == Enabled* ]] && native_fov_value=1 || native_fov_value=0
[[ "$new_cinematic_fov" == Enabled* ]] && cinematic_fov_value=1 || cinematic_fov_value=0
[[ "$new_hud" == Enabled* ]] && hud_value=1 || hud_value=0
[[ "$new_supersampling" == Enabled* ]] && supersampling_value=1 || supersampling_value=0
[[ "$new_controller" == Enabled* ]] && controller_value=1 || controller_value=0
[[ "$new_launcher" == 'Skip Unity launcher' ]] && launcher_value=1 || launcher_value=0
[[ "$new_unsupported" == Attempt* ]] && unsupported_value=1 || unsupported_value=0

if [[ "$unsupported_value" == 1 ]]; then
  zenity --question --title='Unsupported executable override' \
    --text='Known code/data offsets will be attempted on an unverified executable. This may crash the game or corrupt its process state. Continue under your responsibility?' || exit 0
fi

if [[ "$supersampling_value" == 1 ]]; then
  internal="$(python3 - "$new_width" "$new_height" "$new_render_scale" <<'PY'
import sys
w, h, scale = int(sys.argv[1]), int(sys.argv[2]), float(sys.argv[3].replace(',', '.'))
print(f"{round(w * scale)}x{round(h * scale)}")
PY
)"
  zenity --question --title='Experimental supersampling' \
    --text="Internal render size will be ${internal}. Widths of 4096 or more can make the aiming reticle flicker or disappear. Continue?" || exit 0
fi

if [[ "$cinematic_fov_value" == 1 || "$hud_value" == 1 ]]; then
  zenity --question --title='Experimental rendering options' \
    --text='Cinematic FOV can reveal actors or animation transitions earlier than intended. The centered HUD is DX12-only and some menus or text may still be misplaced. If a problem appears, close the game and disable the affected option to return to the reference behavior. Continue?' || exit 0
fi

set_launcher_wrapper "$launcher_value"
apply_values "$new_width" "$new_height" "$new_fov" "$native_fov_value" "$cinematic_fov_value" "$new_cinematic_multiplier" "$ultrawide_value" "$controller_value" "$launcher_value" "$new_language" "$unsupported_value" "$supersampling_value" "$new_render_scale" "$hud_value" "$new_fps"

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
if [[ "$fps_installed" == 1 ]]; then fps_note="corrected FPS ${new_fps}"
else fps_note='corrected FPS component not installed'; fi
zenity --info --title="MGS4 Ultra120 $VERSION" \
  --text="Saved: ultrawide ${new_ultrawide}, ${new_width}x${new_height}, gameplay FOV ${new_fov}; cinematic FOV ${new_cinematic_fov} (${new_cinematic_multiplier}); centered HUD ${new_hud}; supersampling ${new_supersampling} at ${new_render_scale}x; ${fps_note}; controller fix ${new_controller}; ${new_launcher}.\n${launch_note}\nStatus: ${status}."
