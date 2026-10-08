/*
 * NierReplicantGFX: graphics fixes for NieR Replicant ver.1.22474487139.
 * Author: Swiftlyx. License: MIT.
 *
 * Ways to load it:
 *   - xinput9_1_0.dll in the game folder. The game imports it, so Windows loads this file before any
 *     game code runs; the XInput exports forward to the system xinput9_1_0.dll;
 *   - a Lunar Tear mod DLL (LunarTearPluginInit is exported, the plugin API is not used);
 *   - an .asi for Ultimate ASI Loader or Special K.
 *
 * Shaders: the game's D3D11CreateDevice import is redirected, and on the device it returns the texture,
 * view and shader creation methods are wrapped. A shader whose DXBC checksum and size are listed in
 * shader_edits.h gets the edits of the enabled fixes (described in tools/gen_shader_edits.py), and the
 * container sizes and checksum are recomputed. With FIX_AO_SHADER the game's AO shader is replaced by
 * the plugin's own (shaders/ssao_mask.hlsl, compiled into ao_shader.h). Other shaders pass through.
 *
 * Supersampling: exe+0x7D40F0 fits the output size to 16:9 and stores it as the render size
 * ([display+0x174], used for every render target); a stub at exe+0x7D425E then calls
 * scale_render_size(). The final pass (exe+0x8B2F50) centres the picture by |output / 2 - render / 2|
 * (exe+0x8B31FB, exe+0x8B3236); both sites take the fitted 16:9 rectangle (xmm13 / xmm14) instead of
 * the render size, so a larger render size stays centred. The final shader samples the scene once per
 * output pixel, so a factor of 2 averages exactly 2x2 pixels.
 *
 * Settings: NierReplicantGFX.ini next to the DLL. Log: NierReplicantGFX.log next to the DLL.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COBJMACROS
#include <d3d11.h>

#include "shader_edits.h"
#include "ao_shader.h"
#include "common.h"
#include "version.h"

#define RENDER_SIZE   0x7D425Eu
#define FIT_OFFSET_X  0x8B31FBu
#define FIT_OFFSET_Y  0x8B3236u
#define DISPLAY_SIZE  0x174u          /* render width and height in the display object */
#define SCALE_MAX     4.0f
#define STRENGTH_MAX  4.0f
#define ENV_MARKER    L"NIER_REPLICANT_GFX_LOADED"
#define CONFIG_NAME   L"NierReplicantGFX.ini"
#define LOG_NAME      L"NierReplicantGFX.log"
#define GFX_DEFAULT   (FIX_DEFAULT | FIX_AO_SHADER)

/* Result bits of NierGFX_Apply (also used by the test). */
#define R_SSAA   0x01
#define R_DEVICE 0x02

static const Site SSAA_SITES[] = {
    {RENDER_SIZE, "488d4b284c8b8374010000", NULL},   /* lea rcx, [rbx+0x28]; mov r8, [rbx+0x174] */
    {FIT_OFFSET_X, "660f6e4424680f5bc0", NULL},       /* movd xmm0, [rsp+0x68]; cvtdq2ps xmm0, xmm0 */
    {FIT_OFFSET_Y, "660f6e44246c0f5bc0", NULL},       /* movd xmm0, [rsp+0x6c]; cvtdq2ps xmm0, xmm0 */
};
static const char FIT_X_NEW[] = "410f28c50f1f440000"; /* movaps xmm0, xmm13; nop */
static const char FIT_Y_NEW[] = "410f28c60f1f440000"; /* movaps xmm0, xmm14; nop */

static unsigned g_fixes = GFX_DEFAULT;
static int g_high_precision = 1;
static float g_ao_strength = 1.0f;
static float g_render_scale = 1.0f;
static int g_max_height = 2160;
static volatile LONG g_upgraded;     /* 10-bit textures created at 16 bits */
static volatile LONG g_patched;      /* bit per SHADER_FIXES entry: changed at least once */

/* ------------------------------------------------------------------ DXBC */

static const uint32_t MD5_K[64] = {
    0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
    0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be, 0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
    0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa, 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
    0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
    0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c, 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
    0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05, 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
    0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
    0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1, 0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391,
};
static const uint8_t MD5_S[16] = {7, 12, 17, 22, 5, 9, 14, 20, 4, 11, 16, 23, 6, 10, 15, 21};

