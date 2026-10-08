/*
 * Offline test of NierReplicantUA.dll: maps the game executable as an image (no game code runs), calls
 * NierUA_Apply on it and checks every site, the pickup stub and pool_init.
 *
 * Usage: test_ua.exe <game exe>
 */
#include "test_common.h"

#define TEXT_MANAGER 0x27E0590u
#define MGR_POOL     0xF490u
#define MGR_USED     0x4F490u
#define POOL_SIZE    0x100000u

typedef unsigned (*Apply_t)(uint8_t *, const wchar_t *);
typedef void *(*PoolInit_t)(uint8_t *);

static void check_pool_init(uint8_t *base)
{
    uint8_t *site = base + 0xD736A;
    uint64_t fn;
    uint8_t *mgr, *pool;
    PoolInit_t init;
    check(bytes_are(site, "4c89b42420c10000498bcd48b8"), "parser start: mov [rsp+0xc120], r14; mov rcx, r13; mov rax");
    check(bytes_are(site + 21, "ffd0488bf8498bc5eb0e"), "parser start: call rax; mov rdi, rax; mov rax, r13; jmp 0xD7397");
    memcpy(&fn, site + 13, 8);
    init = (PoolInit_t)(uintptr_t)fn;
    mgr = VirtualAlloc(NULL, 0x50000, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    pool = init(mgr);
    check(pool && *(uint8_t **)(mgr + MGR_POOL) == pool && *(uint64_t *)(mgr + MGR_USED) == 1,
          "pool_init: allocates, stores the pointer, used = 1");
    pool[POOL_SIZE - 1] = 0x55;
    *(uint64_t *)(mgr + MGR_USED) = 1234;
    check(init(mgr) == pool && pool[POOL_SIZE - 1] == 0 && *(uint64_t *)(mgr + MGR_USED) == 1,
          "pool_init again: same pool, cleared, used = 1");
}

static void check_applied(uint8_t *base)
{
    static const uint32_t pickup[] = {0xC19C9, 0xC1B59, 0xC1CE9, 0xC1FE9, 0xC2179};
    uint8_t *stub = call_target(base + pickup[0]);
    int i, same = 1;
    check_pool_init(base);
    check(bytes_are(base + 0xD6E84, "488b9990f40000"), "intern: mov rbx, [rcx+0xf490]");
    check(bytes_are(base + 0xD6F0D, "483d00001000"), "intern: cmp rax, 0x100000");
    check(bytes_are(base + 0xD7A75, "498bbd90f40000"), "NoText: mov rdi, [r13+0xf490]");
    check(bytes_are(base + 0xD79CA, "483d00001000"), "NoText: cmp rax, 0x100000");
    check(bytes_are(base + 0x3A7D01, "83c0fe83f803") && bytes_are(base + 0x3A80D9, "83c0fe83f803"),
          "fishing: language range 2-5");
    check(bytes_are(base + 0xD68F3, "eb1b909090909090909090909090909090909090909090909090909090"),
          "talker: strncpy skipped");
    for (i = 0; i < 5; i++)
        same &= base[pickup[i]] == 0xE8 && call_target(base + pickup[i]) == stub;
    check(same, "pickup: all 5 calls go to one stub");
    check(bytes_are(stub, "c60100488d4c2460488d442460803900740548ffc1ebf6488bd1482bd048f7da4881c280000000e9"),
          "pickup stub code");
    check(call_target(stub + 39) == base + 0x7BAE0, "pickup stub: jmp snprintf");
}

/* thunk(obj, stub): rsi = obj, call stub, return r9 (the index name the mount function would use) */
static const char *index_name(uint8_t *stub, const char *folder)
{
    static const uint8_t code[] = {0x56, 0x48, 0x89, 0xCE, 0xFF, 0xD2, 0x4C, 0x89, 0xC8, 0x5E, 0xC3};
    typedef const char *(*Thunk_t)(uint8_t *, uint8_t *);
    static uint8_t *thunk;
    static uint8_t obj[0x80];
    if (!thunk) {
        thunk = VirtualAlloc(NULL, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        memcpy(thunk, code, sizeof code);
    }
    memset(obj, 0, sizeof obj);
    strcpy_s((char *)obj + 0x28, 0x40, folder);
    return ((Thunk_t)thunk)(obj, stub);
}

static void check_index(uint8_t *base)
{
    const char *game_name = (const char *)base + 0xD07728;
    uint8_t *stub = call_target(base + 0x8ED281);
    check(base[0x8ED281] == 0xE8 && bytes_are(base + 0x8ED286, "9090"), "index: lea at 0x8ED281 -> call stub; nop; nop");
    check(strcmp(index_name(stub, "data/"), "info_uk.arc") == 0, "index stub: data/ -> info_uk.arc");
    check(index_name(stub, "dlc/dlc01/") == game_name, "index stub: dlc/dlc01/ -> the game's info.arc");
    check(index_name(stub, "") == game_name && index_name(stub, "data") == game_name &&
              index_name(stub, "database/") == game_name, "index stub: \"\", data, database/ -> the game's info.arc");
    check(memcmp(game_name, "info.arc\0\0\0\0", 12) == 0, "index: \"info.arc\" and the empty prefix at 0xD07731 unchanged");
}

int wmain(int argc, wchar_t **argv)
{
    wchar_t dir_with[MAX_PATH], dir_without[MAX_PATH], path[MAX_PATH], temp[MAX_PATH];
    HMODULE ua;
    Apply_t apply;
    uint8_t *base, before[32];
    unsigned r;
    HANDLE h;

    if (argc < 2) {
        printf("usage: test_ua <game exe>\n");
        return 2;
    }
    ua = LoadLibraryW(L"NierReplicantUA.dll");
    apply = ua ? (Apply_t)GetProcAddress(ua, "NierUA_Apply") : NULL;
    if (!apply) {
        printf("NierReplicantUA.dll or NierUA_Apply not found\n");
        return 2;
    }
    /* game folders with a trailing backslash, one of them with data\info_uk.arc */
    GetTempPathW(MAX_PATH, temp);
    swprintf(dir_without, MAX_PATH, L"%lsnierua_test_a\\", temp);
    swprintf(dir_with, MAX_PATH, L"%lsnierua_test_b\\", temp);
    CreateDirectoryW(dir_without, NULL);
    CreateDirectoryW(dir_with, NULL);
    swprintf(path, MAX_PATH, L"%lsdata", dir_with);
    CreateDirectoryW(path, NULL);
    swprintf(path, MAX_PATH, L"%lsdata\\info_uk.arc", dir_with);
    h = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    CloseHandle(h);

    printf("1. original exe, no data\\info_uk.arc\n");
    base = map_image(argv[1]);
    check(base != NULL, "image mapped");
    if (!base)
        return 1;
    r = apply(base, dir_without);
    printf("  result 0x%x\n", r);
    check((r & 0xF) == 0xF && !(r & 0x10) && !(r >> 16), "pool, fishing, pickup, talker applied; index not");
    check_applied(base);
    check(memcmp(base + 0xD07728, "info.arc\0\0\0\0", 12) == 0 && bytes_are(base + 0x8ED281, "4c8d0da0a44100"),
          "index: string and lea unchanged");
    FreeLibrary((HMODULE)base);

    printf("2. original exe, data\\info_uk.arc present\n");
    base = map_image(argv[1]);
    r = apply(base, dir_with);
    printf("  result 0x%x\n", r);
    check((r & 0x1F) == 0x1F && !(r >> 16), "all five applied");
    check_index(base);
    FreeLibrary((HMODULE)base);

    printf("3. original exe, text_common already parsed\n");
    base = map_image(argv[1]);
    *(uint32_t *)(base + TEXT_MANAGER + MGR_USED) = 4096;
    memcpy(before, base + 0xD736A, sizeof before);
    r = apply(base, dir_without);
    printf("  result 0x%x\n", r);
    check(!(r & 1) && (r & (1 << 16)) && (r & 0xE) == 0xE, "pool skipped, the rest applied");
    check(memcmp(before, base + 0xD736A, sizeof before) == 0, "parser start unchanged");
    FreeLibrary((HMODULE)base);

    printf("3b. original exe after the static constructor: used = 1, pool empty\n");
    base = map_image(argv[1]);
    *(uint32_t *)(base + TEXT_MANAGER + MGR_USED) = 1;
    r = apply(base, dir_without);
    printf("  result 0x%x\n", r);
    check((r & 0xF) == 0xF && !(r >> 16), "pool, fishing, pickup, talker applied");
    check_pool_init(base);
    FreeLibrary((HMODULE)base);

    printf("3c. original exe: used = 1 but the pool already holds bytes\n");
    base = map_image(argv[1]);
    *(uint32_t *)(base + TEXT_MANAGER + MGR_USED) = 1;
    memcpy(base + TEXT_MANAGER + MGR_POOL + 1, "ab", 2);
    memcpy(before, base + 0xD736A, sizeof before);
    r = apply(base, dir_without);
    printf("  result 0x%x\n", r);
    check(!(r & 1) && (r & (1 << 16)), "pool skipped");
    check(memcmp(before, base + 0xD736A, sizeof before) == 0, "parser start unchanged");
    FreeLibrary((HMODULE)base);

    printf("4. original exe with one pool site changed by someone else\n");
    base = map_image(argv[1]);
    {
        DWORD old;
        VirtualProtect(base + 0xD6F0D, 1, PAGE_EXECUTE_READWRITE, &old);
        base[0xD6F0D] = 0x90;
        VirtualProtect(base + 0xD6F0D, 1, old, &old);
    }
    memcpy(before, base + 0xD736A, sizeof before);
    r = apply(base, dir_without);
    printf("  result 0x%x\n", r);
    check(!(r & 1) && (r & (1 << 16)), "pool skipped");
    check(memcmp(before, base + 0xD736A, sizeof before) == 0, "parser start unchanged");
    FreeLibrary((HMODULE)base);
    return summary();
}
