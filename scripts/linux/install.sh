#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PACKAGE_DIR="$(cd -- "$SCRIPT_DIR/../.." && pwd)"
GAME_DIR="${MGS4_GAME_DIR:-$HOME/.local/share/Steam/steamapps/common/METAL GEAR SOLID 4/MGS4}"
BACKUP_DIR="$GAME_DIR/.mgs4ultra120-backup"
STATE_FILE="$BACKUP_DIR/steam-options.json"

[[ -f "$GAME_DIR/mgs4.exe" ]] || { echo "mgs4.exe not found in: $GAME_DIR" >&2; exit 1; }
[[ -f "$PACKAGE_DIR/bin/winmm.dll" ]] || { echo "Release DLL not found in: $PACKAGE_DIR/bin" >&2; exit 1; }
pgrep -f '[m]gs4.exe' >/dev/null && { echo "Exit the game before installing." >&2; exit 1; }
pgrep -x steam >/dev/null && { echo "Exit Steam completely before installing." >&2; exit 1; }

mkdir -p -- "$BACKUP_DIR"
for name in winmm.dll mgs4_ultrawide.ini; do
  if [[ -e "$GAME_DIR/$name" && ! -e "$BACKUP_DIR/$name.preinstall" ]]; then
    cp -a -- "$GAME_DIR/$name" "$BACKUP_DIR/$name.preinstall"
  fi
done

install -m0644 "$PACKAGE_DIR/bin/winmm.dll" "$GAME_DIR/winmm.dll"
install -m0644 "$PACKAGE_DIR/config/mgs4_ultrawide.ini" "$GAME_DIR/mgs4_ultrawide.ini"
python3 "$SCRIPT_DIR/steam_options.py" install "$STATE_FILE"
echo "Installed in: $GAME_DIR"
echo "Launch through Steam; DirectX 12 is the validated UI path."