static void md5_block(uint32_t st[4], const uint8_t *blk)
{
    uint32_t m[16], a = st[0], b = st[1], c = st[2], d = st[3];
    int i;
    memcpy(m, blk, 64);
    for (i = 0; i < 64; i++) {
        uint32_t f, g, t;
        if (i < 16) {
            f = (b & c) | (~b & d);
            g = i;
        } else if (i < 32) {
            f = (d & b) | (~d & c);
            g = (5 * i + 1) & 15;
        } else if (i < 48) {
            f = b ^ c ^ d;
            g = (3 * i + 5) & 15;
        } else {
            f = c ^ (b | ~d);
            g = (7 * i) & 15;
        }
        f += a + MD5_K[i] + m[g];
        t = MD5_S[(i >> 4) * 4 + (i & 3)];
        a = d;
        d = c;
        c = b;
        b += (f << t) | (f >> (32 - t));
    }
    st[0] += a;
    st[1] += b;
    st[2] += c;
    st[3] += d;
}

/* DXBC checksum: MD5 over bytes 20.. with the bit count in the first dword of the last block and
   (bits >> 2) | 1 in its last dword. */
static void dxbc_checksum(const uint8_t *blob, size_t size, uint8_t out[16])
{
    const uint8_t *data = blob + 20;
    size_t n = size - 20, full = n & ~(size_t)63, tail = n - full, i;
    uint32_t bits = (uint32_t)(n * 8), last = (bits >> 2) | 1;
    uint32_t st[4] = {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476};
    uint8_t blk[64];
    for (i = 0; i < full; i += 64)
        md5_block(st, data + i);
    memset(blk, 0, sizeof blk);
    if (tail >= 56) {
        memcpy(blk, data + full, tail);
        blk[tail] = 0x80;
        md5_block(st, blk);
        memset(blk, 0, sizeof blk);
        memcpy(blk, &bits, 4);
    } else {
        memcpy(blk, &bits, 4);
        memcpy(blk + 4, data + full, tail);
        blk[4 + tail] = 0x80;
    }
    memcpy(blk + 60, &last, 4);
    md5_block(st, blk);
    memcpy(out, st, 16);
}

static uint32_t rd32(const uint8_t *p)
{
    uint32_t v;
    memcpy(&v, p, 4);
    return v;
}

static void wr32(uint8_t *p, uint32_t v)
{
    memcpy(p, &v, 4);
}

/* 1 when the SHADER_FIXES entry `index` is replaced by the plugin's own shader under `fixes`. */
static int replaced(int index, unsigned fixes)
{
    return (fixes & FIX_AO_SHADER) && strcmp(SHADER_FIXES[index].name, AO_SHADER_REPLACES) == 0;
}

/* A copy of the plugin's AO shader with the current strength. */
static uint8_t *own_ao_shader(size_t *out_size)
{
    uint8_t *out = HeapAlloc(GetProcessHeap(), 0, sizeof AO_SHADER);
    if (!out)
        return NULL;
    memcpy(out, AO_SHADER, sizeof AO_SHADER);
    if (g_ao_strength != 1.0f) {
        memcpy(out + AO_SHADER_STRENGTH, &g_ao_strength, 4);
        dxbc_checksum(out, sizeof AO_SHADER, out + 4);
    }
    *out_size = sizeof AO_SHADER;
    return out;
}

/*
 * The shader to create instead of `code` (HeapAlloc, free with NierGFX_FreeShader): `code` with the
 * edits that `fixes` selects, or the plugin's AO shader when it replaces this one. NULL when `code` is
 * not listed in SHADER_FIXES; *index gets its entry otherwise.
 */
