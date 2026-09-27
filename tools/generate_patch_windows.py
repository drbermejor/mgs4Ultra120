"""Generate src/patch_windows.h from an unpacked reference mgs4.exe.

Each window is a masked byte signature taken from the reference executable.
Relative call/jump displacements and RIP-relative displacements are masked so
the signature survives code and data movement; opcodes, registers, stack
offsets and immediates remain significant. Every window must match exactly
once in .text, and every named field must decode as the expected instruction,
or generation fails without writing the header.

Usage: python tools/generate_patch_windows.py <mgs4.exe.unpacked.exe> <out.h>
"""
import re
import sys

import capstone
import pefile
from capstone import x86

# (name, reference_rva, length, [(field, absolute_rva, kind)])
# kind: "call" = E8 rel32, "rip" = RIP-relative memory operand, "site" = offset.
WINDOWS = [
    # Core: startup copy of the compositor getters into the render extent.
    ("render_size_init", 0x3C095, 0x4C, [
        ("width_getter_call", 0x3C0B0, "call"),
        ("width_store", 0x3C0B5, "rip"),
        ("height_getter_call", 0x3C0BB, "call"),
        ("height_store", 0x3C0C0, "rip"),
    ]),
    ("resolution_setter", 0x65F250, 47, []),
    ("projection_setter", 0x0E34D0, 68, []),
    ("camera_builder", 0x0B9B70, 64, []),
    ("cinematic_owner", 0x653000, 54, []),
    # The setter body has a native twin; it is selected through its caller.
    ("controller_setter", 0x7511C0, 49, []),
    ("controller_caller", 0x7462D2, 46, [("setter_call", 0x7462EB, "call")]),
    ("controller_mask", 0x744907, 0x6D, [
        ("mask_load", 0x744918, "rip"),
        ("mask_store", 0x744965, "rip"),
    ]),
    ("camera_primary_route", 0x0BA344, 50, [("call", 0x0BA35E, "call")]),
    ("camera_cinematic_source_route", 0x0B9B44, 44,
     [("call", 0x0B9B5B, "call")]),
    ("camera_final_rebuild_route", 0x0EB193, 46,
     [("call", 0x0EB1A6, "call")]),
    ("reticle_a", 0xE3A13C, 57, [
        ("x_site", 0xE3A146, "site"),
        ("y_site", 0xE3A160, "site"),
        ("width_divisor", 0xE3A154, "rip"),
        ("height_divisor", 0xE3A16F, "rip"),
    ]),
    ("reticle_b", 0xE3A217, 56, [
        ("y_site", 0xE3A221, "site"),
        ("x_site", 0xE3A23C, "site"),
        ("height_divisor", 0xE3A22F, "rip"),
        ("width_divisor", 0xE3A249, "rip"),
    ]),
    ("scaled_extent", 0x16F720, 97, [
        ("base_width", 0x16F72C, "rip"),
        ("base_height", 0x16F753, "rip"),
        ("scaled_width", 0x16F75E, "rip"),
        ("scaled_height", 0x16F776, "rip"),
    ]),
    ("compositor_mirror", 0x0B5433, 59, [
        ("source0", 0x0B5433, "rip"), ("target0", 0x0B543C, "rip"),
        ("source1", 0x0B5442, "rip"), ("target1", 0x0B5448, "rip"),
        ("source2", 0x0B544E, "rip"), ("target2", 0x0B5454, "rip"),
        ("source3", 0x0B545A, "rip"), ("target3", 0x0B5460, "rip"),
    ]),
    # Native centered HUD functions.
    ("hud_layout", 0x4399E0, 66, []),
    ("hud_physical_rect", 0x0BE050, 31, []),
    ("hud_semantic_rect", 0x4DA780, 48, []),
    ("hud_solid_node", 0x4256F0, 31, []),
    ("hud_node_dispatcher", 0x427A80, 27, []),
    ("hud_layer_traversal", 0x4286E0, 28, []),
    ("hud_map_builder", 0x4E9ED0, 28, []),
    ("hud_aux_surface_factory", 0x4DBF30, 65, []),
    ("hud_briefing_init", 0xE6D850, 68, []),
    ("hud_briefing_child", 0xE7D750, 65, []),
    # Native centered HUD caller routes.
    ("hud_route_layout_root", 0x435EF9, 41, [("call", 0x435F0D, "call")]),
    ("hud_route_subtitle", 0x084AF3, 67, [("call", 0x084B17, "call")]),
    ("hud_route_movie", 0x096C27, 42, [("call", 0x096C3B, "call")]),
    ("hud_route_tv_movie", 0xE370CE, 48, [("call", 0xE370E6, "call")]),
    ("hud_route_camouflage", 0x4F8CEE, 47, [("call", 0x4F8D06, "call")]),
    ("hud_route_item", 0x4FCB9A, 123, [("call", 0x4FCBAE, "call")]),
    ("hud_route_drebin", 0x5044F9, 90, [("call", 0x50452E, "call")]),
    ("hud_route_weapon", 0x50875A, 122, [("call", 0x50876E, "call")]),
    ("hud_route_normal_traversal", 0x4287FA, 44,
     [("call", 0x42880E, "call")]),
    ("hud_route_map_init", 0x4E7322, 48, [("call", 0x4E733A, "call")]),
    ("hud_route_map_frame", 0x4E3F74, 49, [("call", 0x4E3F8C, "call")]),
    ("hud_route_codec", 0x51113A, 44, [("call", 0x51114F, "call")]),
    ("hud_route_briefing_child", 0xE6DDB6, 46, [("call", 0xE6DDCD, "call")]),
    ("hud_map_descriptor", 0x4E6EE0, 51, [("lea", 0x4E6EFB, "rip")]),
    ("hud_map_callback", 0x4D7310, 32, [("lea", 0x4D7310, "rip")]),
]

