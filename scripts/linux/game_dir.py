#!/usr/bin/env python3
"""Resolve and persist the MGS4 Steam installation directory on Linux."""

from __future__ import annotations

import argparse
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path


APP_ID = "2492670"
DEFAULT_INSTALL_DIR = "METAL GEAR SOLID 4"


def config_file() -> Path:
    base = os.environ.get("XDG_CONFIG_HOME")
    root = Path(base).expanduser() if base else Path.home() / ".config"
    return root / "mgs4Ultra120" / "game-dir"


def clean_vdf_string(value: str) -> str:
    return value.replace(r"\"", '"').replace(r"\\", "\\")


def normalize_game_dir(value: str | Path) -> Path | None:
    text = str(value).strip()
    if not text or "\n" in text or "\r" in text:
        return None
    candidate = Path(text).expanduser()
    if candidate.is_file() and candidate.name.lower() == "mgs4.exe":
        candidate = candidate.parent
    if (candidate / "MGS4" / "mgs4.exe").is_file():
        candidate = candidate / "MGS4"
    if not (candidate / "mgs4.exe").is_file():
        return None
    return candidate.resolve()


def steam_roots() -> list[Path]:
    home = Path.home()
    values: list[Path] = []
    for variable in ("STEAM_DIR", "STEAM_ROOT"):
        if os.environ.get(variable):
            values.append(Path(os.environ[variable]).expanduser())
    values.extend(
        [
            home / ".local/share/Steam",
            home / ".steam/steam",
            home / ".steam/root",
            home / ".var/app/com.valvesoftware.Steam/.local/share/Steam",
            home / ".var/app/com.valvesoftware.Steam/data/Steam",
        ]
    )
    result: list[Path] = []
    seen: set[str] = set()
    for value in values:
        try:
            resolved = value.resolve()
        except OSError:
            resolved = value.absolute()
        key = str(resolved)
        if key not in seen:
            seen.add(key)
            result.append(resolved)
    return result


def libraries_from_vdf(root: Path) -> list[Path]:
    libraries = [root]
    vdf = root / "steamapps/libraryfolders.vdf"
    try:
        text = vdf.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return libraries
    patterns = (
        r'"path"\s*"((?:\\.|[^"\\])*)"',
        r'"\d+"\s*"((?:\\.|[^"\\])*)"',
    )
    for pattern in patterns:
        for match in re.finditer(pattern, text, flags=re.IGNORECASE):
            value = clean_vdf_string(match.group(1))
            if value:
                libraries.append(Path(value).expanduser())
    return libraries


def manifest_install_dir(manifest: Path) -> str:
    try:
        text = manifest.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return DEFAULT_INSTALL_DIR
    match = re.search(
        r'"installdir"\s*"((?:\\.|[^"\\])*)"', text, flags=re.IGNORECASE
    )
    return clean_vdf_string(match.group(1)) if match else DEFAULT_INSTALL_DIR


def discover_game_dirs() -> list[Path]:
    result: list[Path] = []
    seen_libraries: set[str] = set()
    seen_games: set[str] = set()
    for root in steam_roots():
        for library in libraries_from_vdf(root):
            try:
                library = library.resolve()
            except OSError:
                library = library.absolute()
            library_key = str(library)
            if library_key in seen_libraries:
                continue
            seen_libraries.add(library_key)
            manifest = library / f"steamapps/appmanifest_{APP_ID}.acf"
            install_dir = manifest_install_dir(manifest)
            candidates = [
                library / "steamapps/common" / install_dir / "MGS4",
                library / "steamapps/common" / DEFAULT_INSTALL_DIR / "MGS4",
            ]
            for candidate in candidates:
                valid = normalize_game_dir(candidate)
                if valid is not None and str(valid) not in seen_games:
                    seen_games.add(str(valid))
                    result.append(valid)
    return result


def graphical_session_available() -> bool:
    return bool(os.environ.get("DISPLAY") or os.environ.get("WAYLAND_DISPLAY"))


def run_zenity(arguments: list[str]) -> str | None:
    if not shutil.which("zenity") or not graphical_session_available():
        return None
    process = subprocess.run(
        ["zenity", *arguments], text=True, capture_output=True, check=False
    )
    if process.returncode != 0:
        return None
    return process.stdout.strip()