__declspec(dllexport) uint8_t *NierGFX_PatchShader(const uint8_t *code, size_t size, unsigned fixes, size_t *out_size,
                                                   int *index)
{
    const ShaderFix *s = NULL;
    const uint8_t *src;
    uint8_t *out, *dst;
    size_t i, new_size, count, code_chunk = 0;
    long delta = 0;
    int k;

    if (!code || size < 32 || memcmp(code, "DXBC", 4) != 0)
        return NULL;
    for (k = 0; k < (int)(sizeof SHADER_FIXES / sizeof SHADER_FIXES[0]); k++) {
        if (SHADER_FIXES[k].size == size && memcmp(SHADER_FIXES[k].checksum, code + 4, 16) == 0) {
            s = &SHADER_FIXES[k];
            break;
        }
    }
    if (!s)
        return NULL;
    if (index)
        *index = k;
    if (replaced(k, fixes))
        return own_ao_shader(out_size);

    for (i = 0; i < s->n_edits; i++)
        if (s->edits[i].kind & fixes)
            delta += 4 * ((long)s->edits[i].n_words - (long)s->edits[i].old_dwords);
    new_size = (size_t)((long)size + delta);
    out = HeapAlloc(GetProcessHeap(), 0, new_size);
    if (!out)
        return NULL;

    /* edits are sorted by offset */
    src = code;
    dst = out;
    for (i = 0; i < s->n_edits; i++) {
        const ShaderEdit *e = &s->edits[i];
        if (!(e->kind & fixes))
            continue;
        memcpy(dst, src, (size_t)(code + e->offset - src));
        dst += code + e->offset - src;
        memcpy(dst, e->words, 4 * e->n_words);
        dst += 4 * e->n_words;
        src = code + e->offset + 4 * e->old_dwords;
    }
    memcpy(dst, src, (size_t)(code + size - src));

    /* the SHEX/SHDR chunk grows by delta: its size and length token, later chunk offsets, total size */
    count = rd32(out + 28);
    for (i = 0; i < count; i++) {
        uint32_t off = rd32(out + 32 + 4 * i);
        if (memcmp(out + off, "SHEX", 4) == 0 || memcmp(out + off, "SHDR", 4) == 0)
            code_chunk = off;
    }
    if (!code_chunk) {
        HeapFree(GetProcessHeap(), 0, out);
        return NULL;
    }
    wr32(out + code_chunk + 4, rd32(out + code_chunk + 4) + (uint32_t)delta);
    wr32(out + code_chunk + 12, rd32(out + code_chunk + 12) + (uint32_t)(delta / 4));
    for (i = 0; i < count; i++) {
        uint32_t off = rd32(out + 32 + 4 * i);
        if (off > code_chunk)
            wr32(out + 32 + 4 * i, off + (uint32_t)delta);
    }
    wr32(out + 24, (uint32_t)new_size);
    dxbc_checksum(out, new_size, out + 4);
    *out_size = new_size;
    return out;
}

__declspec(dllexport) void NierGFX_FreeShader(uint8_t *code)
{
    if (code)
        HeapFree(GetProcessHeap(), 0, code);
}

/* ------------------------------------------------------------------ D3D11 hooks */

typedef HRESULT(STDMETHODCALLTYPE *CreateShader_t)(void *device, const void *code, SIZE_T size, void *linkage,
                                                   void **shader);
typedef HRESULT(STDMETHODCALLTYPE *CreateTexture2D_t)(ID3D11Device *device, const D3D11_TEXTURE2D_DESC *desc,
                                                      const D3D11_SUBRESOURCE_DATA *init, ID3D11Texture2D **texture);
typedef HRESULT(STDMETHODCALLTYPE *CreateSrv_t)(ID3D11Device *, ID3D11Resource *, const D3D11_SHADER_RESOURCE_VIEW_DESC *,
                                                ID3D11ShaderResourceView **);
typedef HRESULT(STDMETHODCALLTYPE *CreateUav_t)(ID3D11Device *, ID3D11Resource *, const D3D11_UNORDERED_ACCESS_VIEW_DESC *,
                                                ID3D11UnorderedAccessView **);
typedef HRESULT(STDMETHODCALLTYPE *CreateRtv_t)(ID3D11Device *, ID3D11Resource *, const D3D11_RENDER_TARGET_VIEW_DESC *,
                                                ID3D11RenderTargetView **);
typedef HRESULT(WINAPI *D3D11CreateDevice_t)(void *adapter, int driver, HMODULE software, UINT flags,
                                             const int *levels, UINT n_levels, UINT sdk, void **device,
                                             int *level, void **context);

/* ID3D11Device vtable slots wrapped here */
enum { H_TEXTURE2D, H_SRV, H_UAV, H_RTV, H_PS, H_CS, N_HOOKS };
static const int SLOT[N_HOOKS] = {5, 7, 8, 9, 15, 18};

#define MAX_VTABLES 4
static struct {
    void **vtable;
    void *original[N_HOOKS];
} g_vt[MAX_VTABLES];
static int g_nvt;
static CRITICAL_SECTION g_lock;
static D3D11CreateDevice_t g_create_device;

static void *original(void *device, int hook)
{
    void **vt = *(void ***)device;
    int i;
    for (i = 0; i < g_nvt; i++)
        if (g_vt[i].vtable == vt)
            return g_vt[i].original[hook];
    return NULL;
}

/* --- shaders */

