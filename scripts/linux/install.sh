#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PACKAGE_DIR="$(cd -- "$SCRIPT_DIR/../.." && pwd)"
VERSION="$(tr -d '\r\n' <"$PACKAGE_DIR/VERSION")"
[[ -n "$VERSION" ]] || { echo "Package VERSION is empty." >&2; exit 1; }
GAME_DIR="${MGS4_GAME_DIR:-$HOME/.local/share/Steam/steamapps/common/METAL GEAR SOLID 4/MGS4}"
BACKUP_DIR="$GAME_DIR/.mgs4ultra120-backup"
STATE_FILE="$BACKUP_DIR/steam-options.json"
CORE_ASI_SOURCE="$PACKAGE_DIR/bin/MGS4Ultra120.asi"
HUD_ASI_SOURCE="$PACKAGE_DIR/bin/MGS4CenteredHUD16x9.asi"
HUD_INI_SOURCE="$PACKAGE_DIR/config/mgs4_centered_hud_16x9.ini"
FPS_ASI_BUNDLED="$PACKAGE_DIR/optional/MGSFPSUnlock.asi"
FPS_INI_SOURCE="$PACKAGE_DIR/config/MGSFPSUnlock.ini"
FPS_PREPARER="$SCRIPT_DIR/prepare_fpsunlock.py"
INSTALL_FPS="${MGS4_INSTALL_FPS_UNLOCK:-1}"
INSTALL_DIR="$(dirname -- "$GAME_DIR")"
LAUNCHER_TARGET="$INSTALL_DIR/Launcher/launcher.exe"
LAUNCHER_BACKUP="$BACKUP_DIR/launcher.exe.preinstall"
WRAPPER_SOURCE="$PACKAGE_DIR/bin/launcher.exe"
FPS_STAGE=""
APPLICATIONS_DIR="${XDG_DATA_HOME:-$HOME/.local/share}/applications"
APPLICATION_SHORTCUT="$APPLICATIONS_DIR/mgs4ultra120-configure.desktop"

desktop_directory() {
  local detected=""
  if [[ -n "${MGS4_DESKTOP_DIR:-}" ]]; then
    printf '%s\n' "$MGS4_DESKTOP_DIR"
    return
  fi
  if command -v xdg-user-dir >/dev/null 2>&1; then
    detected="$(xdg-user-dir DESKTOP 2>/dev/null || true)"
  fi
  if [[ -n "$detected" && "$detected" != "$HOME" ]]; then
    printf '%s\n' "$detected"
  elif [[ -d "$HOME/Escritorio" ]]; then
    printf '%s\n' "$HOME/Escritorio"
  else
    printf '%s\n' "$HOME/Desktop"
  fi
}

install_configurator_shortcut() {
  local target="$1" target_dir
  target_dir="$(dirname -- "$target")"
  mkdir -p -- "$target_dir"
  if [[ -e "$target" ]] && ! grep -q '^X-MGS4Ultra120-Managed=true$' "$target"; then
    echo "Shortcut already exists and is not managed by MGS4 Ultra120; preserving it: $target" >&2
    return
  fi
  printf '%s\n' \
    '[Desktop Entry]' \
    'Type=Application' \
    'Version=1.0' \
    "Name=MGS4 Ultra120 $VERSION Configurator" \
    'Comment=Configure the MGS4 Ultra120 patch' \
    "Exec=\"$PACKAGE_DIR/MGS4Ultra120-Linux-Configure.sh\"" \
    'Icon=applications-games' \
    'Terminal=false' \
    'Categories=Game;' \
    'StartupNotify=true' \
    'X-MGS4Ultra120-Managed=true' >"$target"
  chmod 0755 "$target"
}

cleanup() {
  [[ -z "$FPS_STAGE" || ! -e "$FPS_STAGE" ]] || rm -f -- "$FPS_STAGE"
}
trap cleanup EXIT

managed_preflight() {
  local target="$1" name="$2" source="$3"
  local marker="$BACKUP_DIR/$name.installed.sha256"
  local reuse="$BACKUP_DIR/$name.reused.sha256"
  local current recorded source_hash
  source_hash="$(sha256sum "$source" | awk '{print $1}')"
  if [[ -f "$marker" ]]; then
    [[ -f "$target" ]] || { echo "Managed $name is missing; refusing to overwrite its backup state." >&2; return 1; }
    current="$(sha256sum "$target" | awk '{print $1}')"
    recorded="$(tr -d '[:space:]' <"$marker")"
    [[ "$current" == "$recorded" ]] || { echo "$name changed outside MGS4 Ultra120; refusing to overwrite it." >&2; return 1; }
  elif [[ -f "$reuse" ]]; then
    [[ -f "$target" ]] || { echo "Reused $name is missing; remove stale backup markers before retrying." >&2; return 1; }
    current="$(sha256sum "$target" | awk '{print $1}')"
    recorded="$(tr -d '[:space:]' <"$reuse")"
    [[ "$current" == "$recorded" ]] || { echo "Reused $name changed outside MGS4 Ultra120; leaving it untouched." >&2; return 1; }
    [[ "$current" == "$source_hash" ]] || { echo "The package loader/plugin differs from reused $name; uninstall before upgrading." >&2; return 1; }
  fi
}

