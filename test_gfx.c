/*
 * Offline test of NierReplicantGFX.dll:
 *   - the D3D11CreateDevice import slot of the game executable (mapped as an image, no game code runs);
 *   - edited shaders compared with the ones tools/gen_shader_edits.py --expected writes, and the
 *     replacement of the game's AO shader by ao_shader.h, with and without a Strength value;
 *   - the wrapped CreatePixelShader / CreateComputeShader on a WARP Direct3D 11 device, and both AO shaders
 *     run on a synthetic depth map;
 *   - 10-bit textures and views created at 16 bits;
 *   - supersampling: the patched sites and a run of the render size stub for 1080p, 1440p and 2160p.
 *
 * Usage: test_gfx.exe <game exe> <folder with orig_*.dxbc and fixed_*.dxbc>
 */
#include "test_common.h"
#include "shader_edits.h"
#include "ao_shader.h"

#define COBJMACROS
#include <d3d11.h>
#include <math.h>
#include <stdlib.h>

#define RENDER_SIZE  0x7D425Eu
#define FIT_OFFSET_X 0x8B31FBu
#define FIT_OFFSET_Y 0x8B3236u

typedef void (*Configure_t)(unsigned, int, float, float, int);
typedef unsigned (*Apply_t)(uint8_t *);
typedef uint32_t (*FindImport_t)(uint8_t *, const char *, const char *);
typedef uint8_t *(*PatchShader_t)(const uint8_t *, size_t, unsigned, size_t *, int *);
typedef void (*FreeShader_t)(uint8_t *);
typedef int (*HookDevice_t)(void *);
typedef unsigned (*Patched_t)(void);
typedef HRESULT(STDMETHODCALLTYPE *CreateShader_t)(void *, const void *, SIZE_T, void *, void **);

static Configure_t configure;
static Apply_t apply;
static FindImport_t find_import;
static PatchShader_t patch_shader;
static FreeShader_t free_shader;
static HookDevice_t hook_device;
static Patched_t patched;

static uint8_t *read_file(const wchar_t *path, size_t *size)
{
    FILE *f;
    uint8_t *buf;
    long n;
    if (_wfopen_s(&f, path, L"rb"))
        return NULL;
    fseek(f, 0, SEEK_END);
    n = ftell(f);
    fseek(f, 0, SEEK_SET);
    buf = malloc((size_t)n);
    *size = fread(buf, 1, (size_t)n, f);
    fclose(f);
    return buf;
}

static uint8_t *expected(const wchar_t *dir, const wchar_t *kind, const wchar_t *name, size_t *size)
{
    wchar_t path[MAX_PATH];
    swprintf(path, MAX_PATH, L"%ls\\%ls_%ls.dxbc", dir, kind, name);
    return read_file(path, size);
}

static void test_shaders(const wchar_t *dir)
{
    WIN32_FIND_DATAW fd;
    wchar_t pattern[MAX_PATH];
    HANDLE h;
    int count = 0;
    printf("2. shader rewriting = Python edits\n");
    swprintf(pattern, MAX_PATH, L"%ls\\orig_*.dxbc", dir);
    h = FindFirstFileW(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) {
        check(0, "expected blobs found");
        return;
    }
    do {
        wchar_t name[MAX_PATH];
        size_t n_orig, n_fixed, n_out = 0;
        uint8_t *orig, *fixed, *out;
        char label[200];
        int index = -1;
        wcsncpy_s(name, MAX_PATH, fd.cFileName + 5, wcslen(fd.cFileName) - 10);   /* orig_<name>.dxbc */
        orig = expected(dir, L"orig", name, &n_orig);
        fixed = expected(dir, L"fixed", name, &n_fixed);
        out = patch_shader(orig, n_orig, FIX_DEFAULT, &n_out, &index);
        snprintf(label, sizeof label, "%ls: %zu -> %zu bytes, same as Python", name, n_orig, n_out);
        check(out && n_out == n_fixed && memcmp(out, fixed, n_fixed) == 0, label);
        check(patch_shader(fixed, n_fixed, FIX_DEFAULT, &n_out, &index) == NULL, "  the fixed shader is not matched again");
        free_shader(out);
        free(orig);
        free(fixed);
        count++;
    } while (FindNextFileW(h, &fd));
    FindClose(h);
    check(count == (int)(sizeof SHADER_FIXES / sizeof SHADER_FIXES[0]), "every listed shader");
}