static HRESULT create_shader(void *device, const void *code, SIZE_T size, void *linkage, void **shader, int compute)
{
    CreateShader_t create = (CreateShader_t)original(device, compute ? H_CS : H_PS);
    size_t new_size;
    int index;
    uint8_t *fixed;
    HRESULT hr;

    if (!create)
        return E_FAIL;
    fixed = NierGFX_PatchShader(code, size, g_fixes, &new_size, &index);
    if (!fixed)
        return create(device, code, size, linkage, shader);
    hr = create(device, fixed, new_size, linkage, shader);
    NierGFX_FreeShader(fixed);
    if (SUCCEEDED(hr)) {
        if (!(InterlockedOr(&g_patched, 1L << index) & (1L << index)))
            log_line(replaced(index, g_fixes) ? "shader   replaced: %s -> NierReplicantGFX AO" : "shader   fixed: %s",
                     SHADER_FIXES[index].name);
    } else {
        log_line("shader   %s rejected by the device (0x%08lx), the original is used", SHADER_FIXES[index].name,
                 (unsigned long)hr);
        hr = create(device, code, size, linkage, shader);
    }
    return hr;
}

static HRESULT STDMETHODCALLTYPE hook_create_ps(void *device, const void *code, SIZE_T size, void *linkage, void **shader)
{
    return create_shader(device, code, size, linkage, shader, 0);
}

static HRESULT STDMETHODCALLTYPE hook_create_cs(void *device, const void *code, SIZE_T size, void *linkage, void **shader)
{
    return create_shader(device, code, size, linkage, shader, 1);
}

/* --- 16-bit colour buffers
 *
 * The game draws the frame into 10-bit linear R10G10B10A2_UNORM buffers, and the final pass converts it
 * to sRGB. Near black one 10-bit step is about 3 levels of the 8-bit output, so dark glows and gradients
 * show rings. Default-usage 10-bit textures without initial data are created as R16G16B16A16 (UNORM, or
 * TYPELESS for TYPELESS); views that ask for R10G10B10A2_UNORM on such a texture get R16G16B16A16_UNORM.
 * UNORM keeps the 0..1 clamp of the original format: some passes write values below zero that the game
 * expects to be clamped (with a FLOAT buffer the name entry screen lost its background and half of its
 * text). Copies stay between textures of the same format because all of them are converted.
 */

static int is_10bit(DXGI_FORMAT f)
{
    return f == DXGI_FORMAT_R10G10B10A2_UNORM || f == DXGI_FORMAT_R10G10B10A2_TYPELESS;
}

static HRESULT STDMETHODCALLTYPE hook_create_texture2d(ID3D11Device *device, const D3D11_TEXTURE2D_DESC *desc,
                                                       const D3D11_SUBRESOURCE_DATA *init, ID3D11Texture2D **texture)
{
    CreateTexture2D_t create = (CreateTexture2D_t)original(device, H_TEXTURE2D);
    const UINT keep = D3D11_RESOURCE_MISC_SHARED | D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX |
                      D3D11_RESOURCE_MISC_GDI_COMPATIBLE;
    if (!create)
        return E_FAIL;
    if (g_high_precision && desc && !init && is_10bit(desc->Format) && desc->Usage == D3D11_USAGE_DEFAULT &&
        !desc->CPUAccessFlags && !(desc->MiscFlags & keep)) {
        D3D11_TEXTURE2D_DESC d = *desc;
        HRESULT hr;
        d.Format = desc->Format == DXGI_FORMAT_R10G10B10A2_UNORM ? DXGI_FORMAT_R16G16B16A16_UNORM
                                                                 : DXGI_FORMAT_R16G16B16A16_TYPELESS;
        hr = create(device, &d, init, texture);
        if (SUCCEEDED(hr)) {
            if (InterlockedIncrement(&g_upgraded) <= 16)
                log_line("precision %ux%u R10G10B10A2 -> R16G16B16A16 (bind 0x%x)", d.Width, d.Height, d.BindFlags);
            return hr;
        }
    }
    return create(device, desc, init, texture);
}

/* 1 when `resource` is a 2D texture with 16 bits per channel. */
static int upgraded(ID3D11Resource *resource)
{
    D3D11_RESOURCE_DIMENSION dim;
    D3D11_TEXTURE2D_DESC d;
    if (!resource)
        return 0;
    ID3D11Resource_GetType(resource, &dim);
    if (dim != D3D11_RESOURCE_DIMENSION_TEXTURE2D)
        return 0;
    ID3D11Texture2D_GetDesc((ID3D11Texture2D *)resource, &d);
    return d.Format == DXGI_FORMAT_R16G16B16A16_UNORM || d.Format == DXGI_FORMAT_R16G16B16A16_TYPELESS;
}

