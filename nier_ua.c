/*
 * NierReplicantUA: game code changes for the Ukrainian translation of NieR Replicant ver.1.22474487139.
 * Author: Swiftlyx. License: MIT.
 *
 * The changes are made in memory when the DLL loads; game files are not modified. Ways to load it:
 *   - dinput8.dll in the game folder. The game imports DINPUT8.dll, so Windows loads this file before
 *     any game code runs; the DirectInput exports forward to the system dinput8.dll;
 *   - a Lunar Tear mod DLL (LunarTearPluginInit is exported, the plugin API is not used);
 *   - an .asi for Ultimate ASI Loader or Special K.
 * A group is applied only when all its sites hold the bytes of the Steam executable.
 *
 * Groups:
 *   pool     The interface strings of the selected language (text_common) are interned into a 256 KB
 *            pool, too small for the Ukrainian text. The parser start calls pool_init(), which allocates
 *            1 MiB and keeps its address in the first 8 bytes of the built-in pool; the intern and
 *            "[key]NoText" paths load the pool from there and check against the new size. Applied only
 *            while the text manager (exe+0x27E0590) is empty: its static constructor (exe+0xD6A90) sets
 *            used (+0x4F490) to 1, and the built-in pool starts with zeros until the first parse.
 *   fishing  Fishing records in cm/kg for the English text slot, which the translation uses
 *            (language range check 3-5 -> 2-5).
 *   pickup   "Obtained <item>": the item name is formatted straight into the 128-byte message buffer
 *            instead of a 32-byte temporary.
 *   talker   The talker_name.tnd loader keeps speaker names longer than 31 bytes.
 *   index    When data\info_uk.arc exists, the game opens it instead of data\info.arc (the file name in
 *            the executable's data is changed), so the original index stays as it is.
 * Results go to NierReplicantUA.log next to the DLL.
 */
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "version.h"

#define POOL_SIZE    0x100000u
#define TEXT_MANAGER 0x27E0590u
#define MGR_POOL     0xF490u
#define MGR_USED     0x4F490u
#define PARSER_START 0xD736Au
#define PARSER_NEXT  0xD7397u
#define SNPRINTF     0x7BAE0u
#define INDEX_NAME   0xD07728u
#define INDEX_FILE   L"data\\info_uk.arc"
#define ENV_MARKER   L"NIER_REPLICANT_UA_LOADED"
#define LOG_NAME     L"NierReplicantUA.log"

/* Result bits of NierUA_Apply (also used by the test). */
#define R_POOL     0x01
#define R_FISHING  0x02
#define R_PICKUP   0x04
#define R_TALKER   0x08
#define R_INDEX    0x10

static const Site POOL_SITES[] = {
    {PARSER_START, "33d2488db990f400004c89b42420c10000488bcf41b800000400e8b7f28500498bc548c7870000040001000000",
     NULL},   /* written by apply_pool() */
    {0xD6E84, "488d9990f40000", "488b9990f40000"},
    {0xD6F0D, "483d00000400", "483d00001000"},
    {0xD7A75, "498dbd90f40000", "498bbd90f40000"},
    {0xD79CA, "483d00000400", "483d00001000"},
};

static const Site FISHING_SITES[] = {
    {0x3A7D01, "83c0fd83f802", "83c0fe83f803"},
    {0x3A80D9, "83c0fd83f802", "83c0fe83f803"},
};

static const Site PICKUP_SITES[] = {
    {0xC19C9, "e812a1fbff", "e8525d9f00"},
    {0xC1B59, "e8829ffbff", "e8c25b9f00"},
    {0xC1CE9, "e8f29dfbff", "e8325a9f00"},
    {0xC1FE9, "e8f29afbff", "e832579f00"},
    {0xC2179, "e86299fbff", "e8a2559f00"},
};

static const Site TALKER_SITES[] = {
    {0xD68F3, "41b91f000000418d5101488d8d703f0000e82f3e86004c8d85703f0000",
     "eb1b909090909090909090909090909090909090909090909090909090"},
};

/*
 * Tail call of snprintf(tmp, 32, "%c%c%s%c%c", ...) from exe+0xC17B0: tmp is emptied and the name goes
 * to buf + strlen(buf), buf being the caller's 128-byte message at [rsp+0x60] on entry.
 */
static const char PICKUP_STUB[] =
    "c60100"          /* mov byte [rcx], 0 */
    "488d4c2460"      /* lea rcx, [rsp+0x60] */
    "488d442460"      /* lea rax, [rsp+0x60] */
    "803900"          /* .loop: cmp byte [rcx], 0 */
    "7405"            /* je .end */
    "48ffc1"          /* inc rcx */
    "ebf6"            /* jmp .loop */
    "488bd1"          /* .end: mov rdx, rcx */
    "482bd0"          /* sub rdx, rax */
    "48f7da"          /* neg rdx */
    "4881c280000000"; /* add rdx, 0x80; followed by jmp snprintf */

