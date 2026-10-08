"""Shader tools on Windows: D3DCompile and D3DDisassemble (d3dcompiler_47) and shader creation on a WARP
D3D11 device, which validates the bytecode and its checksum like the game's device does."""
import ctypes


def _blob_bytes(blob):
    """Contents of an ID3DBlob (GetBufferPointer, GetBufferSize), then Release."""
    vt = ctypes.cast(ctypes.cast(blob, ctypes.POINTER(ctypes.c_void_p))[0], ctypes.POINTER(ctypes.c_void_p))
    ptr = ctypes.WINFUNCTYPE(ctypes.c_void_p, ctypes.c_void_p)(vt[3])(blob)
    n = ctypes.WINFUNCTYPE(ctypes.c_size_t, ctypes.c_void_p)(vt[4])(blob)
    data = ctypes.string_at(ptr, n)
    ctypes.WINFUNCTYPE(ctypes.c_ulong, ctypes.c_void_p)(vt[2])(blob)
    return data


def compile_hlsl(source, name, entry, target, flags, defines=None):
    """(bytecode or None, compiler messages); defines: {macro: value}"""
    compiler = ctypes.WinDLL("d3dcompiler_47.dll")
    code, errors = ctypes.c_void_p(), ctypes.c_void_p()
    macros = None
    if defines:
        pairs = [(k.encode(), str(v).encode()) for k, v in defines.items()] + [(None, None)]
        macros = (ctypes.c_char_p * (2 * len(pairs)))(*[s for pair in pairs for s in pair])
    hr = compiler.D3DCompile(source, len(source), name.encode(), macros, None, entry.encode(), target.encode(),
                             flags, 0, ctypes.byref(code), ctypes.byref(errors))
    messages = _blob_bytes(errors).decode("latin-1").rstrip("\0") if errors.value else ""
    return (_blob_bytes(code) if hr == 0 and code.value else None), messages


def d3d():
    """(disassemble(blob) -> str or None, create(blob, compute) -> HRESULT as unsigned)"""
    compiler = ctypes.WinDLL("d3dcompiler_47.dll")
    d3d11 = ctypes.WinDLL("d3d11.dll")

    def disassemble(blob):
        out = ctypes.c_void_p()
        if compiler.D3DDisassemble(blob, len(blob), 0, None, ctypes.byref(out)):
            return None
        return _blob_bytes(out).decode("latin-1")

    device = ctypes.c_void_p()
    hr = d3d11.D3D11CreateDevice(None, 5, None, 0, None, 0, 7, ctypes.byref(device), None, None)  # WARP
    assert hr == 0, hex(hr & 0xFFFFFFFF)
    vt = ctypes.cast(ctypes.cast(device, ctypes.POINTER(ctypes.c_void_p))[0], ctypes.POINTER(ctypes.c_void_p))
    proto = ctypes.WINFUNCTYPE(ctypes.c_long, ctypes.c_void_p, ctypes.c_char_p, ctypes.c_size_t,
                               ctypes.c_void_p, ctypes.POINTER(ctypes.c_void_p))
    create_ps, create_cs = proto(vt[15]), proto(vt[18])   # ID3D11Device::CreatePixelShader / CreateComputeShader

    def create(blob, compute):
        shader = ctypes.c_void_p()
        return (create_cs if compute else create_ps)(device, blob, len(blob), None, ctypes.byref(shader)) & 0xFFFFFFFF

    return disassemble, create