static HRESULT STDMETHODCALLTYPE hook_create_srv(ID3D11Device *device, ID3D11Resource *resource,
                                                 const D3D11_SHADER_RESOURCE_VIEW_DESC *desc,
                                                 ID3D11ShaderResourceView **view)
{
    CreateSrv_t create = (CreateSrv_t)original(device, H_SRV);
    if (!create)
        return E_FAIL;
    if (desc && desc->Format == DXGI_FORMAT_R10G10B10A2_UNORM && upgraded(resource)) {
        D3D11_SHADER_RESOURCE_VIEW_DESC d = *desc;
        d.Format = DXGI_FORMAT_R16G16B16A16_UNORM;
        return create(device, resource, &d, view);
    }
    return create(device, resource, desc, view);
}

static HRESULT STDMETHODCALLTYPE hook_create_uav(ID3D11Device *device, ID3D11Resource *resource,
                                                 const D3D11_UNORDERED_ACCESS_VIEW_DESC *desc,
                                                 ID3D11UnorderedAccessView **view)
{
    CreateUav_t create = (CreateUav_t)original(device, H_UAV);
    if (!create)
        return E_FAIL;
    if (desc && desc->Format == DXGI_FORMAT_R10G10B10A2_UNORM && upgraded(resource)) {
        D3D11_UNORDERED_ACCESS_VIEW_DESC d = *desc;
        d.Format = DXGI_FORMAT_R16G16B16A16_UNORM;
        return create(device, resource, &d, view);
    }
    return create(device, resource, desc, view);
}

static HRESULT STDMETHODCALLTYPE hook_create_rtv(ID3D11Device *device, ID3D11Resource *resource,
                                                 const D3D11_RENDER_TARGET_VIEW_DESC *desc,
                                                 ID3D11RenderTargetView **view)
{
    CreateRtv_t create = (CreateRtv_t)original(device, H_RTV);
    if (!create)
        return E_FAIL;
    if (desc && desc->Format == DXGI_FORMAT_R10G10B10A2_UNORM && upgraded(resource)) {
        D3D11_RENDER_TARGET_VIEW_DESC d = *desc;
        d.Format = DXGI_FORMAT_R16G16B16A16_UNORM;
        return create(device, resource, &d, view);
    }
    return create(device, resource, desc, view);
}

/* Wraps the device's texture, view and shader creation (once per vtable). */
__declspec(dllexport) int NierGFX_HookDevice(void *device)
{
    void *hooks[N_HOOKS] = {(void *)hook_create_texture2d, (void *)hook_create_srv, (void *)hook_create_uav,
                            (void *)hook_create_rtv, (void *)hook_create_ps, (void *)hook_create_cs};
    void **vt = *(void ***)device;
    int i, ok = 1;
    DWORD old;
    EnterCriticalSection(&g_lock);
    for (i = 0; i < g_nvt; i++)
        if (g_vt[i].vtable == vt)
            goto done;
    if (g_nvt == MAX_VTABLES || vt[SLOT[H_PS]] == hooks[H_PS]) {
        ok = 0;
        goto done;
    }
    if (!VirtualProtect(&vt[SLOT[0]], (SLOT[N_HOOKS - 1] - SLOT[0] + 1) * sizeof(void *), PAGE_READWRITE, &old)) {
        ok = 0;
        goto done;
    }
    g_vt[g_nvt].vtable = vt;
    for (i = 0; i < N_HOOKS; i++) {
        g_vt[g_nvt].original[i] = vt[SLOT[i]];
        vt[SLOT[i]] = hooks[i];
    }
    VirtualProtect(&vt[SLOT[0]], (SLOT[N_HOOKS - 1] - SLOT[0] + 1) * sizeof(void *), old, &old);
    g_nvt++;
done:
    LeaveCriticalSection(&g_lock);
    return ok;
}

__declspec(dllexport) unsigned NierGFX_PatchedShaders(void)
{
    return (unsigned)g_patched;
}

static HRESULT WINAPI hook_d3d11_create_device(void *adapter, int driver, HMODULE software, UINT flags,
                                               const int *levels, UINT n_levels, UINT sdk, void **device,
                                               int *level, void **context)
{
    HRESULT hr = g_create_device(adapter, driver, software, flags, levels, n_levels, sdk, device, level, context);
    if (SUCCEEDED(hr) && device && *device) {
        if (NierGFX_HookDevice(*device))
            log_line("shaders  device created at %u ms, creation methods wrapped", ms_since_start());
        else
            log_line("shaders  device created, but its methods could not be wrapped");
    }
    return hr;
}