static void test_replacement(const wchar_t *dir)
{
    size_t n, n_out = 0;
    uint8_t *orig = expected(dir, L"orig", L"gen_ssao_mask_default_c", &n), *out;
    int index = -1;
    printf("2b. own AO shader\n");
    out = orig ? patch_shader(orig, n, FIX_DEFAULT | FIX_AO_SHADER, &n_out, &index) : NULL;
    check(out && n_out == sizeof AO_SHADER && memcmp(out, AO_SHADER, n_out) == 0 && index == 0,
          "gen_ssao_mask_default_c + FIX_AO_SHADER -> ao_shader.h");
    free_shader(out);

    configure(FIX_DEFAULT | FIX_AO_SHADER, 1, 0.5f, 1.0f, 2160);
    out = orig ? patch_shader(orig, n, FIX_DEFAULT | FIX_AO_SHADER, &n_out, &index) : NULL;
    if (out) {
        float strength;
        memcpy(&strength, out + AO_SHADER_STRENGTH, 4);
        check(n_out == sizeof AO_SHADER && strength == 0.5f && memcmp(out, AO_SHADER, 4) == 0 &&
                  memcmp(out + 4, AO_SHADER + 4, 16) != 0 && memcmp(out + 20, AO_SHADER + 20, AO_SHADER_STRENGTH - 20) == 0 &&
                  memcmp(out + AO_SHADER_STRENGTH + 4, AO_SHADER + AO_SHADER_STRENGTH + 4,
                         sizeof AO_SHADER - AO_SHADER_STRENGTH - 4) == 0,
              "  Strength 0.5: only the strength and the checksum differ");
    } else {
        check(0, "  Strength 0.5: shader made");
    }
    free_shader(out);
    configure(FIX_DEFAULT, 1, 1.0f, 1.0f, 2160);

    check(patch_shader(AO_SHADER, sizeof AO_SHADER, FIX_DEFAULT | FIX_AO_SHADER, &n_out, &index) == NULL,
          "  the own shader is not matched");
    free(orig);
}

/* AO on a 64x64 depth map: a plane facing the camera at view depth 5 and a 16x16 block 0.5 nearer in the
   middle; view radius 1 (11 pixels there), far clip 100. ao gets (AO, depth) per pixel. */
#define AO_N 64
static int in_block(int x, int y)
{
    return x >= 24 && x < 40 && y >= 24 && y < 40;
}

