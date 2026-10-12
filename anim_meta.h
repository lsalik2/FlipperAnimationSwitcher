#pragma once

/* Pure meta.txt parser for Flipper animation folders.
 *
 * Deliberately free of Flipper SDK includes so it can be compiled and
 * exercised on a host machine; see tests/host/run.sh.  All file I/O lives
 * in anim_player.c. */

#include <stdbool.h>
#include <stddef.h>

/* The Flipper screen.  An animation declaring more is refused, not clamped:
 * the render view's frame buffer is exactly FAS_ANIM_XBM_MAX bytes. */
#define FAS_ANIM_MAX_W 128
#define FAS_ANIM_MAX_H  64

#define FAS_ANIM_XBM_MAX  ((FAS_ANIM_MAX_W / 8) * FAS_ANIM_MAX_H) /* 1024 */
#define FAS_ANIM_FILE_MAX (FAS_ANIM_XBM_MAX + 8) /* worst-case .bm on disk */
#define FAS_ANIM_META_MAX 1024                   /* bytes of meta.txt read */

/* Cap on parsed "Frames order" entries.  A longer animation is truncated
 * to the first FAS_ANIM_MAX_FRAMES rather than rejected. */
#define FAS_ANIM_MAX_FRAMES 64

#define FAS_ANIM_RATE_DEFAULT 2
#define FAS_ANIM_RATE_MAX    30

typedef enum {
    FasAnimMetaOk = 0,
    FasAnimMetaBadFiletype,
    FasAnimMetaNoPassiveFrames,
    FasAnimMetaNoFramesOrder,
    FasAnimMetaUnsupportedSize,
} FasAnimMetaResult;

typedef struct {
    int  width;
    int  height;
    int  frame_rate;  /* always 1..FAS_ANIM_RATE_MAX once parsing succeeds */
    int  frame_count; /* usable passive frames                             */
    int  order[FAS_ANIM_MAX_FRAMES];
    bool order_truncated;
} FasAnimMeta;

/* Parses the text of a meta.txt.  `text` need not be NUL-terminated; `len`
 * bounds it.  Accepts both LF and CRLF line endings -- real meta.txt files
 * on device use CRLF.  `out` is fully overwritten on every call, including
 * on failure. */
FasAnimMetaResult fas_anim_meta_parse(const char* text, size_t len, FasAnimMeta* out);