/* RVA of the import address table slot of `dll`!`func` in the image at `base`, 0 when absent. */
__declspec(dllexport) uint32_t NierGFX_FindImport(uint8_t *base, const char *dll, const char *func)
{
    IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(base + ((IMAGE_DOS_HEADER *)base)->e_lfanew);
    IMAGE_DATA_DIRECTORY dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    IMAGE_IMPORT_DESCRIPTOR *d;
    if (!dir.VirtualAddress)
        return 0;
    for (d = (IMAGE_IMPORT_DESCRIPTOR *)(base + dir.VirtualAddress); d->Name; d++) {
        IMAGE_THUNK_DATA64 *names;
        uint32_t i;
        if (_stricmp((const char *)(base + d->Name), dll) != 0)
            continue;
        names = (IMAGE_THUNK_DATA64 *)(base + (d->OriginalFirstThunk ? d->OriginalFirstThunk : d->FirstThunk));
        for (i = 0; names[i].u1.AddressOfData; i++) {
            IMAGE_IMPORT_BY_NAME *by_name;
            if (IMAGE_SNAP_BY_ORDINAL64(names[i].u1.Ordinal))
                continue;
            by_name = (IMAGE_IMPORT_BY_NAME *)(base + names[i].u1.AddressOfData);
            if (strcmp((const char *)by_name->Name, func) == 0)
                return d->FirstThunk + i * (uint32_t)sizeof(IMAGE_THUNK_DATA64);
        }
    }
    return 0;
}

static int hook_create_device(uint8_t *base)
{
    uint32_t slot = NierGFX_FindImport(base, "d3d11.dll", "D3D11CreateDevice");
    void *hook = (void *)hook_d3d11_create_device;
    if (!slot)
        return 0;
    g_create_device = *(D3D11CreateDevice_t *)(base + slot);
    if (!g_create_device)
        return 0;
    return write_bytes(base + slot, &hook, sizeof hook);
}

/* ------------------------------------------------------------------ supersampling */

/* Called from the stub at exe+0x7D425E with the display object: scales the fitted render size. */
static void scale_render_size(uint8_t *display)
{
    int32_t *wh = (int32_t *)(display + DISPLAY_SIZE);
    float s = g_render_scale;
    int32_t w, h;
    if (wh[0] <= 0 || wh[1] <= 0)
        return;
    if (g_max_height > 0 && (float)wh[1] * s > (float)g_max_height)
        s = (float)g_max_height / (float)wh[1];
    if (s <= 1.0f) {
        log_line("ssaa     render size %dx%d kept (MaxRenderHeight = %d)", wh[0], wh[1], g_max_height);
        return;
    }
    w = (int32_t)((float)wh[0] * s + 0.5f);
    h = (int32_t)((float)wh[1] * s + 0.5f);
    log_line("ssaa     render size %dx%d -> %dx%d", wh[0], wh[1], w, h);
    wh[0] = w;
    wh[1] = h;
}

/*
 * Stub called at exe+0x7D425E (rbx = display object; rsp is 16-byte aligned at the call site, so 8 on
 * entry): calls scale_render_size(rbx), then runs the two instructions the call replaced. The volatile
 * registers it clobbers are free there: rcx and r8 are set by those instructions, rdx right after.
 */
static int apply_ssaa(uint8_t *base)
{
    uint8_t *stub = alloc_near(base), *p, call[11], fx[9], fy[9];
    void *fn = (void *)scale_render_size;
    int32_t d;
    if (!stub)
        return 0;
    p = stub;
    p += unhex("4883ec28", p, 4);                 /* sub rsp, 0x28 */
    p += unhex("488bcb", p, 3);                   /* mov rcx, rbx */
    *p++ = 0xFF;                                  /* call [rip+slot] */
    *p++ = 0x15;
    if (!rel32(p + 4, stub + 0x40, &d))
        return 0;
    memcpy(p, &d, 4);
    p += 4;
    p += unhex("4883c428", p, 4);                 /* add rsp, 0x28 */
    p += unhex(SSAA_SITES[0].orig, p, 11);        /* lea rcx, [rbx+0x28]; mov r8, [rbx+0x174] */
    *p++ = 0xC3;                                  /* ret */
    memcpy(stub + 0x40, &fn, sizeof fn);
    FlushInstructionCache(GetCurrentProcess(), stub, 0x48);

    call[0] = 0xE8;
    if (!rel32(base + RENDER_SIZE + 5, stub, &d))
        return 0;
    memcpy(call + 1, &d, 4);
    memset(call + 5, 0x90, 6);
    unhex(FIT_X_NEW, fx, sizeof fx);
    unhex(FIT_Y_NEW, fy, sizeof fy);
    return write_bytes(base + FIT_OFFSET_X, fx, sizeof fx) && write_bytes(base + FIT_OFFSET_Y, fy, sizeof fy) &&
           write_bytes(base + RENDER_SIZE, call, sizeof call);
}