static int run_ao(ID3D11Device *dev, ID3D11ComputeShader *cs, float *ao)
{
    static float depth[AO_N * AO_N];
    float cb[40] = {0};
    const float tan_y = 0.57735f;
    D3D11_TEXTURE2D_DESC td = {0};
    D3D11_SUBRESOURCE_DATA init = {0};
    D3D11_BUFFER_DESC bd = {0};
    D3D11_MAPPED_SUBRESOURCE m;
    ID3D11DeviceContext *ctx = NULL;
    ID3D11Texture2D *dt = NULL, *ot = NULL, *st = NULL;
    ID3D11ShaderResourceView *srv = NULL;
    ID3D11UnorderedAccessView *uav = NULL;
    ID3D11Buffer *buf = NULL;
    int x, y, ok = 0;

    for (y = 0; y < AO_N; y++)
        for (x = 0; x < AO_N; x++)
            depth[y * AO_N + x] = in_block(x, y) ? 0.045f : 0.05f;
    cb[16] = cb[17] = 100.0f * tan_y;                     /* uv_to_view */
    cb[22] = 100.0f;                                      /* far clip */
    cb[23] = 0.1f;                                        /* near clip */
    cb[24] = cb[25] = cb[28] = cb[29] = (float)AO_N;      /* out and in size */
    cb[26] = cb[27] = cb[30] = cb[31] = 1.0f / AO_N;
    cb[32] = AO_N / 2 / tan_y;                            /* radius_ss */
    cb[33] = 0.1f;                                        /* bias */
    cb[34] = 1.0f;                                        /* intensity */
    cb[35] = -1.0f;                                       /* -1 / r^2 */
    cb[36] = cb[37] = 1.0f;                               /* multiplier, ao_scale */

    ID3D11Device_GetImmediateContext(dev, &ctx);
    td.Width = td.Height = AO_N;
    td.MipLevels = td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R32_FLOAT;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    init.pSysMem = depth;
    init.SysMemPitch = AO_N * 4;
    if (FAILED(ID3D11Device_CreateTexture2D(dev, &td, &init, &dt)) ||
        FAILED(ID3D11Device_CreateShaderResourceView(dev, (ID3D11Resource *)dt, NULL, &srv)))
        goto done;
    td.Format = DXGI_FORMAT_R32G32_FLOAT;
    td.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
    if (FAILED(ID3D11Device_CreateTexture2D(dev, &td, NULL, &ot)) ||
        FAILED(ID3D11Device_CreateUnorderedAccessView(dev, (ID3D11Resource *)ot, NULL, &uav)))
        goto done;
    td.Usage = D3D11_USAGE_STAGING;
    td.BindFlags = 0;
    td.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    bd.ByteWidth = sizeof cb;
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    init.pSysMem = cb;
    if (FAILED(ID3D11Device_CreateTexture2D(dev, &td, NULL, &st)) ||
        FAILED(ID3D11Device_CreateBuffer(dev, &bd, &init, &buf)))
        goto done;
    ID3D11DeviceContext_CSSetShader(ctx, cs, NULL, 0);
    ID3D11DeviceContext_CSSetShaderResources(ctx, 0, 1, &srv);
    ID3D11DeviceContext_CSSetUnorderedAccessViews(ctx, 0, 1, &uav, NULL);
    ID3D11DeviceContext_CSSetConstantBuffers(ctx, 0, 1, &buf);
    ID3D11DeviceContext_Dispatch(ctx, AO_N / 16, AO_N / 16, 1);
    ID3D11DeviceContext_CopyResource(ctx, (ID3D11Resource *)st, (ID3D11Resource *)ot);
    if (FAILED(ID3D11DeviceContext_Map(ctx, (ID3D11Resource *)st, 0, D3D11_MAP_READ, 0, &m)))
        goto done;
    for (y = 0; y < AO_N; y++)
        memcpy(ao + y * AO_N * 2, (uint8_t *)m.pData + y * m.RowPitch, AO_N * 8);
    ID3D11DeviceContext_Unmap(ctx, (ID3D11Resource *)st, 0);
    ok = 1;
done:
    if (buf)
        ID3D11Buffer_Release(buf);
    if (st)
        ID3D11Texture2D_Release(st);
    if (uav)
        ID3D11UnorderedAccessView_Release(uav);
    if (ot)
        ID3D11Texture2D_Release(ot);
    if (srv)
        ID3D11ShaderResourceView_Release(srv);
    if (dt)
        ID3D11Texture2D_Release(dt);
    if (ctx)
        ID3D11DeviceContext_Release(ctx);
    return ok;
}

/* occlusion (1 - AO) summed over the plane pixels within 8 pixels of the block */
static float contact(const float *ao)
{
    float sum = 0;
    int x, y;
    for (y = 16; y < 48; y++)
        for (x = 16; x < 48; x++)
            if (!in_block(x, y))
                sum += 1 - ao[(y * AO_N + x) * 2];
    return sum;
}

/* Creates gen_ssao_mask_default_c through the wrapped device with `fixes` and `strength` and runs it. */
static int ao_with(ID3D11Device *dev, const uint8_t *orig, size_t n, unsigned fixes, float strength, float *ao)
{
    ID3D11ComputeShader *cs = NULL;
    int ok;
    configure(fixes, 1, strength, 1.0f, 2160);
    ok = SUCCEEDED(ID3D11Device_CreateComputeShader(dev, orig, n, NULL, &cs)) && run_ao(dev, cs, ao);
    if (cs)
        ID3D11ComputeShader_Release(cs);
    configure(FIX_DEFAULT, 1, 1.0f, 1.0f, 2160);
    return ok;
}

