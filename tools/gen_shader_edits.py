"""Generates shader_edits.h: edits of the game's shaders that NierReplicantGFX applies.

The plugin changes a shader in ID3D11Device::CreatePixelShader / CreateComputeShader when its DXBC
checksum and size match an entry of the header, inserting the instructions of the enabled fixes. The
header holds checksums, sizes, offsets and the inserted instructions, no code of the game.

Fixes (FIX_* bits):
  AO_PATTERN     The HBAO shaders turn their sampling directions by a per-pixel hash,
                 ((x*3) ^ (x*y + y)) * 10 rad. This noise is fixed to the screen and coarser than the
                 game's ~8 px blur can remove, so it shows as a pattern that slides over surfaces when the
                 camera moves. The edit uses a rotation tiled every 4x4 pixels instead, which the blur
                 removes:
                     pixel:   mul s.xy, v0.xyxx, l(0.25)      compute: utof s.yz, vThreadID.xxyx
                                                                       mul  s.yz, s.yyzy, l(0.25)
                     frc s, s ; dp2 angle, s, l(c1, c2, 0, 0) ; sincos (the original instruction)
                 With N directions 2*pi/N apart (default 4, high/highest 8) only the angle modulo 2*pi/N
                 matters: c = (2*pi/N) * (3, 2.12109375) gives 16 distinct angles per 4x4 tile, with
                 neighbours (diagonals included) at least 0.16 of the period apart.
  AO_EDGES       gen_ssao_mask_default_c ends a sampling direction at its first step outside the screen
                 (ftoi p; ige; ult p, size; and; and cond), so AO fades near the edges. With this edit the
                 step loads the depth at its position clamped to the screen (clamp-to-edge); the position
                 itself is kept:
                     ftoi p.xy, pos ; imax p.xy, p, l(0) ; iadd t.xy, size, l(-1) ; imin p.xy, p, t
                     mov t, l(-1) ; mov a.zw, l(-1) ; mov cond, l(-1)
                 Afterwards every register holds what it holds for a step inside the screen.
  AO_EDGES_SOFT  As AO_EDGES, with the falloff of a step outside the screen multiplied by
                 w = saturate(1 - d / radius), d = pixels beyond the edge (Chebyshev), radius = 9 * step
                 (r1.z after "max r1.z, r1.z, l(1)"): every "mad_sat rX.c, rX.c, cb0[8].w, l(1)" is
                 followed by "mul rX.c, rX.c, r13.x". A new temp r13 holds w (x) and 1 / radius (z).
                 Both edge edits take the area beyond the edge as a surface at the edge pixel's depth. On
                 a surface seen at an angle it lies in front of the real one and shades a band along the
                 edge, so both are off by default. The plugin's own AO shader handles the edges instead.
  FEEDBACK       pfx_feedback_blur_p writes current * src_coeff + previous * dst_coeff to the screen and to
                 its history, a trail of the last frames most visible when rolling. The edit writes the
                 current frame: "mad r0, current, cb0[0].x, r0" -> "mov r0, current".
  DITHER         pfx_manual_apply_oetf_p (the final pass; the SDR variants with and without a colour
                 matrix) quantises to 8 bits. The colour goes to a new temp t and gets +-0.5/255 of
                 interleaved gradient noise from the pixel position:
                     dcl_input_ps_siv linear noperspective v0.xy, position (and SV_POSITION used in ISGN)
                     dp2 t.w, v0.xyxx, l(0.06711056, 0.00583715) ; frc ; mul l(52.9829189) ; frc ;
                     add t.w, t.w, l(-0.5) ; mad o0.xyz, t.wwww, l(1/255), t.xyzx

Usage:
  python gen_shader_edits.py <pfx_shader> <resident_shader> [--out shader_edits.h] [--expected DIR] [--check]
    pfx_shader, resident_shader  the PACK files system/graphic/libtp/* extracted from the game
    --expected DIR               write orig_<shader>.dxbc and fixed_<shader>.dxbc (FIX_DEFAULT) for the test
    --check                      create every variant on a WARP Direct3D 11 device
"""
import argparse
import math
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import dxbc  # noqa: E402
import pack  # noqa: E402