/* ------------------------------------------------------------------ entry points */

/* Settings for NierGFX_Apply and the shader hooks (the plugin reads them from the ini, the test sets them). */
__declspec(dllexport) void NierGFX_Configure(unsigned fixes, int high_precision, float ao_strength, float render_scale,
                                             int max_height)
{
    g_fixes = fixes;
    g_high_precision = high_precision;
    g_ao_strength = ao_strength;
    g_render_scale = render_scale;
    g_max_height = max_height;
}

/* Patches the game image at `base`: supersampling and the D3D11CreateDevice import. */
__declspec(dllexport) unsigned NierGFX_Apply(uint8_t *base)
{
    unsigned result = 0;
    if (g_render_scale <= 1.0f) {
        log_line("ssaa     off (RenderScale = 1.0)");
    } else if (check_group(base, SSAA_SITES, 3, R_SSAA, "ssaa", &result)) {
        if (apply_ssaa(base)) {
            result |= R_SSAA;
            log_line("ssaa     applied: RenderScale %.2f, MaxRenderHeight %d", g_render_scale, g_max_height);
        } else {
            result |= R_SSAA << 16;
            log_line("ssaa     failed: no memory within 2 GB of the game or write error");
        }
    }
    if (!g_fixes && !g_high_precision) {
        log_line("shaders  all fixes off");
    } else if (hook_create_device(base)) {
        result |= R_DEVICE;
        if (g_fixes & FIX_AO_SHADER)
            log_line("shaders  waiting for D3D11CreateDevice (AO: NierReplicantGFX shader, strength %.2f; "
                     "feedback blur %s, dither %s)", g_ao_strength, g_fixes & FIX_FEEDBACK ? "removed" : "on",
                     g_fixes & FIX_DITHER ? "on" : "off");
        else
            log_line("shaders  waiting for D3D11CreateDevice (AO: game shader, pattern %s, screen edges %s; "
                     "feedback blur %s, dither %s)", g_fixes & FIX_AO_PATTERN ? "fixed" : "original",
                     g_fixes & FIX_AO_EDGES_SOFT ? "soft" : g_fixes & FIX_AO_EDGES ? "clamped" : "original",
                     g_fixes & FIX_FEEDBACK ? "removed" : "on", g_fixes & FIX_DITHER ? "on" : "off");
        log_line("shaders  10-bit colour buffers %s", g_high_precision ? "created at 16 bits" : "kept");
    } else {
        result |= R_DEVICE << 16;
        log_line("shaders  failed: D3D11CreateDevice import not found");
    }
    return result;
}

/* Lunar Tear calls this after loading the DLL; everything is done in DllMain already. */
__declspec(dllexport) void LunarTearPluginInit(const void *api, void *handle)
{
    (void)api;
    (void)handle;
    log_line("Lunar Tear plugin init at %u ms", ms_since_start());
}

/* A float setting within [lo, hi]; `fallback` (and a log line) when it is outside. */
static float read_float(const wchar_t *path, const wchar_t *section, const wchar_t *key, const wchar_t *def,
                        float lo, float hi, float fallback)
{
    wchar_t value[32];
    float v;
    GetPrivateProfileStringW(section, key, def, value, 32, path);
    v = (float)_wtof(value);
    if (v < lo || v > hi) {
        log_line("config   %ls = %ls is outside %.1f-%.1f, %.1f is used", key, value, lo, hi, fallback);
        return fallback;
    }
    return v;
}