install_managed() {
  local source="$1" target="$2" name="$3"
  local backup="$BACKUP_DIR/$name.preinstall"
  local marker="$BACKUP_DIR/$name.installed.sha256"
  local reuse="$BACKUP_DIR/$name.reused.sha256"
  local source_hash target_hash
  source_hash="$(sha256sum "$source" | awk '{print $1}')"
  if [[ -f "$reuse" ]]; then
    return 0
  fi
  if [[ ! -f "$marker" && -f "$target" ]]; then
    target_hash="$(sha256sum "$target" | awk '{print $1}')"
    if [[ "$target_hash" == "$source_hash" ]]; then
      printf '%s\n' "$target_hash" >"$reuse"
      return 0
    fi
    [[ -e "$backup" ]] || cp -a -- "$target" "$backup"
  fi
  install -m0644 "$source" "$target"
  printf '%s\n' "$source_hash" >"$marker"
}

install_patch_config() {
  local template="$PACKAGE_DIR/config/mgs4_ultrawide.ini"
  local destination="$GAME_DIR/mgs4_ultrawide.ini"
  if [[ ! -f "$destination" ]] || ! grep -q '^\[Patch\]$' "$destination"; then
    install -m0644 "$template" "$destination"
    return
  fi
  python3 - "$template" "$destination" <<'PY'
import math
import re
import sys
from pathlib import Path

template_path, destination = map(Path, sys.argv[1:])
text = template_path.read_text()
existing = destination.read_text()
preserved = (
    "UltrawideEnabled", "FPSOverrideEnabled", "AllowUnsupportedExecutable",
    "Width", "Height", "FOVMultiplier", "NativeCameraFOV",
    "ExperimentalCinematicFOV", "CinematicFOVMultiplier",
    "SupersamplingEnabled", "RenderScale", "Limit",
    "ControllerProfileFixEnabled", "SkipUnityLauncher", "Region",
    "SelfRegion", "Language", "ControllerType", "DisplayMode",
    "UsePrimaryPhysicalResolution",
)
for key in preserved:
    match = re.search(rf"(?m)^{re.escape(key)}=(.*)$", existing)
    if match and re.search(rf"(?m)^{re.escape(key)}=.*$", text):
        text = re.sub(rf"(?m)^{re.escape(key)}=.*$",
                      f"{key}={match.group(1).strip()}", text, count=1)

text = re.sub(r"(?m)^FPSOverrideEnabled=.*$", "FPSOverrideEnabled=0", text, count=1)
text = re.sub(r"(?m)^Limit=120$", "Limit=60", text, count=1)
model = re.search(r"(?m)^FOVModelVersion=(\d+)\s*$", existing)
old_model = model is None or int(model.group(1)) < 2
fov = re.search(r"(?m)^FOVMultiplier=(.*)$", text)
try:
    parsed_fov = float(fov.group(1).strip().replace(",", ".")) if fov else 1.2
except ValueError:
    parsed_fov = 1.2
if not math.isfinite(parsed_fov) or parsed_fov < 0.5 or (old_model and abs(parsed_fov - 1.05) < 0.0005):
    text = re.sub(r"(?m)^FOVMultiplier=.*$", "FOVMultiplier=1.200", text, count=1)

temporary = destination.with_suffix(".ini.tmp")
temporary.write_text(text)
temporary.replace(destination)
PY
  chmod 0644 "$destination"
}