AO_PATTERN, AO_EDGES, AO_EDGES_SOFT, FEEDBACK, DITHER = 1, 2, 4, 8, 16
FIX_NAMES = {AO_PATTERN: "AO_PATTERN", AO_EDGES: "AO_EDGES", AO_EDGES_SOFT: "AO_EDGES_SOFT",
             FEEDBACK: "FEEDBACK", DITHER: "DITHER"}
DEFAULT = AO_PATTERN | FEEDBACK | DITHER
EXCLUSIVE = (AO_EDGES, AO_EDGES_SOFT)        # alternative edits of the same sites

# (pack, shader, sampling directions for AO or 0); order matters: the plugin numbers them, the test relies on
# gen_ssao_mask_default_c = 0 and pfx_hbao_default_p = 1
SHADERS = [
    ("resident_shader", "gen_ssao_mask_default_c", 4),
    ("pfx_shader", "pfx_hbao_default_p", 4),
    ("pfx_shader", "pfx_hbao_nrm_default_p", 4),
    ("pfx_shader", "pfx_hbao_high_p", 8),
    ("pfx_shader", "pfx_hbao_nrm_high_p", 8),
    ("pfx_shader", "pfx_hbao_highest_p", 8),
    ("pfx_shader", "pfx_hbao_nrm_highest_p", 8),
    ("pfx_shader", "pfx_feedback_blur_p", 0),
    ("pfx_shader", "pfx_manual_apply_oetf_p.0", 0),
    ("pfx_shader", "pfx_manual_apply_oetf_p.1", 0),
    ("pfx_shader", "pfx_manual_apply_oetf_p.6", 0),
    ("pfx_shader", "pfx_manual_apply_oetf_p.7", 0),
]
ROTATION = (3.0, 2.12109375)
OP = dxbc.OPCODES
PIXEL_HASH = [OP["ftoi"], OP["imul"], OP["imad"], OP["xor"], OP["imul"], OP["itof"], OP["sincos"]]
COMPUTE_HASH = PIXEL_HASH[1:]
BOUND_CHECK = [OP["ftoi"], OP["ige"], OP["ult"], OP["and"], OP["and"]]
FALLOFF_TAIL = [0x0020803A, 0, 8, 0x00004001, 0x3F800000]   # mad_sat rX.c, rX.c, | cb0[8].w, l(1)
STEP = [0x07000038, 0x00100042, 1, 0x0010002A, 1, 0x00004001, 0x3DE38E39]   # mul r1.z, r1.z, l(0.111111)
SHBIN_HEADER = 0x80
QUARTER = dxbc.imm(0.25, 0.25, 0.25, 0.25)
MINUS_ONE = dxbc.imm(-1, -1, -1, -1)
W = 13                       # the temp AO_EDGES_SOFT adds to the compute shader


def _find(ins, seq):
    ops = [op for _, op, _ in ins]
    return [i for i in range(len(ops) - len(seq) + 1) if ops[i:i + len(seq)] == seq]


def _end(i):
    off, _, n = i
    return off + 4 * n


# ------------------------------------------------------------------ AO