static void read_config(const wchar_t *dir)
{
    wchar_t path[MAX_PATH];
    unsigned fixes = 0;
    int edges;
    swprintf(path, MAX_PATH, L"%ls%ls", dir, CONFIG_NAME);
    if (GetPrivateProfileIntW(L"AmbientOcclusion", L"Shader", 1, path))
        fixes |= FIX_AO_SHADER;
    if (GetPrivateProfileIntW(L"AmbientOcclusion", L"Pattern", 1, path))
        fixes |= FIX_AO_PATTERN;
    edges = (int)GetPrivateProfileIntW(L"AmbientOcclusion", L"ScreenEdges", 0, path);
    if (edges == 1)
        fixes |= FIX_AO_EDGES;
    else if (edges == 2)
        fixes |= FIX_AO_EDGES_SOFT;
    if (!GetPrivateProfileIntW(L"PostProcess", L"FeedbackBlur", 0, path))
        fixes |= FIX_FEEDBACK;
    if (GetPrivateProfileIntW(L"PostProcess", L"Dither", 1, path))
        fixes |= FIX_DITHER;
    g_fixes = fixes;
    g_ao_strength = read_float(path, L"AmbientOcclusion", L"Strength", L"1.0", 0.0f, STRENGTH_MAX, 1.0f);
    g_high_precision = (int)GetPrivateProfileIntW(L"PostProcess", L"HighPrecision", 1, path);
    g_render_scale = read_float(path, L"Supersampling", L"RenderScale", L"1.0", 1.0f, SCALE_MAX, 1.0f);
    g_max_height = (int)GetPrivateProfileIntW(L"Supersampling", L"MaxRenderHeight", 2160, path);
}

static void attach(HINSTANCE self)
{
    wchar_t dll_dir[MAX_PATH];
    InitializeCriticalSection(&g_lock);
    module_dir(self, dll_dir, MAX_PATH);
    swprintf(g_log_path, MAX_PATH, L"%ls%ls", dll_dir, LOG_NAME);
    if (GetEnvironmentVariableW(ENV_MARKER, NULL, 0)) {
        g_log_path[0] = 0; /* another copy is already loaded and keeps the log */
        return;
    }
    SetEnvironmentVariableW(ENV_MARKER, L"1");

    log_line("NierReplicantGFX " NIERGFX_VERSION_STR " by " PLUGIN_AUTHOR ", loaded at %u ms after process start",
             ms_since_start());
    read_config(dll_dir);
    if (!host_is_game()) {
        log_line("host is not NieR Replicant ver.1.22474487139 (Steam): nothing patched");
        return;
    }
    NierGFX_Apply((uint8_t *)GetModuleHandleW(NULL));
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    (void)reserved;
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(instance);
        attach(instance);
    }
    return TRUE;
}

/* ------------------------------------------------------------------ xinput9_1_0.dll proxy */

#define XINPUT_NOT_CONNECTED 1167u   /* ERROR_DEVICE_NOT_CONNECTED */

static FARPROC real_xinput(const char *name)
{
    static HMODULE dll;
    if (!dll) {
        wchar_t path[MAX_PATH];
        UINT n = GetSystemDirectoryW(path, MAX_PATH);
        if (!n || n >= MAX_PATH - 20)
            return NULL;
        wcscat_s(path, MAX_PATH, L"\\xinput9_1_0.dll");
        dll = LoadLibraryW(path);
        if (!dll)
            return NULL;
    }
    return GetProcAddress(dll, name);
}

typedef DWORD(WINAPI *XInputState_t)(DWORD, void *);
typedef DWORD(WINAPI *XInputCaps_t)(DWORD, DWORD, void *);
typedef DWORD(WINAPI *XInputGuids_t)(DWORD, GUID *, GUID *);

DWORD WINAPI proxy_XInputGetState(DWORD user, void *state)
{
    XInputState_t f = (XInputState_t)real_xinput("XInputGetState");
    return f ? f(user, state) : XINPUT_NOT_CONNECTED;
}

DWORD WINAPI proxy_XInputSetState(DWORD user, void *vibration)
{
    XInputState_t f = (XInputState_t)real_xinput("XInputSetState");
    return f ? f(user, vibration) : XINPUT_NOT_CONNECTED;
}

DWORD WINAPI proxy_XInputGetCapabilities(DWORD user, DWORD flags, void *caps)
{
    XInputCaps_t f = (XInputCaps_t)real_xinput("XInputGetCapabilities");
    return f ? f(user, flags, caps) : XINPUT_NOT_CONNECTED;
}

DWORD WINAPI proxy_XInputGetDSoundAudioDeviceGuids(DWORD user, GUID *render, GUID *capture)
{
    XInputGuids_t f = (XInputGuids_t)real_xinput("XInputGetDSoundAudioDeviceGuids");
    return f ? f(user, render, capture) : XINPUT_NOT_CONNECTED;
}