static void test_ao_run(ID3D11Device *dev, const wchar_t *dir)
{
    static float game[AO_N * AO_N * 2], own[AO_N * AO_N * 2], off[AO_N * AO_N * 2], dark[AO_N * AO_N * 2];
    size_t n;
    uint8_t *orig = expected(dir, L"orig", L"gen_ssao_mask_default_c", &n);
    float flat = 1, c_game, c_own;
    int range_ok = 1, depth_ok = 1, i;
    char label[160];
    printf("3b. AO shaders on a synthetic depth map\n");
    if (!orig) {
        check(0, "gen_ssao_mask_default_c found");
        return;
    }
    check(ao_with(dev, orig, n, FIX_DEFAULT, 1.0f, game), "game shader (pattern fix) ran");
    check(ao_with(dev, orig, n, FIX_DEFAULT | FIX_AO_SHADER, 1.0f, own), "own shader ran");
    check(ao_with(dev, orig, n, FIX_DEFAULT | FIX_AO_SHADER, 0.0f, off) &&
              ao_with(dev, orig, n, FIX_DEFAULT | FIX_AO_SHADER, 2.0f, dark),
          "own shader ran with Strength 0 and 2");
    free(orig);
    for (i = 0; i < AO_N * AO_N; i++) {
        int x = i % AO_N, y = i / AO_N;
        float a = own[2 * i], d = own[2 * i + 1];
        if (!(a >= 0 && a <= 1))
            range_ok = 0;
        if (!(fabsf(d - (in_block(x, y) ? 4.5f : 5.0f) / 99.9f) < 1e-5f))
            depth_ok = 0;
        if ((x < 12 || x >= 52 || y < 12 || y >= 52) && a < flat)
            flat = a;
    }
    check(range_ok, "own: AO within 0..1 everywhere");
    check(depth_ok, "own: y = view depth / (far - near)");
    snprintf(label, sizeof label, "own: plane away from the block unshaded, screen edges included (min AO %.4f)", flat);
    check(flat == 1.0f, label);
    c_game = contact(game);
    c_own = contact(own);
    snprintf(label, sizeof label, "own: contact shade around the block %.1f, game %.1f (0.7..1.5x)", c_own, c_game);
    check(c_game > 1 && c_own > 0.7f * c_game && c_own < 1.5f * c_game, label);
    snprintf(label, sizeof label, "Strength 0: no shade (%.2f); Strength 2: darker (%.1f)", contact(off), contact(dark));
    check(contact(off) == 0.0f && contact(dark) > 1.2f * c_own, label);
}

static DXGI_FORMAT texture_format(ID3D11Device *dev, DXGI_FORMAT format, int with_data)
{
    D3D11_TEXTURE2D_DESC d = {0}, got = {0};
    D3D11_SUBRESOURCE_DATA init = {0};
    static uint32_t pixels[16 * 16];
    ID3D11Texture2D *tex = NULL;
    d.Width = d.Height = 16;
    d.MipLevels = d.ArraySize = 1;
    d.Format = format;
    d.SampleDesc.Count = 1;
    d.Usage = D3D11_USAGE_DEFAULT;
    d.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    init.pSysMem = pixels;
    init.SysMemPitch = 16 * 4;
    if (FAILED(ID3D11Device_CreateTexture2D(dev, &d, with_data ? &init : NULL, &tex)))
        return DXGI_FORMAT_UNKNOWN;
    ID3D11Texture2D_GetDesc(tex, &got);
    ID3D11Texture2D_Release(tex);
    return got.Format;
}