install_hud_config() {
  local destination="$GAME_DIR/mgs4_centered_hud_16x9.ini"
  python3 - "$HUD_INI_SOURCE" "$destination" "$GAME_DIR/mgs4_ultrawide.ini" <<'PY'
import re
import sys
from pathlib import Path

template_path, destination, main_path = map(Path, sys.argv[1:])
text = template_path.read_text()
enabled = "0"
if destination.is_file():
    existing = destination.read_text()
    recognized = (re.search(r"(?m)^\[MGS4Ultra120HUD\]\s*$", existing) or
                  (re.search(r"(?m)^\[Lab\]\s*$", existing) and
                   re.search(r"(?m)^Mode=CenteredHUD16x9Preview1\s*$", existing)))
    match = re.search(r"(?m)^Enabled=(0|1)\s*$", existing)
    if recognized and match:
        enabled = match.group(1)
main = main_path.read_text()
width = re.search(r"(?m)^Width=(\d+)\s*$", main)
height = re.search(r"(?m)^Height=(\d+)\s*$", main)
if not width or not height:
    raise SystemExit("Installed Width/Height are missing or invalid")
for key, value in (("Enabled", enabled), ("Width", width.group(1)),
                   ("Height", height.group(1))):
    text = re.sub(rf"(?m)^{key}=.*$", f"{key}={value}", text, count=1)
temporary = destination.with_suffix(".ini.tmp")
temporary.write_text(text)
temporary.replace(destination)
PY
  chmod 0644 "$destination"
}

[[ "$INSTALL_FPS" == 0 || "$INSTALL_FPS" == 1 ]] || {
  echo "MGS4_INSTALL_FPS_UNLOCK must be 0 or 1." >&2
  exit 1
}
[[ -f "$GAME_DIR/mgs4.exe" ]] || { echo "mgs4.exe not found in: $GAME_DIR" >&2; exit 1; }
[[ -f "$PACKAGE_DIR/bin/winmm.dll" ]] || { echo "Ultimate ASI Loader not found in: $PACKAGE_DIR/bin" >&2; exit 1; }
[[ -f "$CORE_ASI_SOURCE" ]] || { echo "MGS4Ultra120.asi not found in: $PACKAGE_DIR/bin" >&2; exit 1; }
[[ -f "$HUD_ASI_SOURCE" ]] || { echo "MGS4CenteredHUD16x9.asi not found in: $PACKAGE_DIR/bin" >&2; exit 1; }
[[ -f "$HUD_INI_SOURCE" ]] || { echo "Centered-HUD configuration is missing." >&2; exit 1; }
[[ -f "$PACKAGE_DIR/config/mgs4_ultrawide.ini" ]] || { echo "Patch configuration is missing." >&2; exit 1; }
[[ -f "$WRAPPER_SOURCE" ]] || { echo "Direct-launch wrapper is missing." >&2; exit 1; }
pgrep -f '[m]gs4.exe' >/dev/null && { echo "Exit the game before installing." >&2; exit 1; }
pgrep -x steam >/dev/null && { echo "Exit Steam completely before installing." >&2; exit 1; }

# Public packages do not redistribute MGSFPSUnlock. Download its official
# release, verify both upstream hashes and apply the one-byte Wine adaptation
# locally before changing the game. Offline integrators may provide the already
# verified result in optional/MGSFPSUnlock.asi.
if [[ "$INSTALL_FPS" == 1 ]]; then
  [[ -f "$FPS_INI_SOURCE" && -x "$FPS_PREPARER" ]] || {
    echo "FPS setup files are missing from the Linux package." >&2
    exit 1
  }
  FPS_STAGE="$(mktemp -t MGSFPSUnlock-Proton-XXXXXX.asi)"
  rm -f -- "$FPS_STAGE"
  if [[ -f "$FPS_ASI_BUNDLED" ]]; then
    install -m0644 "$FPS_ASI_BUNDLED" "$FPS_STAGE"
  fi
  fps_arguments=(--output "$FPS_STAGE")
  if [[ -n "${MGS4_FPS_UNLOCK_ARCHIVE:-}" ]]; then
    fps_arguments+=(--archive "$MGS4_FPS_UNLOCK_ARCHIVE")
  fi
  python3 "$FPS_PREPARER" "${fps_arguments[@]}"
fi

mkdir -p -- "$BACKUP_DIR" "$GAME_DIR/scripts"
managed_preflight "$GAME_DIR/winmm.dll" winmm.dll "$PACKAGE_DIR/bin/winmm.dll"
managed_preflight "$GAME_DIR/scripts/MGS4Ultra120.asi" MGS4Ultra120.asi "$CORE_ASI_SOURCE"
managed_preflight "$GAME_DIR/scripts/MGS4CenteredHUD16x9.asi" MGS4CenteredHUD16x9.asi "$HUD_ASI_SOURCE"
if [[ "$INSTALL_FPS" == 1 ]]; then
  managed_preflight "$GAME_DIR/scripts/MGSFPSUnlock.asi" MGSFPSUnlock.asi "$FPS_STAGE"
fi