def pattern_edits(blob, directions):
    ins = dxbc.instructions(blob)
    pixel = _find(ins, PIXEL_HASH)
    starts = pixel or _find(ins, COMPUTE_HASH)
    assert len(starts) == 1, f"hash sequence found {len(starts)} times"
    k = starts[0]
    first, last = ins[k], ins[k + (7 if pixel else 6) - 1]
    start, end = first[0], _end(last)
    sincos = dxbc.words(blob, last[0], last[2])
    assert sincos[5] == 0x0010003A, "sincos source is not a .w temp"
    angle = sincos[6]
    c1, c2 = ((2 * math.pi / directions) * p for p in ROTATION)
    if pixel:
        ftoi = dxbc.words(blob, start, 5)
        assert ftoi[1] == 0x00100032 and ftoi[3] == 0x00101046, "ftoi rN.xy, v0.xyxx expected"
        reg, comps = ftoi[2], "xy"
        words = (dxbc.instr("mul", dxbc.temp(reg, "xy", True), dxbc.input_reg(0, "xyxx"), QUARTER)
                 + dxbc.instr("frc", dxbc.temp(reg, "xy", True), dxbc.temp(reg, "xyxx"))
                 + dxbc.instr("dp2", dxbc.temp(angle, "w", True), dxbc.temp(reg, "xyxx"), dxbc.imm(c1, c2, 0.0, 0.0)))
    else:
        reg, comps = sincos[2], "yz"
        words = (dxbc.instr("utof", dxbc.temp(reg, "yz", True), dxbc.thread_id("xxyx"))
                 + dxbc.instr("mul", dxbc.temp(reg, "yz", True), dxbc.temp(reg, "yyzy"), QUARTER)
                 + dxbc.instr("frc", dxbc.temp(reg, "yz", True), dxbc.temp(reg, "yyzy"))
                 + dxbc.instr("dp2", dxbc.temp(angle, "w", True), dxbc.temp(reg, "yzyy"), dxbc.imm(c1, c2, 0.0, 0.0)))
    # the components the copied sincos writes hold its result, not scratch
    sin_reg, sin_mask = sincos[2], {c for k, c in enumerate("xyzw") if (sincos[1] >> (4 + k)) & 1}
    scratch = set(comps) - (sin_mask if sin_reg == reg else set())
    assert dxbc.dead_after(blob, end, reg, scratch), f"r{reg}.{''.join(sorted(scratch))} is read after the hash"
    return [(AO_PATTERN, start, (end - start) // 4, words + sincos)]


def _dst(blob, off, n):
    """The destination operand tokens (token + index) of an instruction without extended tokens."""
    w = dxbc.words(blob, off, n)
    assert not w[0] >> 31 and not w[1] >> 31
    return w[1:3]


def edge_edits(blob):
    """AO_EDGES and AO_EDGES_SOFT edits of the compute shader."""
    ins = dxbc.instructions(blob)
    checks = _find(ins, BOUND_CHECK)
    assert len(checks) == 32, f"{len(checks)} bound checks, 32 expected (4 directions x 8 steps)"
    def is_falloff(off, n):
        w = dxbc.words(blob, off, n)
        return n == 10 and w[0] == 0x0A002032 and w[5:] == FALLOFF_TAIL and w[2] == w[4] and \
            dxbc.operands(blob, off, n)[0][2] == dxbc.operands(blob, off, n)[1][2]
    falloffs = [i for i, (off, _, n) in enumerate(ins) if is_falloff(off, n)]
    steps = [i for i, (off, _, n) in enumerate(ins) if n == len(STEP) and dxbc.words(blob, off, n) == STEP]
    temps = [(off, n) for off, op, n in ins if op == OP["dcl_temps"]]
    assert len(falloffs) == 32 and len(steps) == 1 and len(temps) == 1
    # every step: its bound check, then its falloff before the next bound check
    for a, b in zip(checks, falloffs):
        assert a < b, "falloff before its bound check"
    for b, a_next in zip(falloffs, checks[1:]):
        assert b < a_next, "two bound checks without a falloff between them"
    assert dxbc.words(blob, *temps[0])[1] == W, f"dcl_temps {W} expected"
    after_step = ins[steps[0] + 1]
    assert dxbc.words(blob, after_step[0], after_step[2]) == [0x07000034, 0x00100042, 1, 0x0010002A, 1, 0x00004001, 0x3F800000], \
        "max r1.z, r1.z, l(1) after the step size expected"

    out = []
    for soft in (False, True):
        kind = AO_EDGES_SOFT if soft else AO_EDGES
        for k in checks:
            (f_off, _, f_n), (g_off, _, g_n), (u_off, _, u_n), (a_off, _, a_n), (c_off, _, c_n) = ins[k:k + 5]
            ftoi = dxbc.words(blob, f_off, f_n)
            p = dxbc.operands(blob, f_off, f_n)[0]
            ige = dxbc.operands(blob, g_off, g_n)
            ult = dxbc.operands(blob, u_off, u_n)
            assert p[0] == dxbc.TEMP and p[2] == {"x", "y"}, "ftoi rP.xy expected"
            assert ige[0][1] == ult[0][1] and ige[0][2] == {"x", "y"} and ult[0][2] == {"z", "w"}, "ige/ult into one temp"
            assert ige[1][1] == p[1] and ult[1][1] == p[1], "checks of the ftoi result"
            t, size, pr = ige[0][1], ult[2][1], p[1]
            words = ftoi
            if soft:
                words += dxbc.instr("mov", dxbc.temp(W, "xy", True), dxbc.temp(pr, "xyxx"))
            words += (dxbc.instr("imax", dxbc.temp(pr, "xy", True), dxbc.temp(pr, "xyxx"), dxbc.imm(0, 0, 0, 0))
                      + dxbc.instr("iadd", dxbc.temp(t, "xy", True), dxbc.temp(size, "xyxx"), MINUS_ONE)
                      + dxbc.instr("imin", dxbc.temp(pr, "xy", True), dxbc.temp(pr, "xyxx"), dxbc.temp(t, "xyxx")))
            if soft:
                words += (dxbc.instr("iadd", dxbc.temp(W, "xy", True), dxbc.temp(W, "xyxx"), dxbc.neg(dxbc.temp(pr, "xyxx")))
                          + dxbc.instr("imax", dxbc.temp(W, "xy", True), dxbc.temp(W, "xyxx"), dxbc.neg(dxbc.temp(W, "xyxx")))
                          + dxbc.instr("imax", dxbc.temp(W, "x", True), dxbc.temp(W, "x"), dxbc.temp(W, "y"))
                          + dxbc.instr("itof", dxbc.temp(W, "x", True), dxbc.temp(W, "x"))
                          + dxbc.instr("mad", dxbc.temp(W, "x", True), dxbc.neg(dxbc.temp(W, "x")), dxbc.temp(W, "z"),
                                       dxbc.imm(1.0), sat=True))
            words += (dxbc.instr("mov", dxbc.temp(t, "xyzw", True), MINUS_ONE)
                      + dxbc.instr("mov", _dst(blob, a_off, a_n), MINUS_ONE)
                      + dxbc.instr("mov", _dst(blob, c_off, c_n), MINUS_ONE))
            out.append((kind, f_off, (c_off + 4 * c_n - f_off) // 4, words))
        if soft:
            # one more temp; 1 / radius once per thread; the falloff of every step times w
            t_off, t_n = temps[0]
            out.append((kind, t_off, t_n, [dxbc.words(blob, t_off, t_n)[0], W + 1]))
            out.append((kind, _end(after_step), 0,
                        dxbc.instr("mul", dxbc.temp(W, "z", True), dxbc.temp(1, "z"), dxbc.imm(9.0))
                        + dxbc.instr("div", dxbc.temp(W, "z", True), dxbc.imm(1.0), dxbc.temp(W, "z"))))
            for b in falloffs:
                f = dxbc.words(blob, ins[b][0], ins[b][2])           # its destination and its own source
                out.append((kind, _end(ins[b]), 0, dxbc.instr("mul", f[1:3], f[3:5], dxbc.temp(W, "x"))))
    return out


# ------------------------------------------------------------------ feedback blur, dither

def feedback_edits(blob):
    ins = dxbc.instructions(blob)
    mads = [(off, n) for off, op, n in ins if op == OP["mad"]]
    assert len(mads) == 1, "one mad expected"
    off, n = mads[0]
    w = dxbc.words(blob, off, n)
    ops = dxbc.operands(blob, off, n)
    assert n == 10 and ops[0][2] == set("xyzw") and ops[1][0] == dxbc.TEMP, "mad r0.xyzw, r1.xyzw, cb0[0].xxxx, r0.xyzw"
    return [(FEEDBACK, off, n, dxbc.instr("mov", w[1:3], w[3:5]))]


def _isgn_position(blob):
    """Offset of the dword holding mask / read-write mask of SV_POSITION in the ISGN chunk."""
    off, _ = dxbc.chunks(blob)["ISGN"]
    count = struct.unpack_from("<I", blob, off)[0]
    for i in range(count):
        e = off + 8 + 24 * i
        name_off = struct.unpack_from("<I", blob, e)[0]
        name = blob[off + name_off:blob.index(b"\0", off + name_off)]
        if name == b"SV_POSITION":
            assert struct.unpack_from("<I", blob, e + 16)[0] == 0, "SV_POSITION in register 0 expected"
            return e + 20
    raise AssertionError("SV_POSITION not in ISGN")


def dither_edits(blob, siv_decl):
    ins = dxbc.instructions(blob)
    out = []
    mask_off = _isgn_position(blob)
    mask = struct.unpack_from("<I", blob, mask_off)[0]
    assert mask & 0xFF00 == 0, "SV_POSITION already read"
    out.append((DITHER, mask_off, 1, [mask | 0x0300]))                      # read-write mask .xy
    temps = [(off, n) for off, op, n in ins if op == OP["dcl_temps"]]
    inputs = [(off, n) for off, op, n in ins if op == OP["dcl_input_ps"]]
    assert len(temps) == 1 and len(inputs) == 1
    t = dxbc.words(blob, *temps[0])[1]
    out.append((DITHER, inputs[0][0], 0, siv_decl))                         # dcl_input_ps_siv v0.xy, position
    out.append((DITHER, temps[0][0], temps[0][1], [dxbc.words(blob, *temps[0])[0], t + 1]))
    writes, last_mov = [], None
    for off, op, n in ins:
        if op in (OP["dcl_input_ps"], OP["dcl_input_ps_siv"]) or op >= 88 and op <= 106:
            continue                                                        # declarations
        ops = dxbc.operands(blob, off, n)
        if ops and ops[0][0] == dxbc.OUTPUT:
            w = dxbc.words(blob, off, n)
            assert not w[0] >> 31 and not w[1] >> 31 and w[2] == 0, "o0 destination without extended tokens"
            if ops[0][2] == {"w"}:
                last_mov = off
            else:
                assert ops[0][2] <= {"x", "y", "z"}
                writes.append(off)
    assert writes and last_mov and all(w < last_mov for w in writes)
    for off in writes:                                                      # o0.<mask> -> rT.<mask>
        tok = struct.unpack_from("<I", blob, off + 4)[0]
        out.append((DITHER, off + 4, 2, [tok & ~(0xFF << 12), t]))
    noise = (dxbc.instr("dp2", dxbc.temp(t, "w", True), dxbc.input_reg(0, "xyxx"), dxbc.imm(0.06711056, 0.00583715, 0.0, 0.0))
             + dxbc.instr("frc", dxbc.temp(t, "w", True), dxbc.temp(t, "w"))
             + dxbc.instr("mul", dxbc.temp(t, "w", True), dxbc.temp(t, "w"), dxbc.imm(52.9829189))
             + dxbc.instr("frc", dxbc.temp(t, "w", True), dxbc.temp(t, "w"))
             + dxbc.instr("add", dxbc.temp(t, "w", True), dxbc.temp(t, "w"), dxbc.imm(-0.5))
             + dxbc.instr("mad", dxbc.output(0, "xyz"), dxbc.temp(t, "wwww"), dxbc.imm(1 / 255, 1 / 255, 1 / 255, 0.0),
                          dxbc.temp(t, "xyzx")))
    out.append((DITHER, last_mov, 0, noise))
    return out


# ------------------------------------------------------------------ container

def apply_edits(blob, edits):
    """The shader with the edits applied, its SHEX/SHDR chunk, sizes and checksum updated."""
    out, pos, delta = bytearray(), 0, 0
    for off, old, words in sorted(edits, key=lambda e: e[0]):
        assert off >= pos, "overlapping edits"
        out += blob[pos:off] + dxbc.encode(words)
        pos = off + 4 * old
        delta += 4 * (len(words) - old)
    out += blob[pos:]
    count = struct.unpack_from("<I", out, 28)[0]
    offsets = [struct.unpack_from("<I", out, 32 + 4 * i)[0] for i in range(count)]
    code = next(o for o in offsets if out[o:o + 4] in (b"SHEX", b"SHDR"))
    struct.pack_into("<I", out, code + 4, struct.unpack_from("<I", out, code + 4)[0] + delta)
    struct.pack_into("<I", out, code + 12, struct.unpack_from("<I", out, code + 12)[0] + delta // 4)
    for i, o in enumerate(offsets):
        if o > code:
            struct.pack_into("<I", out, 32 + 4 * i, o + delta)
    struct.pack_into("<I", out, 24, struct.unpack_from("<I", out, 24)[0] + delta)
    out[4:20] = dxbc.checksum(out)
    return bytes(out)


def select(edits, kinds):
    return [(off, old, words) for kind, off, old, words in edits if kind & kinds]


def shaders(pfx_shader, resident_shader):
    """[(name, original blob, [(kind, offset, old dwords, words), ...])] for every listed shader."""
    paths = {"pfx_shader": pfx_shader, "resident_shader": resident_shader}
    blobs = {}
    for pack_name, path in paths.items():
        for f in pack.read(Path(path).read_bytes()).files:
            ser = f.serialized
            if f.name.endswith(".shbin"):
                assert ser[:4] == b"TPSB" and struct.unpack_from("<I", ser, 8)[0] == SHBIN_HEADER
                blobs[(pack_name, f.name[:-6])] = ser[SHBIN_HEADER:SHBIN_HEADER + dxbc.size(ser[SHBIN_HEADER:])]
    hbao = blobs[("pfx_shader", "pfx_hbao_default_p")]
    siv = next(dxbc.words(hbao, off, n) for off, op, n in dxbc.instructions(hbao) if op == OP["dcl_input_ps_siv"])
    out, seen = [], set()
    for pack_name, name, directions in SHADERS:
        blob = blobs[(pack_name, name)]
        assert dxbc.checksum(blob) == blob[4:20], f"{name}: checksum of the original"
        if blob[4:20] in seen:
            continue                                                         # identical variant
        seen.add(blob[4:20])
        if directions:
            edits = pattern_edits(blob, directions) + (edge_edits(blob) if name.endswith("_c") else [])
        elif name == "pfx_feedback_blur_p":
            edits = feedback_edits(blob)
        else:
            edits = dither_edits(blob, siv)
        edits.sort(key=lambda e: (e[1], e[0]))                               # the plugin applies them in this order
        out.append((name, blob, edits))
    return out


def header(items):
    lines = ["/* Generated by tools/gen_shader_edits.py from the game's shaders: edits only, no game code. */",
             "#pragma once", "#include <stdint.h>", ""]
    lines += [f"#define FIX_{v:<14} {k}" for k, v in FIX_NAMES.items()]
    lines += [f"#define FIX_DEFAULT        {DEFAULT}", "",
              "typedef struct { uint32_t kind, offset, old_dwords, n_words; const uint32_t *words; } ShaderEdit;",
              "typedef struct { const char *name; uint8_t checksum[16]; uint32_t size, n_edits; const ShaderEdit *edits; } ShaderFix;",
              ""]
    for s, (name, blob, edits) in enumerate(items):
        for e, (kind, off, old, words) in enumerate(edits):
            lines.append(f"static const uint32_t sw{s}_{e}[] = {{{', '.join(f'0x{w:08x}' for w in words)}}};")
        lines.append(f"static const ShaderEdit se{s}[] = {{")
        for e, (kind, off, old, words) in enumerate(edits):
            lines.append(f"    {{FIX_{FIX_NAMES[kind]}, 0x{off:x}, {old}, {len(words)}, sw{s}_{e}}},")
        lines.append("};")
    lines.append("static const ShaderFix SHADER_FIXES[] = {")
    for s, (name, blob, edits) in enumerate(items):
        cs = ", ".join(f"0x{b:02x}" for b in blob[4:20])
        lines.append(f'    {{"{name}", {{{cs}}}, {len(blob)}, {len(edits)}, se{s}}},')
    lines.append("};")
    return "\n".join(lines) + "\n"


def check(items):
    from d3dcheck import d3d
    disassemble, create = d3d()
    failed = 0
    for name, blob, edits in items:
        compute = name.endswith("_c")
        kinds = {k for k, *_ in edits}
        subsets = [{k} for k in sorted(kinds)] + ([kinds - {AO_EDGES}, kinds - {AO_EDGES_SOFT}] if len(kinds) > 2 else [])
        for subset in subsets:
            mask = sum(subset)
            fixed = apply_edits(blob, select(edits, mask))
            hr = create(fixed, compute)
            code = [l.strip() for l in (disassemble(fixed) or "").splitlines() if l.strip() and not l.startswith("//")]
            ok = hr == 0 and bool(code)
            if AO_PATTERN in kinds:
                ok &= any(l.startswith("xor") for l in code) == (AO_PATTERN not in subset)
            if subset & {AO_EDGES, AO_EDGES_SOFT}:
                ok &= sum(l.startswith("imin") for l in code) - sum(l.strip().startswith("imin") for l in
                                                                    (disassemble(blob) or "").splitlines()) == 32
            if AO_EDGES_SOFT in subset:
                ok &= sum(l.startswith("mul ") and l.endswith(", r13.x") for l in code) == 32 and "dcl_temps 14" in code
            if FEEDBACK in subset:
                ok &= not any(l.startswith("mad ") for l in code)
            if DITHER in subset:
                ok &= any(l.startswith("dcl_input_ps_siv") and "position" in l for l in code) and \
                    any(l.startswith("mad o0.xyz") for l in code)
            failed += not ok
            print(f"{'ok  ' if ok else 'FAIL'} {name} {'+'.join(FIX_NAMES[k] for k in sorted(subset))}: "
                  f"{len(blob)} -> {len(fixed)} bytes, Create{'Compute' if compute else 'Pixel'}Shader 0x{hr:08x}")
    print("all variants accepted" if not failed else f"{failed} FAILED")
    return failed == 0


def main(argv):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("pfx_shader")
    ap.add_argument("resident_shader")
    ap.add_argument("--out", default=str(Path(__file__).resolve().parent.parent / "shader_edits.h"))
    ap.add_argument("--expected")
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args(argv)
    items = shaders(args.pfx_shader, args.resident_shader)
    Path(args.out).write_text(header(items), encoding="utf-8")
    print(f"{args.out}: {len(items)} shaders, {sum(len(e) for _, _, e in items)} edits")
    if args.expected:
        d = Path(args.expected)
        d.mkdir(parents=True, exist_ok=True)
        for name, blob, edits in items:
            (d / f"orig_{name}.dxbc").write_bytes(blob)
            (d / f"fixed_{name}.dxbc").write_bytes(apply_edits(blob, select(edits, DEFAULT)))
    if args.check and not check(items):
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