static void test_precision(ID3D11Device *dev)
{
    D3D11_TEXTURE2D_DESC d = {0};
    D3D11_RENDER_TARGET_VIEW_DESC rd = {0};
    D3D11_SHADER_RESOURCE_VIEW_DESC sd = {0};
    ID3D11Texture2D *tex = NULL;
    ID3D11RenderTargetView *rtv = NULL;
    ID3D11ShaderResourceView *srv = NULL;
    printf("4. 10-bit colour buffers at 16 bits\n");
    check(texture_format(dev, DXGI_FORMAT_R10G10B10A2_UNORM, 0) == DXGI_FORMAT_R16G16B16A16_FLOAT,
          "R10G10B10A2_UNORM render target created as R16G16B16A16_FLOAT");
    check(texture_format(dev, DXGI_FORMAT_R10G10B10A2_TYPELESS, 0) == DXGI_FORMAT_R16G16B16A16_TYPELESS,
          "R10G10B10A2_TYPELESS created as R16G16B16A16_TYPELESS");
    check(texture_format(dev, DXGI_FORMAT_R10G10B10A2_UNORM, 1) == DXGI_FORMAT_R10G10B10A2_UNORM,
          "a texture with initial data stays R10G10B10A2_UNORM");
    check(texture_format(dev, DXGI_FORMAT_R8G8B8A8_UNORM, 0) == DXGI_FORMAT_R8G8B8A8_UNORM, "other formats untouched");

    d.Width = d.Height = 16;
    d.MipLevels = d.ArraySize = 1;
    d.Format = DXGI_FORMAT_R10G10B10A2_UNORM;
    d.SampleDesc.Count = 1;
    d.Usage = D3D11_USAGE_DEFAULT;
    d.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    ID3D11Device_CreateTexture2D(dev, &d, NULL, &tex);
    rd.Format = DXGI_FORMAT_R10G10B10A2_UNORM;
    rd.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    check(tex && SUCCEEDED(ID3D11Device_CreateRenderTargetView(dev, (ID3D11Resource *)tex, &rd, &rtv)),
          "RTV naming R10G10B10A2_UNORM on it created");
    sd.Format = DXGI_FORMAT_R10G10B10A2_UNORM;
    sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    sd.Texture2D.MipLevels = 1;
    check(tex && SUCCEEDED(ID3D11Device_CreateShaderResourceView(dev, (ID3D11Resource *)tex, &sd, &srv)),
          "SRV naming R10G10B10A2_UNORM on it created");

    configure(FIX_DEFAULT, 0, 1.0f, 1.0f, 0);
    check(texture_format(dev, DXGI_FORMAT_R10G10B10A2_UNORM, 0) == DXGI_FORMAT_R10G10B10A2_UNORM,
          "HighPrecision = 0: stays R10G10B10A2_UNORM");
    configure(FIX_DEFAULT, 1, 1.0f, 2.0f, 2160);
}

static void test_device(const wchar_t *dir)
{
    ID3D11Device *dev = NULL;
    void **vt;
    void *shader = NULL;
    size_t n;
    uint8_t *cs, *ps, *ps_fixed;
    HRESULT hr;
    printf("3. wrapped shader creation on a WARP device\n");
    configure(FIX_DEFAULT, 1, 1.0f, 1.0f, 2160);
    hr = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_WARP, NULL, 0, NULL, 0, D3D11_SDK_VERSION, &dev, NULL, NULL);
    check(SUCCEEDED(hr), "WARP device");
    if (FAILED(hr))
        return;
    check(hook_device(dev) == 1 && hook_device(dev) == 1, "device hooked (once)");
    vt = *(void ***)dev;
    cs = expected(dir, L"orig", L"gen_ssao_mask_default_c", &n);
    hr = ((CreateShader_t)vt[18])(dev, cs, n, NULL, &shader);
    check(SUCCEEDED(hr) && shader && (patched() & 1), "CreateComputeShader(gen_ssao_mask_default_c): rewritten, created");
    ps_fixed = expected(dir, L"fixed", L"pfx_hbao_default_p", &n);
    hr = ((CreateShader_t)vt[15])(dev, ps_fixed, n, NULL, &shader);
    check(SUCCEEDED(hr) && patched() == 1, "CreatePixelShader(unknown shader): passed through");
    ps = expected(dir, L"orig", L"pfx_hbao_default_p", &n);
    hr = ((CreateShader_t)vt[15])(dev, ps, n, NULL, &shader);
    check(SUCCEEDED(hr) && patched() == 3, "CreatePixelShader(pfx_hbao_default_p): rewritten, created");
    free(cs);
    free(ps);
    free(ps_fixed);
    test_ao_run(dev, dir);
    test_precision(dev);
}

