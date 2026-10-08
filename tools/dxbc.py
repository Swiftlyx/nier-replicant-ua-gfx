"""DXBC (Direct3D shader bytecode) container: chunks, checksum, SM4/SM5 instruction tokens.

Container: "DXBC", 16-byte checksum, u32 1, u32 total size, u32 chunk count, u32 chunk offsets; each
chunk is a fourcc, u32 size and data. The SHDR/SHEX chunk holds a version token, a length token
(dwords, both tokens included) and the instructions. An instruction starts with an opcode token:
bits 0-10 opcode, bits 24-30 length in dwords (customdata, opcode 53, keeps its length in the next
dword instead).

The checksum is MD5 over bytes 20.. with its own padding: the bit count goes into the first dword of
the last block and (bits >> 2) | 1 into its last dword.
"""
import struct

OPCODES = {
    "add": 0, "and": 1, "div": 14, "dp2": 15, "frc": 26, "ftoi": 27, "iadd": 30, "if": 31, "ige": 33,
    "imad": 35, "imax": 36, "imin": 37, "imul": 38, "itof": 43, "mad": 50, "max": 52, "mov": 54, "movc": 55,
    "mul": 56, "nop": 58, "sample": 69, "sincos": 77, "ult": 79, "utof": 86, "xor": 87, "customdata": 53,
    "dcl_input_ps": 98, "dcl_input_ps_siv": 100, "dcl_temps": 104,
}
SATURATE = 1 << 13              # opcode token: _sat
OUTPUT = 2                      # operand type of o#
# Flow control: liveness checks stop here (else, endif, loop, endloop, break(c), continue(c), ret(c), switch...).
FLOW = {2, 3, 4, 5, 6, 7, 8, 10, 18, 21, 22, 23, 31, 48, 62, 63, 76}
TWO_DESTS = {38, 77, 78, 81, 82}   # imul, sincos, udiv, umul, umad: two destination operands
TEMP, IMMEDIATE32, INPUT_THREAD_ID = 0, 4, 0x20

# MD5 per-round shift amounts and constants
_S = [7, 12, 17, 22] * 4 + [5, 9, 14, 20] * 4 + [4, 11, 16, 23] * 4 + [6, 10, 15, 21] * 4
_K = [int(abs(__import__("math").sin(i + 1)) * 2 ** 32) & 0xFFFFFFFF for i in range(64)]


def _rotl(x, c):
    return ((x << c) | (x >> (32 - c))) & 0xFFFFFFFF


def _md5_block(state, block):
    m = struct.unpack("<16I", block)
    a, b, c, d = state
    for i in range(64):
        if i < 16:
            f, g = (b & c) | (~b & d), i
        elif i < 32:
            f, g = (d & b) | (~d & c), (5 * i + 1) % 16
        elif i < 48:
            f, g = b ^ c ^ d, (3 * i + 5) % 16
        else:
            f, g = c ^ (b | ~d), (7 * i) % 16
        f = (f + a + _K[i] + m[g]) & 0xFFFFFFFF
        a, d, c = d, c, b
        b = (b + _rotl(f, _S[i])) & 0xFFFFFFFF
    return [(x + y) & 0xFFFFFFFF for x, y in zip(state, (a, b, c, d))]


def checksum(blob):
    data = bytes(blob[20:])
    bits = len(data) * 8
    state = [0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476]
    full = len(data) & ~63
    for i in range(0, full, 64):
        state = _md5_block(state, data[i:i + 64])
    tail = data[full:]
    if len(tail) >= 56:
        block = bytearray(64)
        block[:len(tail)] = tail
        block[len(tail)] = 0x80
        state = _md5_block(state, bytes(block))
        block = bytearray(64)
        struct.pack_into("<I", block, 0, bits)
        struct.pack_into("<I", block, 60, (bits >> 2) | 1)
    else:
        block = bytearray(64)
        struct.pack_into("<I", block, 0, bits)
        block[4:4 + len(tail)] = tail
        block[4 + len(tail)] = 0x80
        struct.pack_into("<I", block, 60, (bits >> 2) | 1)
    state = _md5_block(state, bytes(block))
    return struct.pack("<4I", *state)


def size(blob):
    return struct.unpack_from("<I", blob, 24)[0]


def chunks(blob):
    """{fourcc: (offset of the chunk data, data size)}"""
    count = struct.unpack_from("<I", blob, 28)[0]
    out = {}
    for i in range(count):
        off = struct.unpack_from("<I", blob, 32 + 4 * i)[0]
        fourcc = blob[off:off + 4].decode("ascii")
        out[fourcc] = (off + 8, struct.unpack_from("<I", blob, off + 4)[0])
    return out


