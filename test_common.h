/* Helpers of the offline tests (test_ua.c, test_gfx.c). */
#pragma once
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int failures;

static void check(int ok, const char *what)
{
    printf("  %s  %s\n", ok ? "ok  " : "FAIL", what);
    if (!ok)
        failures++;
}

static int bytes_are(const uint8_t *p, const char *hex)
{
    for (; hex[0] && hex[1]; hex += 2, p++) {
        unsigned v;
        sscanf_s(hex, "%2x", &v);
        if (*p != (uint8_t)v)
            return 0;
    }
    return 1;
}

/* The game executable mapped as an image: sections at their RVAs, imports unresolved, no code run. */
static uint8_t *map_image(const wchar_t *path)
{
    return (uint8_t *)LoadLibraryExW(path, NULL, DONT_RESOLVE_DLL_REFERENCES);
}

/* Target of the rel32 call or jump at `site`. */
static uint8_t *call_target(uint8_t *site)
{
    int32_t d;
    memcpy(&d, site + 1, 4);
    return site + 5 + d;
}

static int summary(void)
{
    printf(failures ? "\n%d check(s) FAILED\n" : "\nall checks passed\n", failures);
    return failures ? 1 : 0;
}