/* thunk(display, stub): rbx = display, call stub, return r8 (the render size it reloads) */
static uint64_t run_stub(uint8_t *stub, uint32_t w, uint32_t h, uint32_t *out_w, uint32_t *out_h)
{
    static const uint8_t code[] = {0x53, 0x48, 0x89, 0xCB, 0x48, 0x83, 0xEC, 0x20, 0xFF, 0xD2,
                                   0x4C, 0x89, 0xC0, 0x48, 0x83, 0xC4, 0x20, 0x5B, 0xC3};
    typedef uint64_t (*Thunk_t)(uint8_t *, uint8_t *);
    static uint8_t *thunk, *display;
    uint64_t r8;
    if (!thunk) {
        thunk = VirtualAlloc(NULL, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        display = VirtualAlloc(NULL, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        memcpy(thunk, code, sizeof code);
    }
    memcpy(display + 0x174, &w, 4);
    memcpy(display + 0x178, &h, 4);
    r8 = ((Thunk_t)thunk)(display, stub);
    memcpy(out_w, display + 0x174, 4);
    memcpy(out_h, display + 0x178, 4);
    return r8;
}

static void test_ssaa(const wchar_t *exe)
{
    static const uint32_t cases[][4] = {{1920, 1080, 3840, 2160}, {2560, 1440, 3840, 2160}, {3840, 2160, 3840, 2160}};
    uint8_t *base, *stub;
    unsigned r;
    int i;
    printf("5. supersampling, RenderScale 2.0, MaxRenderHeight 2160\n");
    base = map_image(exe);
    configure(FIX_DEFAULT, 1, 1.0f, 2.0f, 2160);
    r = apply(base);
    printf("  result 0x%x\n", r);
    check((r & 3) == 3 && !(r >> 16), "ssaa applied, D3D11CreateDevice import redirected");
    check(base[RENDER_SIZE] == 0xE8 && bytes_are(base + RENDER_SIZE + 5, "909090909090"), "render size: call stub + 6 nops");
    check(bytes_are(base + FIT_OFFSET_X, "410f28c50f1f440000") && bytes_are(base + FIT_OFFSET_Y, "410f28c60f1f440000"),
          "final pass: offsets from the fitted 16:9 rectangle");
    stub = call_target(base + RENDER_SIZE);
    for (i = 0; i < 3; i++) {
        uint32_t w, h;
        uint64_t r8 = run_stub(stub, cases[i][0], cases[i][1], &w, &h);
        char label[120];
        snprintf(label, sizeof label, "stub: %ux%u -> %ux%u, r8 = new size", cases[i][0], cases[i][1], w, h);
        check(w == cases[i][2] && h == cases[i][3] && r8 == ((uint64_t)h << 32 | w), label);
    }
    FreeLibrary((HMODULE)base);

    printf("6. supersampling off (RenderScale 1.0)\n");
    base = map_image(exe);
    configure(FIX_DEFAULT, 1, 1.0f, 1.0f, 2160);
    r = apply(base);
    check(!(r & 1) && bytes_are(base + RENDER_SIZE, "488d4b284c8b8374010000") &&
              bytes_are(base + FIT_OFFSET_X, "660f6e4424680f5bc0"), "ssaa sites unchanged");
    FreeLibrary((HMODULE)base);
}

int wmain(int argc, wchar_t **argv)
{
    HMODULE gfx;
    uint8_t *base;
    if (argc < 3) {
        printf("usage: test_gfx <original exe> <expected shaders folder>\n");
        return 2;
    }
    gfx = LoadLibraryW(L"NierReplicantGFX.dll");
    if (!gfx) {
        printf("NierReplicantGFX.dll not found\n");
        return 2;
    }
    configure = (Configure_t)GetProcAddress(gfx, "NierGFX_Configure");
    apply = (Apply_t)GetProcAddress(gfx, "NierGFX_Apply");
    find_import = (FindImport_t)GetProcAddress(gfx, "NierGFX_FindImport");
    patch_shader = (PatchShader_t)GetProcAddress(gfx, "NierGFX_PatchShader");
    free_shader = (FreeShader_t)GetProcAddress(gfx, "NierGFX_FreeShader");
    hook_device = (HookDevice_t)GetProcAddress(gfx, "NierGFX_HookDevice");
    patched = (Patched_t)GetProcAddress(gfx, "NierGFX_PatchedShaders");
    if (!configure || !apply || !find_import || !patch_shader || !free_shader || !hook_device || !patched) {
        printf("NierReplicantGFX.dll exports missing\n");
        return 2;
    }

    printf("1. D3D11CreateDevice import\n");
    base = map_image(argv[1]);
    check(base && find_import(base, "D3D11.dll", "D3D11CreateDevice") == 0xAB8780, "IAT slot exe+0xAB8780");
    check(base && find_import(base, "d3d11.dll", "D3D11CreateDeviceAndSwapChain") == 0, "absent import -> 0");
    FreeLibrary((HMODULE)base);

    test_shaders(argv[2]);
    test_replacement(argv[2]);
    test_device(argv[2]);
    test_ssaa(argv[1]);
    return summary();
}