static const char INDEX_ORIG[12] = "info.arc\0\0\0";
static const char INDEX_NEW[12] = "info_uk.arc";

/* ------------------------------------------------------------------ pool */

/*
 * Called from the parser start with rcx = text manager: allocates the 1 MiB pool once, keeps its
 * address in the first 8 bytes of the built-in pool, clears it and resets the used counter.
 */
static void *pool_init(uint8_t *mgr)
{
    void **slot = (void **)(mgr + MGR_POOL);
    if (!*slot)
        *slot = VirtualAlloc(NULL, POOL_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    memset(*slot, 0, POOL_SIZE);
    *(uint64_t *)(mgr + MGR_USED) = 1;
    return *slot;
}

static int apply_pool(uint8_t *base)
{
    uint8_t code[45];
    uint64_t target = (uint64_t)(uintptr_t)&pool_init;
    size_t n = 0;
    memset(code, 0xCC, sizeof code);
    n += unhex("4c89b42420c10000", code + n, 8); /* mov [rsp+0xc120], r14 */
    n += unhex("498bcd", code + n, 3);           /* mov rcx, r13 */
    code[n++] = 0x48;                            /* mov rax, pool_init */
    code[n++] = 0xB8;
    memcpy(code + n, &target, 8);
    n += 8;
    n += unhex("ffd0", code + n, 2);             /* call rax */
    n += unhex("488bf8", code + n, 3);           /* mov rdi, rax */
    n += unhex("498bc5", code + n, 3);           /* mov rax, r13 */
    code[n++] = 0xEB;                            /* jmp exe+0xD7397 */
    code[n] = (uint8_t)(PARSER_NEXT - (PARSER_START + n + 1));
    n++;
    if (!apply_plain(base, POOL_SITES + 1, 4))
        return 0;
    return write_bytes(base + PARSER_START, code, sizeof code);
}

/* ------------------------------------------------------------------ pickup */

static int apply_pickup(uint8_t *base)
{
    uint8_t stub[64];
    uint8_t *cave = alloc_near(base);
    size_t n, i;
    int32_t d;
    if (!cave)
        return 0;
    n = unhex(PICKUP_STUB, stub, sizeof stub);
    stub[n] = 0xE9;
    if (!rel32(cave + n + 5, base + SNPRINTF, &d))
        return 0;
    memcpy(stub + n + 1, &d, 4);
    n += 5;
    memcpy(cave, stub, n);
    FlushInstructionCache(GetCurrentProcess(), cave, n);
    for (i = 0; i < sizeof PICKUP_SITES / sizeof PICKUP_SITES[0]; i++) {
        uint8_t call[5];
        uint8_t *site = base + PICKUP_SITES[i].rva;
        call[0] = 0xE8;
        if (!rel32(site + 5, cave, &d))
            return 0;
        memcpy(call + 1, &d, 4);
        if (!write_bytes(site, call, 5))
            return 0;
    }
    return 1;
}

/* ------------------------------------------------------------------ entry points */

/* Patches the game image at `base`; `game_dir` is the folder of the executable, with a trailing backslash. */
__declspec(dllexport) unsigned NierUA_Apply(uint8_t *base, const wchar_t *game_dir)
{
    unsigned result = 0;
    uint32_t used = *(volatile uint32_t *)(base + TEXT_MANAGER + MGR_USED);
    uint64_t head = *(volatile uint64_t *)(base + TEXT_MANAGER + MGR_POOL);
    wchar_t index_path[MAX_PATH];

    if (check_group(base, POOL_SITES, 5, R_POOL, "pool", &result)) {
        if (used > 1 || head != 0) {
            result |= R_POOL << 16;
            log_line("pool     skipped: text_common is already loaded (used = %u). Load the plugin earlier: "
                     "as dinput8.dll or as an early ASI", used);
        } else if (apply_pool(base)) {
            result |= R_POOL;
            log_line("pool     applied: text_common pool 1 MiB");
        } else {
            result |= R_POOL << 16;
            log_line("pool     failed to write");
        }
    }
    if (check_group(base, FISHING_SITES, 2, R_FISHING, "fishing", &result)) {
        if (apply_plain(base, FISHING_SITES, 2)) {
            result |= R_FISHING;
            log_line("fishing  applied: records in cm/kg");
        } else {
            result |= R_FISHING << 16;
            log_line("fishing  failed to write");
        }
    }
    if (check_group(base, PICKUP_SITES, 5, R_PICKUP, "pickup", &result)) {
        if (apply_pickup(base)) {
            result |= R_PICKUP;
            log_line("pickup   applied: long item names in \"Obtained\"");
        } else {
            result |= R_PICKUP << 16;
            log_line("pickup   failed: no memory within 2 GB of the game or write error");
        }
    }
    if (check_group(base, TALKER_SITES, 1, R_TALKER, "talker", &result)) {
        if (apply_plain(base, TALKER_SITES, 1)) {
            result |= R_TALKER;
            log_line("talker   applied: long speaker names");
        } else {
            result |= R_TALKER << 16;
            log_line("talker   failed to write");
        }
    }

    swprintf(index_path, MAX_PATH, L"%ls%ls", game_dir, INDEX_FILE);
    if (GetFileAttributesW(index_path) == INVALID_FILE_ATTRIBUTES) {
        log_line("index    data\\info_uk.arc not found, the game reads data\\info.arc");
    } else if (memcmp(base + INDEX_NAME, INDEX_NEW, sizeof INDEX_NEW) == 0) {
        result |= R_INDEX << 8;
        log_line("index    already info_uk.arc");
    } else if (memcmp(base + INDEX_NAME, INDEX_ORIG, sizeof INDEX_ORIG) != 0) {
        result |= R_INDEX << 16;
        log_line("index    skipped: the code differs from the Steam version");
    } else if (write_bytes(base + INDEX_NAME, INDEX_NEW, sizeof INDEX_NEW)) {
        result |= R_INDEX;
        log_line("index    applied: data\\info_uk.arc");
    } else {
        result |= R_INDEX << 16;
        log_line("index    failed to write");
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

static void attach(HINSTANCE self)
{
    wchar_t dll_dir[MAX_PATH], exe_dir[MAX_PATH];

    module_dir(self, dll_dir, MAX_PATH);
    swprintf(g_log_path, MAX_PATH, L"%ls%ls", dll_dir, LOG_NAME);
    if (GetEnvironmentVariableW(ENV_MARKER, NULL, 0)) {
        g_log_path[0] = 0; /* another copy is already loaded and keeps the log */
        return;
    }
    SetEnvironmentVariableW(ENV_MARKER, L"1");

    log_line("NierReplicantUA " NIERUA_VERSION_STR " by " PLUGIN_AUTHOR ", loaded at %u ms after process start",
             ms_since_start());
    if (!host_is_game()) {
        log_line("host is not NieR Replicant ver.1.22474487139 (Steam): nothing patched");
        return;
    }
    module_dir(NULL, exe_dir, MAX_PATH);
    NierUA_Apply((uint8_t *)GetModuleHandleW(NULL), exe_dir);
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

/* ------------------------------------------------------------------ dinput8.dll proxy */

static FARPROC real_dinput8(const char *name)
{
    static HMODULE dll;
    if (!dll) {
        wchar_t path[MAX_PATH];
        UINT n = GetSystemDirectoryW(path, MAX_PATH);
        if (!n || n >= MAX_PATH - 16)
            return NULL;
        wcscat_s(path, MAX_PATH, L"\\dinput8.dll");
        dll = LoadLibraryW(path);
        if (!dll)
            return NULL;
    }
    return GetProcAddress(dll, name);
}

typedef HRESULT(WINAPI *DirectInput8Create_t)(HINSTANCE, DWORD, REFIID, LPVOID *, LPUNKNOWN);
typedef HRESULT(WINAPI *DllGetClassObject_t)(REFCLSID, REFIID, LPVOID *);
typedef HRESULT(WINAPI *NoArgs_t)(void);
typedef LPCVOID(WINAPI *GetdfDIJoystick_t)(void);

HRESULT WINAPI proxy_DirectInput8Create(HINSTANCE inst, DWORD version, REFIID riid, LPVOID *out, LPUNKNOWN outer)
{
    DirectInput8Create_t f = (DirectInput8Create_t)real_dinput8("DirectInput8Create");
    return f ? f(inst, version, riid, out, outer) : E_FAIL;
}

HRESULT WINAPI proxy_DllCanUnloadNow(void)
{
    NoArgs_t f = (NoArgs_t)real_dinput8("DllCanUnloadNow");
    return f ? f() : S_FALSE;
}

HRESULT WINAPI proxy_DllGetClassObject(REFCLSID clsid, REFIID riid, LPVOID *out)
{
    DllGetClassObject_t f = (DllGetClassObject_t)real_dinput8("DllGetClassObject");
    return f ? f(clsid, riid, out) : E_FAIL;
}

HRESULT WINAPI proxy_DllRegisterServer(void)
{
    NoArgs_t f = (NoArgs_t)real_dinput8("DllRegisterServer");
    return f ? f() : E_FAIL;
}

HRESULT WINAPI proxy_DllUnregisterServer(void)
{
    NoArgs_t f = (NoArgs_t)real_dinput8("DllUnregisterServer");
    return f ? f() : E_FAIL;
}

LPCVOID WINAPI proxy_GetdfDIJoystick(void)
{
    GetdfDIJoystick_t f = (GetdfDIJoystick_t)real_dinput8("GetdfDIJoystick");
    return f ? f() : NULL;
}
