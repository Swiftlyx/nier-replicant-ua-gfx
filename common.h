/*
 * Code shared by NierReplicantUA and NierReplicantGFX: log file, hex strings, code writes, executable
 * memory near the game image, patch sites.
 * Author: Swiftlyx. License: MIT.
 */
#pragma once
#include <windows.h>
#include <stdint.h>

#define GAME_IMAGE_SIZE 0x5339000u   /* SizeOfImage of the Steam executable */

/* A patched place in the game's code. */
typedef struct {
    uint32_t rva;
    const char *orig;   /* hex: bytes of the Steam executable */
    const char *patch;  /* hex: bytes written by apply_plain(); NULL when the plugin writes them itself */
} Site;

extern wchar_t g_log_path[MAX_PATH];   /* empty: no log */

void log_line(const char *fmt, ...);
unsigned ms_since_start(void);
size_t unhex(const char *hex, uint8_t *out, size_t cap);
int write_bytes(uint8_t *dst, const void *src, size_t len);
int rel32(const uint8_t *from_end, const uint8_t *to, int32_t *out);
uint8_t *alloc_near(uint8_t *target);

/* 1 when every site holds its original bytes. Otherwise logs that the group is skipped, sets
   bit << 16 in *result and returns 0. */
int check_group(const uint8_t *base, const Site *sites, size_t n, unsigned bit, const char *name, unsigned *result);
/* Writes `patch` of every site. */
int apply_plain(uint8_t *base, const Site *sites, size_t n);

/* Folder of `module` with a trailing backslash. */
void module_dir(HMODULE module, wchar_t *out, size_t cap);
/* 1 when the host process is the Steam executable of the game. */
int host_is_game(void);
