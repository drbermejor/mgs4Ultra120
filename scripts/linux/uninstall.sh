#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
GAME_DIR="${MGS4_GAME_DIR:-$HOME/.local/share/Steam/steamapps/common/METAL GEAR SOLID 4/MGS4}"
BACKUP_DIR="$GAME_DIR/.mgs4ultra120-backup"
STATE_FILE="$BACKUP_DIR/steam-options.json"

pgrep -f '[m]gs4.exe' >/dev/null && { echo "Exit the game before uninstalling." >&2; exit 1; }
pgrep -x steam >/dev/null && { echo "Exit Steam completely before uninstalling." >&2; exit 1; }
python3 "$SCRIPT_DIR/steam_options.py" uninstall "$STATE_FILE"

for name in winmm.dll mgs4_ultrawide.ini; do
  if [[ -e "$BACKUP_DIR/$name.preinstall" ]]; then
    mv -- "$BACKUP_DIR/$name.preinstall" "$GAME_DIR/$name"
  else
    rm -f -- "$GAME_DIR/$name"
  fi
done
rmdir -- "$BACKUP_DIR" 2>/dev/null || true
echo "Uninstalled from: $GAME_DIR"
