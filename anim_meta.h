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
 * to the first FAS_ANIM_MAX_FRAMES rather than rejected, so a preview of a
 * very long animation shows its opening and loops. */
#define FAS_ANIM_MAX_FRAMES 64

#define FAS_ANIM_RATE_DEFAULT 2
#define FAS_ANIM_RATE_MAX    30

typedef enum {
    FasAnimMetaOk = 0,
    FasAnimMetaBadFiletype,
    FasAnimMetaNoPassiveFrames,
    FasAnimMetaNoFramesOrder,
    FasAnimMetaUnsupportedSize, /* declared larger than the screen         */
    FasAnimMetaNoSize,          /* no size declared, or a non-positive one */
} FasAnimMetaResult;

typedef struct {
    int  width;
    int  height;
    int  frame_rate;  /* always 1..FAS_ANIM_RATE_MAX once parsing succeeds */
    int  frame_count; /* usable passive frames                             */
    int  order[FAS_ANIM_MAX_FRAMES];
} FasAnimMeta;

/* Parses the text of a meta.txt.  `text` need not be NUL-terminated; `len`
 * bounds it.  Accepts both LF and CRLF line endings -- real meta.txt files
 * on device use CRLF.  `out` is fully overwritten on every call, including
 * on failure. */
FasAnimMetaResult fas_anim_meta_parse(const char* text, size_t len, FasAnimMeta* out);

/* Length of the prefix of `text` that ends on a line boundary.  Only a
 * bounded prefix of a meta.txt is ever read, so a longer file arrives with
 * its final line cut at an arbitrary byte; trimming to the last complete
 * line keeps a half-line from being parsed as a whole one.  Returns 0 when
 * there is no complete line. */
size_t fas_anim_meta_whole_lines(const char* text, size_t len);

/* ── .bm container validation ─────────────────────────────────────────── */

#include <stdint.h>

typedef enum {
    FasAnimBmInvalid = 0,
    FasAnimBmCompressed, /* 0x01 0x00, LE u16 length, heatshrink payload */
    FasAnimBmRaw,        /* 0x00, then raw XBM bytes                     */
} FasAnimBmKind;

/* Classifies a .bm frame container and checks that the length it declares is
 * actually backed by bytes on disk.
 *
 * This exists because the toolbox decoder takes its input length from the
 * header embedded in the data and ignores the size it is handed.  That is
 * safe for the compiled-in icon assets it was written for, and unsafe for a
 * file off the SD card: a frame declaring more payload than the file holds
 * would walk the decoder past the end of the caller's buffer.
 *
 * `xbm_bytes` is the decoded frame size the caller expects, ((w+7)/8)*h. */
FasAnimBmKind fas_anim_bm_validate(const uint8_t* data, size_t size, size_t xbm_bytes);
