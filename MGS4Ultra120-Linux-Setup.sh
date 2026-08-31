#!/usr/bin/env bash
set -euo pipefail

if (( $# > 1 )); then
  echo "Usage: $0 [folder-containing-mgs4.exe]" >&2
  exit 1
fi
if (( $# == 1 )); then
  export MGS4_GAME_DIR="$1"
fi

PACKAGE_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
CANONICAL_DIR="${MGS4_PACKAGE_INSTALL_DIR:-${XDG_DATA_HOME:-$HOME/.local/share}/mgs4Ultra120}"
if [[ "$PACKAGE_DIR" != "$CANONICAL_DIR" && "${MGS4_PACKAGE_CANONICALIZED:-0}" != 1 ]]; then
  install -d "$(dirname -- "$CANONICAL_DIR")"
  STAGING_DIR="$(mktemp -d "$(dirname -- "$CANONICAL_DIR")/.mgs4Ultra120-new-XXXXXX")"
  cp -a "$PACKAGE_DIR/." "$STAGING_DIR/"
  if [[ -e "$CANONICAL_DIR" ]]; then
    PREVIOUS_DIR="$CANONICAL_DIR.previous.$(date -u +%Y%m%dT%H%M%SZ).$$"
    mv -- "$CANONICAL_DIR" "$PREVIOUS_DIR"
    echo "Previous local package retained at: $PREVIOUS_DIR"
  fi
  mv -- "$STAGING_DIR" "$CANONICAL_DIR"
  export MGS4_PACKAGE_CANONICALIZED=1
  exec "$CANONICAL_DIR/MGS4Ultra120-Linux-Setup.sh"
fi
GAME_DIR_RESOLVER="$PACKAGE_DIR/scripts/linux/game_dir.py"
[[ -f "$GAME_DIR_RESOLVER" ]] || {
  echo "Linux game-directory resolver is missing: $GAME_DIR_RESOLVER" >&2
  exit 1
}
export MGS4_GAME_DIR="$(python3 "$GAME_DIR_RESOLVER" resolve --interactive)"
if [[ -z "${MGS4_INSTALL_FPS_UNLOCK+x}" &&
      "${MGS4_CONFIGURE_AFTER_INSTALL:-1}" != 0 &&
      -n "$(command -v zenity 2>/dev/null)" ]]; then
  if zenity --question --title='MGS4 Ultra120 setup' \
      --text='Install corrected 30/60/120 FPS support? The official MGSFPSUnlock 0.1.0 package will be downloaded, verified and adapted locally for Proton. Select Core only to keep the game normal FPS behavior.' \
      --ok-label='Install FPS support' --cancel-label='Core only'; then
    export MGS4_INSTALL_FPS_UNLOCK=1
  else
    export MGS4_INSTALL_FPS_UNLOCK=0
  fi
fi
"$PACKAGE_DIR/scripts/linux/install.sh"
if [[ "${MGS4_CONFIGURE_AFTER_INSTALL:-1}" == 0 ]]; then
  exit 0
fi
"$PACKAGE_DIR/scripts/linux/configure.sh" detected || true
if command -v zenity >/dev/null 2>&1; then
  "$PACKAGE_DIR/scripts/linux/configure.sh" gui
else
  echo "Installed successfully. Install zenity for the graphical configurator,"
  echo "or edit mgs4_ultrawide.ini, mgs4_native_centered_hud.ini and scripts/MGSFPSUnlock.ini manually."
fi
