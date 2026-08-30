#!/usr/bin/env python3
import os
import subprocess
import sys
import tempfile
from pathlib import Path


SCRIPT = Path(__file__).resolve().parents[1] / "scripts/linux/game_dir.py"


def run(arguments, environment, expected=0, cwd=None):
    process = subprocess.run(
        [sys.executable, str(SCRIPT), *arguments],
        text=True,
        capture_output=True,
        env=environment,
        cwd=cwd,
        check=False,
    )
    if process.returncode != expected:
        raise AssertionError(
            f"{arguments} returned {process.returncode}:\n"
            f"stdout={process.stdout}\nstderr={process.stderr}"
        )
    return process


with tempfile.TemporaryDirectory(prefix="mgs4-game-dir-") as temporary:
    root = Path(temporary)
    home = root / "home"
    steam = home / ".local/share/Steam"
    external = root / "External Steam Library"
    game = external / "steamapps/common/Custom MGS4 Name/MGS4"
    game.mkdir(parents=True)
    (game / "mgs4.exe").touch()
    (steam / "steamapps").mkdir(parents=True)
    (steam / "steamapps/libraryfolders.vdf").write_text(
        '"libraryfolders"\n{\n  "1"\n  {\n'
        f'    "path" "{external}"\n'
        f'    "apps" {{ "2492670" "1" }}\n'
        "  }\n}\n",
        encoding="utf-8",
    )
    (external / "steamapps/appmanifest_2492670.acf").write_text(
        '"AppState"\n{\n  "appid" "2492670"\n'
        '  "installdir" "Custom MGS4 Name"\n}\n',
        encoding="utf-8",
    )
    environment = os.environ.copy()
    environment.update(
        {
            "HOME": str(home),
            "XDG_CONFIG_HOME": str(root / "config"),
            "STEAM_DIR": str(steam),
        }
    )
    environment.pop("MGS4_GAME_DIR", None)
    environment.pop("STEAM_ROOT", None)

    resolved = run(["resolve"], environment).stdout.strip()
    assert Path(resolved) == game.resolve()

    run(["persist", str(game)], environment)
    (steam / "steamapps/libraryfolders.vdf").unlink()
    assert Path(run(["resolve"], environment).stdout.strip()) == game.resolve()

    explicit = environment.copy()
    explicit["MGS4_GAME_DIR"] = str(game.parent)
    assert Path(run(["resolve"], explicit).stdout.strip()) == game.resolve()

    cwd_environment = environment.copy()
    cwd_environment["XDG_CONFIG_HOME"] = str(root / "cwd-config")
    cwd_environment["STEAM_DIR"] = str(root / "missing-steam")
    assert Path(
        run(["resolve"], cwd_environment, cwd=game).stdout.strip()
    ) == game.resolve()

    invalid = environment.copy()
    invalid["MGS4_GAME_DIR"] = str(root / "missing")
    failure = run(["resolve"], invalid, expected=1)
    assert "does not contain mgs4.exe" in failure.stderr

    run(["forget", str(game)], environment)
    assert not (root / "config/mgs4Ultra120/game-dir").exists()

    if os.name == "posix":
        flatpak_home = root / "flatpak-home"
        flatpak_steam = (
            flatpak_home
            / ".var/app/com.valvesoftware.Steam/.local/share/Steam"
        )
        flatpak_game = (
            flatpak_steam
            / "steamapps/common/METAL GEAR SOLID 4/MGS4"
        )
        flatpak_game.mkdir(parents=True)
        (flatpak_game / "mgs4.exe").touch()
        (flatpak_steam / "steamapps/appmanifest_2492670.acf").write_text(
            '"AppState"\n{\n  "appid" "2492670"\n'
            '  "installdir" "METAL GEAR SOLID 4"\n}\n',
            encoding="utf-8",
        )
        flatpak_environment = os.environ.copy()
        flatpak_environment.update(
            {
                "HOME": str(flatpak_home),
                "XDG_CONFIG_HOME": str(root / "flatpak-config"),
            }
        )
        for variable in ("MGS4_GAME_DIR", "STEAM_DIR", "STEAM_ROOT"):
            flatpak_environment.pop(variable, None)
        assert Path(
            run(["resolve"], flatpak_environment).stdout.strip()
        ) == flatpak_game.resolve()