def choose_interactively(discovered: list[Path]) -> Path | None:
    if discovered and shutil.which("zenity") and graphical_session_available():
        arguments = [
            "--list",
            "--radiolist",
            "--title=MGS4 Ultra120 - Select game folder",
            "--text=Select the Steam installation that contains mgs4.exe.",
            "--column=Use",
            "--column=Game folder",
            "--width=900",
            "--height=360",
        ]
        for index, path in enumerate(discovered):
            arguments.extend(["TRUE" if index == 0 else "FALSE", str(path)])
        selected = run_zenity(arguments)
        valid = normalize_game_dir(selected) if selected else None
        if valid is not None:
            return valid

    if shutil.which("zenity") and graphical_session_available():
        while True:
            selected = run_zenity(
                [
                    "--file-selection",
                    "--directory",
                    "--title=MGS4 Ultra120 - Select the folder containing mgs4.exe",
                    "--width=900",
                    "--height=600",
                ]
            )
            if not selected:
                return None
            valid = normalize_game_dir(selected)
            if valid is not None:
                return valid
            subprocess.run(
                [
                    "zenity",
                    "--error",
                    "--title=MGS4 Ultra120",
                    "--text=The selected folder does not contain mgs4.exe. Select the MGS4 subfolder inside the game installation.",
                ],
                check=False,
            )

    if sys.stdin.isatty():
        print(
            "Enter the folder that directly contains mgs4.exe "
            "(leave empty to cancel):",
            file=sys.stderr,
        )
        selected = input().strip()
        return normalize_game_dir(selected) if selected else None
    return None


def resolve(interactive: bool) -> Path:
    override = os.environ.get("MGS4_GAME_DIR")
    if override is not None:
        valid = normalize_game_dir(override)
        if valid is None:
            raise SystemExit(
                f"MGS4_GAME_DIR does not contain mgs4.exe: {override}"
            )
        return valid

    state = config_file()
    try:
        stored = state.read_text(encoding="utf-8").strip()
    except OSError:
        stored = ""
    valid = normalize_game_dir(stored) if stored else None
    if valid is not None:
        return valid

    discovered = discover_game_dirs()
    if len(discovered) == 1:
        return discovered[0]
    if interactive:
        selected = choose_interactively(discovered)
        if selected is not None:
            return selected
        if graphical_session_available() or sys.stdin.isatty():
            raise SystemExit("MGS4 Ultra120 setup was cancelled.")

    details = ""
    if discovered:
        details = "\nDetected candidates:\n" + "\n".join(
            f"  {path}" for path in discovered
        )
    raise SystemExit(
        "Could not locate mgs4.exe in the configured Steam libraries.\n"
        "Run the graphical setup again and select the MGS4 folder, or use:\n"
        'MGS4_GAME_DIR="/path/to/METAL GEAR SOLID 4/MGS4" '
        "./MGS4Ultra120-Linux-Setup.sh"
        + details
    )


def persist(value: str) -> None:
    valid = normalize_game_dir(value)
    if valid is None:
        raise SystemExit(f"Cannot persist an invalid MGS4 game directory: {value}")
    destination = config_file()
    destination.parent.mkdir(parents=True, exist_ok=True, mode=0o700)
    temporary = destination.with_name(destination.name + ".tmp")
    temporary.write_text(f"{valid}\n", encoding="utf-8")
    os.chmod(temporary, 0o600)
    os.replace(temporary, destination)


def forget(value: str) -> None:
    destination = config_file()
    try:
        stored = destination.read_text(encoding="utf-8").strip()
    except OSError:
        return
    try:
        expected = str(Path(value).expanduser().resolve())
        current = str(Path(stored).expanduser().resolve())
    except OSError:
        return
    if current != expected:
        return
    destination.unlink(missing_ok=True)
    try:
        destination.parent.rmdir()
    except OSError:
        pass


def main() -> int:
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers(dest="command", required=True)
    resolve_parser = subparsers.add_parser("resolve")
    resolve_parser.add_argument("--interactive", action="store_true")
    persist_parser = subparsers.add_parser("persist")
    persist_parser.add_argument("path")
    forget_parser = subparsers.add_parser("forget")
    forget_parser.add_argument("path")
    arguments = parser.parse_args()

    if arguments.command == "resolve":
        print(resolve(arguments.interactive))
    elif arguments.command == "persist":
        persist(arguments.path)
    else:
        forget(arguments.path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
