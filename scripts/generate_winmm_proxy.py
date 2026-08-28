#!/usr/bin/env python3
"""Generate the complete x64 WinMM proxy export table and trampolines."""

from __future__ import annotations

import argparse
from pathlib import Path


CUSTOM_EXPORTS = {"timeBeginPeriod", "timeGetTime"}
CUSTOM_TARGETS = {
    "timeBeginPeriod": "mgs4_timeBeginPeriod",
    "timeGetTime": "mgs4_timeGetTime",
}


def read_exports(path: Path) -> list[tuple[int, str | None]]:
    exports: list[tuple[int, str | None]] = []
    for line_number, raw in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        line = raw.split("#", 1)[0].strip()
        if not line:
            continue
        fields = line.split()
        if len(fields) != 2 or not fields[0].isdigit():
            raise ValueError(f"{path}:{line_number}: expected '<ordinal> <name>'")
        ordinal = int(fields[0])
        name = None if fields[1] == "NONAME" else fields[1]
        exports.append((ordinal, name))

    expected = list(range(2, 183))
    ordinals = [ordinal for ordinal, _ in exports]
    if ordinals != expected:
        raise ValueError("WinMM exports must contain every ordinal from 2 through 182")
    names = [name for _, name in exports if name]
    if len(names) != len(set(names)):
        raise ValueError("WinMM export names must be unique")
    if not CUSTOM_EXPORTS.issubset(names):
        raise ValueError("Custom time exports are missing from the export list")
    return exports


def write_definition(path: Path, exports: list[tuple[int, str | None]]) -> None:
    lines = ["LIBRARY winmm", "EXPORTS"]
    for index, (ordinal, name) in enumerate(exports):
        if name is None:
            lines.append(f"    winmm_proxy_{index} @{ordinal} NONAME")
        elif name in CUSTOM_EXPORTS:
            lines.append(f"    {name}={CUSTOM_TARGETS[name]} @{ordinal}")
        else:
            lines.append(f"    {name}=winmm_proxy_{index} @{ordinal}")
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def write_include(path: Path, exports: list[tuple[int, str | None]]) -> None:
    lines = ["static constexpr WinmmExportSpec kWinmmExports[] = {"]
    for ordinal, name in exports:
        literal = "nullptr" if name is None else f'"{name}"'
        lines.append(f"    {{{ordinal}, {literal}}},")
    lines.append("};")
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def masm_stub(index: int) -> str:
    return f"""winmm_proxy_{index} PROC FRAME
    sub rsp, 136
    .allocstack 136
    .endprolog
    mov [rsp + 32], rcx
    mov [rsp + 40], rdx
    mov [rsp + 48], r8
    mov [rsp + 56], r9
    movdqu xmmword ptr [rsp + 64], xmm0
    movdqu xmmword ptr [rsp + 80], xmm1
    movdqu xmmword ptr [rsp + 96], xmm2
    movdqu xmmword ptr [rsp + 112], xmm3
    mov ecx, {index}
    call winmm_proxy_resolve
    mov r11, rax
    movdqu xmm0, xmmword ptr [rsp + 64]
    movdqu xmm1, xmmword ptr [rsp + 80]
    movdqu xmm2, xmmword ptr [rsp + 96]
    movdqu xmm3, xmmword ptr [rsp + 112]
    mov rcx, [rsp + 32]
    mov rdx, [rsp + 40]
    mov r8, [rsp + 48]
    mov r9, [rsp + 56]
    add rsp, 136
    jmp r11
winmm_proxy_{index} ENDP
"""


def gnu_stub(index: int) -> str:
    return f""".globl winmm_proxy_{index}
.def winmm_proxy_{index}; .scl 2; .type 32; .endef
.seh_proc winmm_proxy_{index}
winmm_proxy_{index}:
    sub rsp, 136
    .seh_stackalloc 136
    .seh_endprologue
    mov [rsp + 32], rcx
    mov [rsp + 40], rdx
    mov [rsp + 48], r8
    mov [rsp + 56], r9
    movdqu [rsp + 64], xmm0
    movdqu [rsp + 80], xmm1
    movdqu [rsp + 96], xmm2
    movdqu [rsp + 112], xmm3
    mov ecx, {index}
    call winmm_proxy_resolve
    mov r11, rax
    movdqu xmm0, [rsp + 64]
    movdqu xmm1, [rsp + 80]
    movdqu xmm2, [rsp + 96]
    movdqu xmm3, [rsp + 112]
    mov rcx, [rsp + 32]
    mov rdx, [rsp + 40]
    mov r8, [rsp + 48]
    mov r9, [rsp + 56]
    add rsp, 136
    jmp r11
.seh_endproc
"""


def write_assembly(
    path: Path, exports: list[tuple[int, str | None]], assembly_format: str
) -> None:
    if assembly_format == "masm":
        parts = ["option casemap:none\nEXTERN winmm_proxy_resolve:PROC\n.code\n"]
        parts.extend(
            masm_stub(index)
            for index, (_, name) in enumerate(exports)
            if name not in CUSTOM_EXPORTS
        )
        parts.append("END\n")
    else:
        parts = [".intel_syntax noprefix\n.text\n.extern winmm_proxy_resolve\n"]
        parts.extend(
            gnu_stub(index)
            for index, (_, name) in enumerate(exports)
            if name not in CUSTOM_EXPORTS
        )
    path.write_text("\n".join(parts), encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--exports", type=Path, required=True)
    parser.add_argument("--definition", type=Path, required=True)
    parser.add_argument("--include", type=Path, required=True)
    parser.add_argument("--assembly", type=Path, required=True)
    parser.add_argument("--format", choices=("masm", "gnu"), required=True)
    args = parser.parse_args()
    exports = read_exports(args.exports)
    for output in (args.definition, args.include, args.assembly):
        output.parent.mkdir(parents=True, exist_ok=True)
    write_definition(args.definition, exports)
    write_include(args.include, exports)
    write_assembly(args.assembly, exports, args.format)


if __name__ == "__main__":
    main()
