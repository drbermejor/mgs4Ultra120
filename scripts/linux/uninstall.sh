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

pgrep -f '[m]gs4.exe' >/dev/null && { echo "Exit the game before uninstalling." >&2; exit 1; }
pgrep -x steam >/dev/null && { echo "Exit Steam completely before uninstalling." >&2; exit 1; }
python3 "$SCRIPT_DIR/steam_options.py" uninstall "$STATE_FILE"

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

for name in winmm.dll mgs4_ultrawide.ini; do
  if [[ -e "$BACKUP_DIR/$name.preinstall" ]]; then
    mv -- "$BACKUP_DIR/$name.preinstall" "$GAME_DIR/$name"
  else
    rm -f -- "$GAME_DIR/$name"
  fi
done
if [[ "$launcher_conflict" == 0 ]]; then
  rmdir -- "$BACKUP_DIR" 2>/dev/null || true
fi
echo "Uninstalled from: $GAME_DIR"
