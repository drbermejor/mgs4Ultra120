#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PACKAGE_DIR="$(cd -- "$SCRIPT_DIR/../.." && pwd)"
GAME_DIR="${MGS4_GAME_DIR:-$HOME/.local/share/Steam/steamapps/common/METAL GEAR SOLID 4/MGS4}"
BACKUP_DIR="$GAME_DIR/.mgs4ultra120-backup"
STATE_FILE="$BACKUP_DIR/steam-options.json"
INSTALL_DIR="$(dirname -- "$GAME_DIR")"
LAUNCHER_TARGET="$INSTALL_DIR/Launcher/launcher.exe"
LAUNCHER_BACKUP="$BACKUP_DIR/launcher.exe.preinstall"
WRAPPER_SOURCE="$PACKAGE_DIR/bin/launcher.exe"
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

remove_owned_shortcut() {
  local target="$1"
  if [[ -f "$target" ]] && grep -q '^X-MGS4Ultra120-Managed=true$' "$target"; then
    rm -f -- "$target"
  fi
}

binary_conflict=0
restore_managed() {
  local target="$1" name="$2"
  local backup="$BACKUP_DIR/$name.preinstall"
  local marker="$BACKUP_DIR/$name.installed.sha256"
  local reuse="$BACKUP_DIR/$name.reused.sha256"
  local current recorded
  if [[ -f "$reuse" ]]; then
    rm -f -- "$reuse"
    return 0
  fi
  if [[ -f "$marker" ]]; then
    if [[ -f "$target" ]]; then
      current="$(sha256sum "$target" | awk '{print $1}')"
      recorded="$(tr -d '[:space:]' <"$marker")"
      if [[ "$current" != "$recorded" ]]; then
        binary_conflict=1
        echo "$name changed outside MGS4 Ultra120; preserving it and its backup." >&2
        return 0
      fi
    fi
    if [[ -e "$backup" ]]; then
      mkdir -p -- "$(dirname -- "$target")"
      mv -- "$backup" "$target"
    else
      rm -f -- "$target"
    fi
    rm -f -- "$marker"
  elif [[ -e "$backup" ]]; then
    # Migrate an older Linux installation that predates ownership markers.
    mkdir -p -- "$(dirname -- "$target")"
    mv -- "$backup" "$target"
  fi
}

pgrep -f '[m]gs4.exe' >/dev/null && { echo "Exit the game before uninstalling." >&2; exit 1; }
pgrep -x steam >/dev/null && { echo "Exit Steam completely before uninstalling." >&2; exit 1; }
python3 "$SCRIPT_DIR/steam_options.py" uninstall "$STATE_FILE"
remove_owned_shortcut "$APPLICATION_SHORTCUT"
remove_owned_shortcut "$(desktop_directory)/MGS4 Ultra120 Configurator.desktop"
if command -v update-desktop-database >/dev/null 2>&1; then
  update-desktop-database "$APPLICATIONS_DIR" >/dev/null 2>&1 || true
fi

launcher_conflict=0
if [[ -e "$LAUNCHER_BACKUP" ]]; then
  if [[ -f "$WRAPPER_SOURCE" && -f "$LAUNCHER_TARGET" ]] &&
     [[ "$(sha256sum "$WRAPPER_SOURCE" | awk '{print $1}')" == "$(sha256sum "$LAUNCHER_TARGET" | awk '{print $1}')" ]]; then
    mv -- "$LAUNCHER_BACKUP" "$LAUNCHER_TARGET"
  else
    launcher_conflict=1
    echo "Launcher changed outside MGS4 Ultra120; preserving it and the launcher backup." >&2
  fi
fi

restore_managed "$GAME_DIR/winmm.dll" winmm.dll
restore_managed "$GAME_DIR/scripts/MGS4Ultra120.asi" MGS4Ultra120.asi
restore_managed "$GAME_DIR/scripts/MGSFPSUnlock.asi" MGSFPSUnlock.asi
for name in mgs4_ultrawide.ini; do
  if [[ -e "$BACKUP_DIR/$name.preinstall" ]]; then
    mv -- "$BACKUP_DIR/$name.preinstall" "$GAME_DIR/$name"
  else
    rm -f -- "$GAME_DIR/$name"
  fi
done
for name in MGSFPSUnlock.ini; do
  if [[ -e "$BACKUP_DIR/$name.preinstall" ]]; then
    mkdir -p -- "$GAME_DIR/scripts"
    mv -- "$BACKUP_DIR/$name.preinstall" "$GAME_DIR/scripts/$name"
  else
    rm -f -- "$GAME_DIR/scripts/$name"
  fi
done
rmdir -- "$GAME_DIR/scripts" 2>/dev/null || true
if [[ "$launcher_conflict" == 0 && "$binary_conflict" == 0 ]]; then
  rmdir -- "$BACKUP_DIR" 2>/dev/null || true
fi
echo "Uninstalled from: $GAME_DIR"