# Windows whose body is known to exist twice; a caller window selects one.
ALLOWED_TWINS = {"controller_setter"}


def identifier(name):
    return "".join(part.capitalize() for part in name.split("_"))


def main():
    exe, out = sys.argv[1], sys.argv[2]
    pe = pefile.PE(exe)
    image = pe.get_memory_mapped_image()
    text = next(s for s in pe.sections if s.Name.rstrip(b"\0") == b".text")
    lo = text.VirtualAddress
    hi = text.VirtualAddress + text.Misc_VirtualSize
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_64)
    md.detail = True

    lines = [
        "#pragma once",
        "",
        "// Generated by tools/generate_patch_windows.py from the reference",
        "// executable recorded in game_profile.h. Do not edit by hand.",
        "",
        "#include \"patch_window.h\"",
        "",
        "namespace mgs4::windows {",
        "",
    ]
    for name, rva, length, fields in WINDOWS:
        data, mask, by_address = b"", [], {}
        for insn in md.disasm(image[rva:rva + length + 16], rva):
            if insn.address >= rva + length:
                break
            significant = [1] * insn.size
            relative = (insn.group(capstone.CS_GRP_JUMP) or
                        insn.group(capstone.CS_GRP_CALL)) and \
                insn.op_str.startswith("0x") and insn.size >= 5
            if relative:
                significant[-4:] = [0] * 4
            for op in insn.operands:
                if op.type == x86.X86_OP_MEM and op.mem.base == x86.X86_REG_RIP:
                    start = insn.disp_offset
                    significant[start:start + 4] = [0] * 4
            data += bytes(insn.bytes)
            mask += significant
            by_address[insn.address] = insn
        pattern = re.compile(b"".join(
            re.escape(bytes([b])) if k else b"." for b, k in zip(data, mask)),
            re.S)
        hits = [m.start() for m in pattern.finditer(image, lo, hi)]
        twin_ok = name in ALLOWED_TWINS and rva in hits and len(hits) == 2
        if hits != [rva] and not twin_ok:
            raise SystemExit(
                f"{name}: expected one match at {rva:#x}, got "
                f"{[hex(h) for h in hits]}")

        ident = identifier(name)
        lines.append(f"inline constexpr std::uint8_t k{ident}Bytes[] = {{")
        for i in range(0, len(data), 12):
            chunk = ", ".join(f"0x{b:02x}" for b in data[i:i + 12])
            lines.append(f"    {chunk},")
        lines.append("};")
        lines.append(f"inline constexpr std::uint8_t k{ident}Mask[] = {{")
        for i in range(0, len(mask), 24):
            lines.append("    " + ", ".join(str(k) for k in mask[i:i + 24]) + ",")
        lines.append("};")
        label = name.replace("_", " ")
        lines.append(
            f"inline constexpr Window k{ident}{{\"{label}\", 0x{rva:x}, "
            f"k{ident}Bytes, k{ident}Mask, sizeof(k{ident}Bytes)}};")
        for field, address, kind in fields:
            insn = by_address.get(address)
            if insn is None:
                raise SystemExit(
                    f"{name}.{field}: {address:#x} is not an instruction")
            if kind == "call":
                if insn.bytes[0] != 0xE8:
                    raise SystemExit(f"{name}.{field}: expected call rel32")
                disp = 1
            elif kind == "rip":
                if not any(op.type == x86.X86_OP_MEM and
                           op.mem.base == x86.X86_REG_RIP
                           for op in insn.operands):
                    raise SystemExit(
                        f"{name}.{field}: expected a RIP-relative operand")
                disp = insn.disp_offset
            else:
                disp = 0
            lines.append(
                f"inline constexpr Field k{ident}{identifier(field)}"
                f"{{{address - rva}, {disp}, {insn.size}}};"
                f"  // {insn.mnemonic} {insn.op_str}")
        lines.append("")
    # Audit lists: each ASI checks only the windows it relies on, before it
    # installs hooks that would change those bytes.
    def audit_list(label, names):
        lines.append(f"inline constexpr AuditEntry k{label}AuditWindows[] = {{")
        for name in names:
            expected = 2 if name in ALLOWED_TWINS else 1
            lines.append(f"    {{&k{identifier(name)}, {expected}}},")
        lines.append("};")

    core = [w[0] for w in WINDOWS if not w[0].startswith("hud_")]
    hud = ["render_size_init"] + [w[0] for w in WINDOWS
                                  if w[0].startswith("hud_")]
    audit_list("Core", core)
    audit_list("Hud", hud)
    lines += ["", "}  // namespace mgs4::windows", ""]
    with open(out, "w", newline="\n") as handle:
        handle.write("\n".join(lines))
    print(f"wrote {len(WINDOWS)} windows to {out}")


if __name__ == "__main__":
    main()