def code_range(blob):
    """(offset of the first instruction, offset after the last) of the SHDR/SHEX chunk."""
    c = chunks(blob)
    off, _ = c.get("SHEX") or c["SHDR"]
    length = struct.unpack_from("<I", blob, off + 4)[0]
    return off + 8, off + 4 * length


def instructions(blob):
    """[(offset, opcode, dword length)] of every instruction, declarations included."""
    start, end = code_range(blob)
    out, at = [], start
    while at < end:
        tok = struct.unpack_from("<I", blob, at)[0]
        op = tok & 0x7FF
        n = struct.unpack_from("<I", blob, at + 4)[0] if op == OPCODES["customdata"] else (tok >> 24) & 0x7F
        out.append((at, op, n))
        at += 4 * n
    return out


def operands(blob, off, n):
    """[(type, register index or None, components)] of one instruction; components are the written mask
    for destinations and the swizzled/selected components for sources."""
    words = struct.unpack_from(f"<{n}I", blob, off)
    i = 1
    if words[0] >> 31:                      # extended opcode tokens
        while words[i] >> 31:
            i += 1
        i += 1
    out = []
    while i < n:
        t = words[i]
        i += 1
        if t >> 31:                         # extended operand token (modifiers)
            i += 1
        ncomp, mode, sel = t & 3, (t >> 2) & 3, (t >> 4) & 0xFF
        typ, dims = (t >> 12) & 0xFF, (t >> 20) & 3
        if ncomp == 2:
            if mode == 0:
                comps = {c for k, c in enumerate(_COMP) if sel >> k & 1}
            elif mode == 1:
                comps = {_COMP[sel >> (2 * k) & 3] for k in range(4)}
            else:
                comps = {_COMP[sel & 3]}
        else:
            comps = set()
        index = None
        for d in range(dims):
            rep = (t >> (22 + 3 * d)) & 7
            assert rep == 0, "relative or 64-bit index"
            if d == 0:
                index = words[i]
            i += 1
        if typ == IMMEDIATE32:
            i += 1 if ncomp == 1 else 4
        out.append((typ, index, comps))
    return out


def dead_after(blob, after, reg, comps):
    """True when temp `reg` components `comps` are written before being read, starting at offset `after`
    and staying in straight-line code (the components are scratch at that point)."""
    left = set(comps)
    for off, op, n in instructions(blob):
        if off < after:
            continue
        if op in FLOW:
            return False
        ops = operands(blob, off, n)
        dests = ops[:2] if op in TWO_DESTS else ops[:1]
        for typ, index, used in ops[len(dests):]:
            if typ == TEMP and index == reg and used & left:
                return False
        for typ, index, written in dests:
            if typ == TEMP and index == reg:
                left -= written
        if not left:
            return True
    return False


# ------------------------------------------------------------------ encoding

_COMP = "xyzw"


def temp(index, comps, dst=False):
    """Temp register operand: dst uses a write mask, a source a swizzle (or select-1 for one component)."""
    if dst:
        mask = sum(1 << _COMP.index(c) for c in comps)
        return [0x00100002 | (mask << 4), index]
    if len(comps) == 1:
        return [0x00100002 | (2 << 2) | (_COMP.index(comps) << 4), index]
    sw = (comps + comps[-1] * 4)[:4]
    bits = sum(_COMP.index(c) << (2 * i) for i, c in enumerate(sw))
    return [0x00100002 | (1 << 2) | (bits << 4), index]


def input_reg(index, comps):
    t = temp(index, comps)
    t[0] |= 1 << 12
    return t


def thread_id(comps):
    """vThreadID source operand (no register index)."""
    t = temp(0, comps)[0]
    return [(t & ~(3 << 20)) | (INPUT_THREAD_ID << 12)]


def imm(*values):
    words = [struct.unpack("<I", struct.pack("<f", v))[0] if isinstance(v, float) else v & 0xFFFFFFFF
             for v in values]
    return [0x00004001 if len(words) == 1 else 0x00004002] + words


def neg(operand):
    """The operand with the negate modifier (an extended operand token)."""
    return [operand[0] | 0x80000000, 0x00000041] + operand[1:]


def output(index, comps):
    """Output register destination o#.mask."""
    t = temp(index, comps, True)
    t[0] |= OUTPUT << 12
    return t


def instr(name, *operands, sat=False):
    body = [w for op in operands for w in op]
    return [OPCODES[name] | ((1 + len(body)) << 24) | (SATURATE if sat else 0)] + body


def words(blob, off, n):
    return list(struct.unpack_from(f"<{n}I", blob, off))


def encode(words):
    return struct.pack(f"<{len(words)}I", *words)
