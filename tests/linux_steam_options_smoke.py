#!/usr/bin/env python3
import importlib.util
import os
import tempfile
from pathlib import Path


SCRIPT = Path(__file__).resolve().parents[1] / "scripts/linux/steam_options.py"
SPEC = importlib.util.spec_from_file_location("steam_options", SCRIPT)
assert SPEC and SPEC.loader
STEAM_OPTIONS = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(STEAM_OPTIONS)


def launch_options(config: Path) -> str | None:
    text = config.read_text()
    start, end = STEAM_OPTIONS.app_block(text)
    match = STEAM_OPTIONS.re.search(
        r'(?m)^([ \t]*)"LaunchOptions"[ \t]+"((?:\\.|[^"\\])*)"[ \t]*$',
        text[start:end],
    )
    return STEAM_OPTIONS.decode_vdf(match.group(2)) if match else None


with tempfile.TemporaryDirectory(prefix="mgs4ultra120-linux-options-") as root:
    root_path = Path(root)
    config = root_path / "localconfig.vdf"
    state = root_path / "state.json"
    original = "PROTON_LOG=1 %command%"
    config.write_text(
        '"UserLocalConfigStore"\n'
        "{\n"
        '  "Software"\n'
        "  {\n"
        '    "Valve"\n'
        "    {\n"
        '      "Steam"\n'
        "      {\n"
        '        "apps"\n'
        "        {\n"
        f'          "{STEAM_OPTIONS.APP_ID}"\n'
        "          {\n"
        f'            "LaunchOptions"  "{original}"\n'
        "          }\n"
        "        }\n"
        "      }\n"
        "    }\n"
        "  }\n"
        "}\n"
    )
    os.environ["STEAM_LOCALCONFIG"] = str(config)

    STEAM_OPTIONS.install(state)
    assert launch_options(config) == f"{STEAM_OPTIONS.PREFIX} {original}"

    STEAM_OPTIONS.configure(state, "3440", "1440", "240", "gamescope")
    gamescope = launch_options(config)
    assert gamescope is not None
    assert "gamescope -f --force-windows-fullscreen" in gamescope
    assert "-W 3440 -H 1440 -w 3440 -h 1440 -r 240" in gamescope
    assert "gamemoderun" not in gamescope
    assert gamescope.endswith("-- %command%")

    STEAM_OPTIONS.configure(state, "3440", "1440", "60", "native")
    assert launch_options(config) == f"{STEAM_OPTIONS.PREFIX} %command%"

    STEAM_OPTIONS.uninstall(state)
    assert launch_options(config) == original
    assert not state.exists()

print("Linux Steam-options smoke test passed.")