if [[ -e "$GAME_DIR/mgs4_ultrawide.ini" &&
      ! -e "$BACKUP_DIR/mgs4_ultrawide.ini.preinstall" &&
      ! -e "$BACKUP_DIR/MGS4Ultra120.asi.installed.sha256" &&
      ! -e "$BACKUP_DIR/MGS4Ultra120.asi.reused.sha256" ]]; then
  cp -a -- "$GAME_DIR/mgs4_ultrawide.ini" "$BACKUP_DIR/mgs4_ultrawide.ini.preinstall"
fi
if [[ -e "$GAME_DIR/mgs4_centered_hud_16x9.ini" &&
      ! -e "$BACKUP_DIR/mgs4_centered_hud_16x9.ini.preinstall" &&
      ! -e "$BACKUP_DIR/MGS4CenteredHUD16x9.asi.installed.sha256" &&
      ! -e "$BACKUP_DIR/MGS4CenteredHUD16x9.asi.reused.sha256" ]] &&
   ! grep -q '^\[MGS4Ultra120HUD\]$' "$GAME_DIR/mgs4_centered_hud_16x9.ini" &&
   ! grep -q '^Mode=CenteredHUD16x9Preview1$' "$GAME_DIR/mgs4_centered_hud_16x9.ini"; then
  cp -a -- "$GAME_DIR/mgs4_centered_hud_16x9.ini" \
    "$BACKUP_DIR/mgs4_centered_hud_16x9.ini.preinstall"
fi
if [[ "$INSTALL_FPS" == 1 ]]; then
  if [[ -e "$GAME_DIR/scripts/MGSFPSUnlock.ini" &&
        ! -e "$BACKUP_DIR/MGSFPSUnlock.ini.preinstall" &&
        ! -e "$BACKUP_DIR/MGSFPSUnlock.asi.installed.sha256" &&
        ! -e "$BACKUP_DIR/MGSFPSUnlock.asi.reused.sha256" ]]; then
    cp -a -- "$GAME_DIR/scripts/MGSFPSUnlock.ini" "$BACKUP_DIR/MGSFPSUnlock.ini.preinstall"
  fi
fi

install_managed "$PACKAGE_DIR/bin/winmm.dll" "$GAME_DIR/winmm.dll" winmm.dll
install_managed "$CORE_ASI_SOURCE" "$GAME_DIR/scripts/MGS4Ultra120.asi" MGS4Ultra120.asi
install_managed "$HUD_ASI_SOURCE" "$GAME_DIR/scripts/MGS4CenteredHUD16x9.asi" MGS4CenteredHUD16x9.asi
install_patch_config
install_hud_config
if [[ "$INSTALL_FPS" == 1 ]]; then
  install_managed "$FPS_STAGE" "$GAME_DIR/scripts/MGSFPSUnlock.asi" MGSFPSUnlock.asi
  install -m0644 "$FPS_INI_SOURCE" "$GAME_DIR/scripts/MGSFPSUnlock.ini"
fi

if [[ "$(sed -n 's/^SkipUnityLauncher=//p' "$GAME_DIR/mgs4_ultrawide.ini" | tr -d '\r' | head -n1)" == 1 ]]; then
  [[ -f "$LAUNCHER_TARGET" ]] || {
    echo "Unity launcher not found in: $(dirname -- "$LAUNCHER_TARGET")" >&2
    exit 1
  }
  if [[ ! -e "$LAUNCHER_BACKUP" ]]; then
    cp -a -- "$LAUNCHER_TARGET" "$LAUNCHER_BACKUP"
  fi
  install -m0755 "$WRAPPER_SOURCE" "$LAUNCHER_TARGET"
fi

python3 "$SCRIPT_DIR/steam_options.py" install "$STATE_FILE"
DESKTOP_SHORTCUT="$(desktop_directory)/MGS4 Ultra120 Configurator.desktop"
install_configurator_shortcut "$APPLICATION_SHORTCUT"
install_configurator_shortcut "$DESKTOP_SHORTCUT"
if command -v update-desktop-database >/dev/null 2>&1; then
  update-desktop-database "$APPLICATIONS_DIR" >/dev/null 2>&1 || true
fi
echo "Installed in: $GAME_DIR"
if [[ "$INSTALL_FPS" == 1 ]]; then
  echo "Corrected 30/60/120 FPS component: installed (official 0.1.0, adapted locally for Proton)."
else
  echo "Corrected FPS component: not installed; the game keeps its normal FPS behavior."
fi
echo "Run scripts/linux/configure.sh gui to choose resolution, gameplay/cinematic FOV, centered HUD, supersampling and fullscreen mode."
echo "Configurator shortcut: $DESKTOP_SHORTCUT"
echo "Launch through Steam; DirectX 12 is the validated path."
