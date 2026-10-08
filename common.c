/* Code shared by NierReplicantUA and NierReplicantGFX, see common.h. */
#include "common.h"

#include <share.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

wchar_t g_log_path[MAX_PATH];
static int g_log_started;

void log_line(const char *fmt, ...)
{
    FILE *f;
    va_list ap;
    if (!g_log_path[0])
        return;
    f = _wfsopen(g_log_path, g_log_started ? L"a" : L"w", _SH_DENYNO);
    if (!f)
        return;
    g_log_started = 1;
    va_start(ap, fmt);
    vfprintf(f, fmt, ap);
    va_end(ap);
    fputc('\n', f);
    fclose(f);
}

unsigned ms_since_start(void)
{
    FILETIME created, exited, kernel, user, now;
    ULARGE_INTEGER a, b;
    if (!GetProcessTimes(GetCurrentProcess(), &created, &exited, &kernel, &user))
        return 0;
    GetSystemTimeAsFileTime(&now);
    a.LowPart = created.dwLowDateTime;
    a.HighPart = created.dwHighDateTime;
    b.LowPart = now.dwLowDateTime;
    b.HighPart = now.dwHighDateTime;
    return (unsigned)((b.QuadPart - a.QuadPart) / 10000);
}

static int nibble(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    return (c | 0x20) - 'a' + 10;
}

size_t unhex(const char *hex, uint8_t *out, size_t cap)
{
    size_t n = 0;
    while (hex[0] && hex[1] && n < cap) {
        out[n++] = (uint8_t)(nibble(hex[0]) << 4 | nibble(hex[1]));
        hex += 2;
    }
    return n;
}

int write_bytes(uint8_t *dst, const void *src, size_t len)
{
    DWORD old;
    if (!VirtualProtect(dst, len, PAGE_EXECUTE_READWRITE, &old))
        return 0;
    memcpy(dst, src, len);
    VirtualProtect(dst, len, old, &old);
    FlushInstructionCache(GetCurrentProcess(), dst, len);
    return 1;
}

int rel32(const uint8_t *from_end, const uint8_t *to, int32_t *out)
{
    int64_t d = (int64_t)(to - from_end);
    if (d < INT32_MIN || d > INT32_MAX)
        return 0;
    *out = (int32_t)d;
    return 1;
}

/* Executable memory within +-2 GB of `target`, so rel32 calls and jumps reach both ways. */
uint8_t *alloc_near(uint8_t *target)
{
    SYSTEM_INFO si;
    uintptr_t gran, base, off;
    GetSystemInfo(&si);
    gran = si.dwAllocationGranularity;
    base = (uintptr_t)target & ~(gran - 1);
    for (off = gran; off < 0x70000000; off += gran) {
        void *p = VirtualAlloc((void *)(base + off), 4096, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
        if (p)
            return p;
        if (base > off) {
            p = VirtualAlloc((void *)(base - off), 4096, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
            if (p)
                return p;
        }
    }
    return NULL;
}

int check_group(const uint8_t *base, const Site *sites, size_t n, unsigned bit, const char *name, unsigned *result)
{
    size_t i;
    for (i = 0; i < n; i++) {
        uint8_t b[64];
        size_t len = unhex(sites[i].orig, b, sizeof b);
        if (memcmp(base + sites[i].rva, b, len) != 0) {
            *result |= bit << 16;
            log_line("%-8s skipped: the code differs from the Steam version (another game version or another mod)",
                     name);
            return 0;
        }
    }
    return 1;
}

int apply_plain(uint8_t *base, const Site *sites, size_t n)
{
    size_t i;
    for (i = 0; i < n; i++) {
        uint8_t b[64];
        size_t lb = unhex(sites[i].patch, b, sizeof b);
        if (!write_bytes(base + sites[i].rva, b, lb))
            return 0;
    }
    return 1;
}

void module_dir(HMODULE module, wchar_t *out, size_t cap)
{
    wchar_t *slash;
    GetModuleFileNameW(module, out, (DWORD)cap);
    slash = wcsrchr(out, L'\\');
    if (slash)
        slash[1] = 0;
}

int host_is_game(void)
{
    uint8_t *base = (uint8_t *)GetModuleHandleW(NULL);
    IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(base + ((IMAGE_DOS_HEADER *)base)->e_lfanew);
    return nt->OptionalHeader.SizeOfImage == GAME_IMAGE_SIZE;
}
