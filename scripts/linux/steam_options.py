#!/usr/bin/env python3
import json
import os
import re
import sys
from pathlib import Path

APP_ID = "2492670"
PREFIX = 'WINEDLLOVERRIDES="winmm=n,b"'


def app_block(text: str) -> tuple[int, int]:
    match = re.search(r'"' + APP_ID + r'"\s*\{', text)
    if not match:
        raise RuntimeError(f"Steam app block {APP_ID} was not found")
    opening = text.find("{", match.start())
    depth = 0
    for index in range(opening, len(text)):
        if text[index] == "{":
            depth += 1
        elif text[index] == "}":
            depth -= 1
            if depth == 0:
                return opening + 1, index
    raise RuntimeError("Incomplete VDF block")


def decode_vdf(value: str) -> str:
    return value.replace(r'\"', '"').replace(r'\\', '\\')


def encode_vdf(value: str) -> str:
    return value.replace('\\', r'\\').replace('"', r'\"')


def find_config() -> Path:
    override = os.environ.get("STEAM_LOCALCONFIG")
    if override:
        return Path(override)
    steam = Path.home() / ".local/share/Steam/userdata"
    for candidate in steam.glob("*/config/localconfig.vdf"):
        if f'"{APP_ID}"' in candidate.read_text(errors="replace"):
            return candidate
    raise RuntimeError("MGS4 localconfig.vdf was not found")


def install(state_path: Path) -> None:
    config = find_config()
    text = config.read_text()
    start, end = app_block(text)
    block = text[start:end]
    pattern = re.compile(r'(?m)^([ \t]*)"LaunchOptions"[ \t]+"((?:\\.|[^"\\])*)"[ \t]*$')
    match = pattern.search(block)
    previous = decode_vdf(match.group(2)) if match else None
    if previous and PREFIX in previous:
        new_value = previous
    elif previous:
        new_value = f"{PREFIX} {previous}"
    else:
        new_value = f"{PREFIX} %command%"

    if state_path.exists() and previous and PREFIX in previous:
        return

    state = {
        "config": str(config),
        "previous_launch_options": previous,
        "installed_launch_options": new_value,
    }
    state_path.write_text(json.dumps(state, ensure_ascii=False, indent=2) + "\n")

    if match:
        line = f'{match.group(1)}"LaunchOptions"\t\t"{encode_vdf(new_value)}"'
        new_block = pattern.sub(line, block, count=1)
    else:
        tail = re.search(r'(\r?\n)([ \t]*)$', block)
        if not tail:
            raise RuntimeError("Could not locate the end of the Steam app block")
        closing_indent = tail.group(2)
        line = f'{closing_indent}\t"LaunchOptions"\t\t"{encode_vdf(new_value)}"'
        new_block = block[:tail.start()] + "\n" + line + tail.group(0)
    updated = text[:start] + new_block + text[end:]
    temporary = config.with_suffix(".vdf.uwmgs4.tmp")
    temporary.write_text(updated)
    os.replace(temporary, config)


def uninstall(state_path: Path) -> None:
    if not state_path.exists():
        return
    state = json.loads(state_path.read_text())
    config = Path(state["config"])
    if not config.exists():
        return
    text = config.read_text()
    start, end = app_block(text)
    block = text[start:end]
    pattern = re.compile(
        r'(?m)^([ \t]*)"LaunchOptions"[ \t]+"((?:\\.|[^"\\])*)"[ \t]*(?:\r?\n)?'
    )
    match = pattern.search(block)
    current = decode_vdf(match.group(2)) if match else None
    installed = state["installed_launch_options"]
    previous = state["previous_launch_options"]
    if current != installed:
        raise RuntimeError("Launch options changed after installation; leaving them untouched")
    if previous is None:
        new_block = pattern.sub("", block, count=1)
    else:
        line = f'{match.group(1)}"LaunchOptions"\t\t"{encode_vdf(previous)}"\n'
        new_block = pattern.sub(line, block, count=1)
    temporary = config.with_suffix(".vdf.uwmgs4.tmp")
    temporary.write_text(text[:start] + new_block + text[end:])
    os.replace(temporary, config)
    state_path.unlink()


def configure(state_path: Path, width: str, height: str, refresh: str, mode: str) -> None:
    if mode not in {"native", "gamescope"}:
        raise RuntimeError("Launch mode must be native or gamescope")
    for label, value, minimum, maximum in (
        ("width", width, 640, 16384),
        ("height", height, 480, 16384),
        ("refresh", refresh, 24, 1000),
    ):
        try:
            parsed = int(value)
        except ValueError as error:
            raise RuntimeError(f"Invalid {label}: {value}") from error
        if not minimum <= parsed <= maximum:
            raise RuntimeError(f"{label} is outside {minimum}-{maximum}")
    if not state_path.exists():
        raise RuntimeError("Install state is missing; run install.sh first")
    state = json.loads(state_path.read_text())
    config = Path(state["config"])
    text = config.read_text()
    start, end = app_block(text)
    block = text[start:end]
    pattern = re.compile(r'(?m)^([ \t]*)"LaunchOptions"[ \t]+"((?:\\.|[^"\\])*)"[ \t]*$')
    match = pattern.search(block)
    current = decode_vdf(match.group(2)) if match else None
    if current != state["installed_launch_options"]:
        raise RuntimeError("Steam launch options changed externally; leaving them untouched")
    if mode == "gamescope":
        new_value = (
            f'{PREFIX} gamescope -f --force-windows-fullscreen '
            f'-W {width} -H {height} -w {width} -h {height} -r {refresh} '
            f'-- gamemoderun %command%'
        )
    else:
        new_value = f"{PREFIX} %command%"
    line = f'{match.group(1)}"LaunchOptions"\t\t"{encode_vdf(new_value)}"'
    updated = text[:start] + pattern.sub(line, block, count=1) + text[end:]
    temporary = config.with_suffix(".vdf.uwmgs4.tmp")
    temporary.write_text(updated)
    os.replace(temporary, config)
    state["installed_launch_options"] = new_value
    state_path.write_text(json.dumps(state, ensure_ascii=False, indent=2) + "\n")


if __name__ == "__main__":
    if len(sys.argv) == 3 and sys.argv[1] in {"install", "uninstall"}:
        arguments = (Path(sys.argv[2]),)
    elif len(sys.argv) == 7 and sys.argv[1] == "configure":
        arguments = (Path(sys.argv[2]), *sys.argv[3:])
    else:
        raise SystemExit(
            f"Usage: {sys.argv[0]} install|uninstall STATE.json OR "
            "configure STATE.json WIDTH HEIGHT REFRESH native|gamescope"
        )
    try:
        globals()[sys.argv[1]](*arguments)
    except Exception as error:
        print(f"ERROR: {error}", file=sys.stderr)
        raise SystemExit(1)
