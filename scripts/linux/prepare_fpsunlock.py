#!/usr/bin/env python3
"""Fetch the official MGSFPSUnlock release and adapt it locally for Proton."""

from __future__ import annotations

import argparse
import hashlib
import os
from pathlib import Path
import tempfile
import urllib.request
from zipfile import ZipFile


RELEASE_URL = (
    "https://github.com/cipherxof/MGSFPSUnlock/releases/download/0.1.0/"
    "MGSFPSUnlock.zip"
)
ARCHIVE_SHA256 = "f5dca70b095dd7ea9a6f181677bc37f35a97ffa068ee6fc6b9b269407cde4d8a"
ASI_SHA256 = "9da6f4bf1478e78dd94627ef0b1bd8255e0d3cb1cf343464d9951775b0674679"
PROTON_ASI_SHA256 = "7a52737883dff4cdf641b986d06bf17101c2dd13c3f4862cc633f4d05fb19dc3"
ASI_MEMBER = "MGSFPSUnlock/scripts/MGSFPSUnlock.asi"

# MinHook's IsExecutableAddress checks PAGE_EXECUTE_* (mask 0xF0). Wine reports
# MGS4's unpacked .text as PAGE_WRITECOPY (0x08), despite executing that code.
# Changing the mask to 0xF8 accepts that Wine-specific report. The complete
# surrounding instruction sequence and both official hashes are pinned.
MINHOOK_SIGNATURE = bytes.fromhex(
    "31 c0 81 7c 24 40 00 10 00 00 75 0a 31 c0 "
    "f6 44 24 44 f0 0f 95 c0"
)
MASK_INDEX = 18


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def patch_for_proton(official: bytes) -> bytes:
    if sha256(official) != ASI_SHA256:
        raise RuntimeError("official MGSFPSUnlock.asi SHA-256 does not match")
    positions: list[int] = []
    start = 0
    while True:
        position = official.find(MINHOOK_SIGNATURE, start)
        if position < 0:
            break
        positions.append(position)
        start = position + 1
    if len(positions) != 1:
        raise RuntimeError(
            f"expected one pinned MinHook signature, found {len(positions)}"
        )
    patched = bytearray(official)
    mask_position = positions[0] + MASK_INDEX
    if patched[mask_position] != 0xF0:
        raise RuntimeError("unexpected MinHook protection mask")
    patched[mask_position] = 0xF8
    result = bytes(patched)
    if sha256(result) != PROTON_ASI_SHA256:
        raise RuntimeError("locally adapted MGSFPSUnlock.asi SHA-256 does not match")
    return result


def load_archive(path: Path | None) -> bytes:
    if path is not None:
        return path.read_bytes()
    request = urllib.request.Request(
        RELEASE_URL, headers={"User-Agent": "MGS4Ultra120-Linux-Setup"}
    )
    with urllib.request.urlopen(request, timeout=60) as response:
        return response.read()


def prepare(output: Path, archive_path: Path | None) -> None:
    if output.is_file() and sha256(output.read_bytes()) == PROTON_ASI_SHA256:
        print(f"Verified existing Proton FPS module: {output}")
        return
    archive = load_archive(archive_path)
    if sha256(archive) != ARCHIVE_SHA256:
        raise RuntimeError("official MGSFPSUnlock.zip SHA-256 does not match")
    with tempfile.TemporaryDirectory(prefix="mgs4ultra120-fps-") as directory:
        archive_file = Path(directory) / "MGSFPSUnlock.zip"
        archive_file.write_bytes(archive)
        with ZipFile(archive_file) as package:
            if ASI_MEMBER not in package.namelist():
                raise RuntimeError(f"{ASI_MEMBER} is missing from the official archive")
            patched = patch_for_proton(package.read(ASI_MEMBER))
    output.parent.mkdir(parents=True, exist_ok=True)
    descriptor, temporary_name = tempfile.mkstemp(
        prefix=f".{output.name}.", suffix=".tmp", dir=output.parent
    )
    try:
        with os.fdopen(descriptor, "wb") as temporary:
            temporary.write(patched)
            temporary.flush()
            os.fsync(temporary.fileno())
        os.chmod(temporary_name, 0o644)
        os.replace(temporary_name, output)
    finally:
        if os.path.exists(temporary_name):
            os.unlink(temporary_name)
    print("Downloaded MGSFPSUnlock 0.1.0 from its official GitHub release.")
    print(f"Prepared Proton FPS module: {output}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument(
        "--archive",
        type=Path,
        help="Use a local official MGSFPSUnlock.zip (primarily for testing)",
    )
    arguments = parser.parse_args()
    try:
        prepare(arguments.output, arguments.archive)
    except Exception as error:
        parser.exit(1, f"ERROR: {error}\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
