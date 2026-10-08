"""Reader of the game's PACK containers (the layout follows libreplicant's pack.cpp).

Offsets inside PACK are relative to the field that stores them. Resource
offsets are relative to the end of the serialized block and carry a has_data
flag in bit 31.
"""
import struct
from dataclasses import dataclass, field

HEADER = struct.Struct("<4s10I")


@dataclass
class PackFile:
    name_hash: int
    name: str
    serialized: bytes
    resource: bytes = b""


@dataclass
class Pack:
    version: int
    imports: list = field(default_factory=list)      # (hash, path, unknown)
    asset_packages: list = field(default_factory=list)  # (hash, name, content)
    files: list = field(default_factory=list)

    def find(self, suffix):
        return next(f for f in self.files if f.name.endswith(suffix))


def _rel(buf, off):
    return off + struct.unpack_from("<I", buf, off)[0]


def _cstr(buf, off):
    return buf[off:buf.index(b"\0", off)].decode("utf-8")


def read(data):
    (magic, version, total, ser_size, res_size, n_imp, _, n_ap, _, n_files, _) = HEADER.unpack_from(data, 0)
    assert magic == b"PACK", "not a PACK file"
    pack = Pack(version)
    if n_imp:
        base = _rel(data, 0x18)
        for i in range(n_imp):
            o = base + i * 12
            h, _, unk = struct.unpack_from("<3I", data, o)
            pack.imports.append((h, _cstr(data, _rel(data, o + 4)), unk))
    if n_ap:
        base = _rel(data, 0x20)
        for i in range(n_ap):
            o = base + i * 20
            h, _, size, _, _ = struct.unpack_from("<5I", data, o)
            start = _rel(data, o + 12)
            pack.asset_packages.append((h, _cstr(data, _rel(data, o + 4)), data[start:start + size]))
    res_entries = []
    if n_files:
        base = _rel(data, 0x28)
        for i in range(n_files):
            o = base + i * 20
            h, _, size, _, data_off = struct.unpack_from("<5I", data, o)
            start = _rel(data, o + 12)
            pack.files.append(PackFile(h, _cstr(data, _rel(data, o + 4)), data[start:start + size]))
            if data_off >> 31:
                res_entries.append((data_off & 0x7FFFFFFF, i))
    res_entries.sort()
    for k, (off, i) in enumerate(res_entries):
        end = res_entries[k + 1][0] if k + 1 < len(res_entries) else res_size
        pack.files[i].resource = data[ser_size + off:ser_size + end]
    return pack
