/* Host-side tests for the pure meta.txt parser.  Run with tests/host/run.sh. */

#include "../../anim_meta.h"

#include <stdio.h>
#include <string.h>

static int failures = 0;
static const char* current_test = "";

#define CHECK(cond)                                                       \
    do {                                                                  \
        if(!(cond)) {                                                     \
            printf("  FAIL [%s] %s:%d: %s\n",                             \
                   current_test, __FILE__, __LINE__, #cond);              \
            failures++;                                                   \
        }                                                                 \
    } while(0)

#define RUN(fn)                     \
    do {                            \
        current_test = #fn;         \
        printf("- %s\n", #fn);      \
        fn();                       \
    } while(0)

#define PARSE(lit, meta) fas_anim_meta_parse((lit), strlen(lit), &(meta))

/* Verbatim content of a real /ext/dolphin/<name>/meta.txt, CRLF and all. */
static const char* REAL_CRLF =
    "Filetype: Flipper Animation\r\n"
    "Version: 1\r\n"
    "\r\n"
    "Width: 128\r\n"
    "Height: 64\r\n"
    "Passive frames: 16\r\n"
    "Active frames: 0\r\n"
    "Frames order: 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15\r\n"
    "Active cycles: 0\r\n"
    "Frame rate: 5\r\n"
    "Duration: 3600\r\n"
    "Active cooldown: 0\r\n"
    "\r\n"
    "Bubble slots: 0\r\n";

/* Review Focus 1: CRLF is the real-world case, not an edge case. */
static void test_real_crlf_file(void) {
    FasAnimMeta m;
    CHECK(PARSE(REAL_CRLF, m) == FasAnimMetaOk);
    CHECK(m.width == 128);
    CHECK(m.height == 64);
    CHECK(m.frame_rate == 5);
    CHECK(m.frame_count == 16);
    CHECK(m.order[0] == 0);
    /* The last entry is the one a stray '\r' would corrupt. */
    CHECK(m.order[15] == 15);
}

static void test_lf_parses_identically(void) {
    char lf[1024];
    size_t j = 0;
    for(size_t i = 0; REAL_CRLF[i] != '\0'; i++) {
        if(REAL_CRLF[i] != '\r') lf[j++] = REAL_CRLF[i];
    }
    lf[j] = '\0';

    FasAnimMeta a, b;
    CHECK(fas_anim_meta_parse(REAL_CRLF, strlen(REAL_CRLF), &a) == FasAnimMetaOk);
    CHECK(fas_anim_meta_parse(lf, j, &b) == FasAnimMetaOk);
    CHECK(memcmp(&a, &b, sizeof(FasAnimMeta)) == 0);
}

static void test_no_trailing_newline(void) {
    const char* t =
        "Filetype: Flipper Animation\r\n"
        "Width: 128\r\n"
        "Height: 64\r\n"
        "Passive frames: 2\r\n"
        "Frame rate: 4\r\n"
        "Frames order: 0 1"; /* file ends mid-line */
    FasAnimMeta m;
    CHECK(PARSE(t, m) == FasAnimMetaOk);
    CHECK(m.frame_count == 2);
    CHECK(m.order[1] == 1);
}

/* Review Focus 2: an oversized animation must be refused outright. */
static void test_oversize_refused(void) {
    const char* t =
        "Filetype: Flipper Animation\r\n"
        "Width: 256\r\n"
        "Height: 64\r\n"
        "Passive frames: 1\r\n"
        "Frames order: 0\r\n";
    FasAnimMeta m;
    CHECK(PARSE(t, m) == FasAnimMetaUnsupportedSize);

    const char* tall =
        "Filetype: Flipper Animation\r\n"
        "Width: 128\r\n"
        "Height: 128\r\n"
        "Passive frames: 1\r\n"
        "Frames order: 0\r\n";
    CHECK(PARSE(tall, m) == FasAnimMetaUnsupportedSize);
}

/* Review Focus 3: a long Frames order truncates, it does not overflow.
 * Under ASan, an off-by-one here is a hard failure rather than silent
 * corruption. */
static void test_frames_order_truncates(void) {
    char t[2048];
    int  n = snprintf(t, sizeof(t),
                      "Filetype: Flipper Animation\r\n"
                      "Width: 128\r\n"
                      "Height: 64\r\n"
                      "Passive frames: 200\r\n"
                      "Frame rate: 5\r\n"
                      "Frames order:");
    for(int i = 0; i < 200; i++) {
        n += snprintf(t + n, sizeof(t) - (size_t)n, " %d", i % 16);
    }
    snprintf(t + n, sizeof(t) - (size_t)n, "\r\n");

    FasAnimMeta m;
    CHECK(fas_anim_meta_parse(t, strlen(t), &m) == FasAnimMetaOk);
    CHECK(m.frame_count == FAS_ANIM_MAX_FRAMES);
    /* Exactly the first FAS_ANIM_MAX_FRAMES entries were stored: parsing
     * neither stopped early nor ran past the end of order[]. */
    CHECK(m.order[0] == 0);
    CHECK(m.order[FAS_ANIM_MAX_FRAMES - 1] == (FAS_ANIM_MAX_FRAMES - 1) % 16);
}

/* Review Focus 4: the frame rate feeds a timer period of 1000/rate. */
static void test_frame_rate_clamped(void) {
    FasAnimMeta m;

    const char* zero =
        "Filetype: Flipper Animation\r\n"
        "Width: 128\r\nHeight: 64\r\nPassive frames: 1\r\n"
        "Frame rate: 0\r\nFrames order: 0\r\n";
    CHECK(PARSE(zero, m) == FasAnimMetaOk);
    CHECK(m.frame_rate == FAS_ANIM_RATE_DEFAULT);

    const char* absent =
        "Filetype: Flipper Animation\r\n"
        "Width: 128\r\nHeight: 64\r\nPassive frames: 1\r\n"
        "Frames order: 0\r\n";
    CHECK(PARSE(absent, m) == FasAnimMetaOk);
    CHECK(m.frame_rate == FAS_ANIM_RATE_DEFAULT);

    const char* negative =
        "Filetype: Flipper Animation\r\n"
        "Width: 128\r\nHeight: 64\r\nPassive frames: 1\r\n"
        "Frame rate: -3\r\nFrames order: 0\r\n";
    CHECK(PARSE(negative, m) == FasAnimMetaOk);
    CHECK(m.frame_rate == FAS_ANIM_RATE_DEFAULT);

    const char* huge =
        "Filetype: Flipper Animation\r\n"
        "Width: 128\r\nHeight: 64\r\nPassive frames: 1\r\n"
        "Frame rate: 9999\r\nFrames order: 0\r\n";
    CHECK(PARSE(huge, m) == FasAnimMetaOk);
    CHECK(m.frame_rate == FAS_ANIM_RATE_MAX);
}

static void test_rejects_non_animation(void) {
    FasAnimMeta m;

    const char* manifest =
        "Filetype: Flipper Animation Manifest\r\n"
        "Width: 128\r\nHeight: 64\r\nPassive frames: 1\r\nFrames order: 0\r\n";
    CHECK(PARSE(manifest, m) == FasAnimMetaBadFiletype);

    const char* empty = "";
    CHECK(fas_anim_meta_parse(empty, 0, &m) == FasAnimMetaBadFiletype);
}

static void test_rejects_unplayable(void) {
    FasAnimMeta m;

    const char* no_passive =
        "Filetype: Flipper Animation\r\n"
        "Width: 128\r\nHeight: 64\r\nPassive frames: 0\r\nFrames order: 0 1\r\n";
    CHECK(PARSE(no_passive, m) == FasAnimMetaNoPassiveFrames);

    const char* absent_passive =
        "Filetype: Flipper Animation\r\n"
        "Width: 128\r\nHeight: 64\r\nFrames order: 0 1\r\n";
    CHECK(PARSE(absent_passive, m) == FasAnimMetaNoPassiveFrames);

    const char* no_order =
        "Filetype: Flipper Animation\r\n"
        "Width: 128\r\nHeight: 64\r\nPassive frames: 4\r\n";
    CHECK(PARSE(no_order, m) == FasAnimMetaNoFramesOrder);

    const char* blank_order =
        "Filetype: Flipper Animation\r\n"
        "Width: 128\r\nHeight: 64\r\nPassive frames: 4\r\nFrames order:   \r\n";
    CHECK(PARSE(blank_order, m) == FasAnimMetaNoFramesOrder);
}

/* Passive frames may exceed the order list; playback is bounded by both. */
static void test_frame_count_is_the_lesser(void) {
    const char* t =
        "Filetype: Flipper Animation\r\n"
        "Width: 128\r\nHeight: 64\r\n"
        "Passive frames: 10\r\nFrame rate: 5\r\nFrames order: 0 1 2\r\n";
    FasAnimMeta m;
    CHECK(PARSE(t, m) == FasAnimMetaOk);
    CHECK(m.frame_count == 3);
}

/* "Active frames: 0" is the common case, and active frames are a non-goal:
 * only the first "Passive frames" entries may ever be played. */
static void test_active_frames_are_ignored(void) {
    const char* t =
        "Filetype: Flipper Animation\r\n"
        "Width: 128\r\nHeight: 64\r\n"
        "Passive frames: 2\r\nActive frames: 3\r\nFrame rate: 5\r\n"
        "Frames order: 0 1 2 3 4\r\n";
    FasAnimMeta m;
    CHECK(PARSE(t, m) == FasAnimMetaOk);
    CHECK(m.frame_count == 2);
}

/* ── .bm container validation ──────────────────────────────────────────
 * compress_icon_decode() takes no input size and trusts the length inside
 * the data, so these guard the one place untrusted file bytes reach it. */

#define XBM_1024 1024

static void test_bm_accepts_a_real_compressed_header(void) {
    /* 01 00, declared 818, followed by exactly 818 payload bytes: the shape
     * the spike confirmed byte-for-byte against a real frame_0.bm. */
    static uint8_t buf[4 + 818];
    buf[0] = 0x01;
    buf[1] = 0x00;
    buf[2] = (uint8_t)(818 & 0xFF);
    buf[3] = (uint8_t)(818 >> 8);
    CHECK(fas_anim_bm_validate(buf, sizeof(buf), XBM_1024) == FasAnimBmCompressed);
}

/* The attack: a header that claims far more payload than the file holds.
 * Without this check the decoder reads past the end of the read buffer. */
static void test_bm_rejects_overlong_declared_length(void) {
    static uint8_t buf[100];
    buf[0] = 0x01;
    buf[1] = 0x00;
    buf[2] = 0xFF;
    buf[3] = 0xFF; /* declares 65535 bytes; the file has 96 */
    CHECK(fas_anim_bm_validate(buf, sizeof(buf), XBM_1024) == FasAnimBmInvalid);
}

static void test_bm_rejects_declared_length_one_over(void) {
    static uint8_t buf[64];
    size_t payload = sizeof(buf) - 4;
    buf[0] = 0x01;
    buf[1] = 0x00;
    buf[2] = (uint8_t)((payload + 1) & 0xFF);
    buf[3] = (uint8_t)((payload + 1) >> 8);
    CHECK(fas_anim_bm_validate(buf, sizeof(buf), XBM_1024) == FasAnimBmInvalid);
}

static void test_bm_tolerates_trailing_padding(void) {
    static uint8_t buf[64];
    size_t payload = 40; /* 20 bytes of slack after the payload */
    buf[0] = 0x01;
    buf[1] = 0x00;
    buf[2] = (uint8_t)(payload & 0xFF);
    buf[3] = (uint8_t)(payload >> 8);
    CHECK(fas_anim_bm_validate(buf, sizeof(buf), XBM_1024) == FasAnimBmCompressed);
}

static void test_bm_rejects_empty_payload(void) {
    static uint8_t buf[4] = {0x01, 0x00, 0x00, 0x00};
    CHECK(fas_anim_bm_validate(buf, sizeof(buf), XBM_1024) == FasAnimBmInvalid);
}

static void test_bm_rejects_truncated_header(void) {
    static uint8_t buf[3] = {0x01, 0x00, 0x10};
    CHECK(fas_anim_bm_validate(buf, sizeof(buf), XBM_1024) == FasAnimBmInvalid);
    CHECK(fas_anim_bm_validate(buf, 0, XBM_1024) == FasAnimBmInvalid);
}

static void test_bm_accepts_uncompressed_frame(void) {
    static uint8_t buf[1 + XBM_1024];
    buf[0] = 0x00;
    CHECK(fas_anim_bm_validate(buf, sizeof(buf), XBM_1024) == FasAnimBmRaw);
}

/* An uncompressed frame shorter than the image the caller is about to copy
 * would display whatever happened to be left in the read buffer. */
static void test_bm_rejects_short_uncompressed_frame(void) {
    static uint8_t buf[1 + XBM_1024 - 1];
    buf[0] = 0x00;
    CHECK(fas_anim_bm_validate(buf, sizeof(buf), XBM_1024) == FasAnimBmInvalid);
}

static void test_bm_rejects_unknown_container(void) {
    static uint8_t buf[64];
    buf[0] = 0x02;
    CHECK(fas_anim_bm_validate(buf, sizeof(buf), XBM_1024) == FasAnimBmInvalid);

    buf[0] = 0x01;
    buf[1] = 0x01; /* the packer always writes 01 00 */
    buf[2] = 0x10;
    buf[3] = 0x00;
    CHECK(fas_anim_bm_validate(buf, sizeof(buf), XBM_1024) == FasAnimBmInvalid);
}

/* ── Deferred-minor fixes ──────────────────────────────────────────────── */

/* A key given twice should mean what it means everywhere else in this
 * format -- the last one wins.  Appending instead would splice two lists
 * into one animation that matches neither. */
static void test_duplicate_frames_order_last_wins(void) {
    const char* t =
        "Filetype: Flipper Animation\r\n"
        "Width: 128\r\nHeight: 64\r\nPassive frames: 16\r\nFrame rate: 5\r\n"
        "Frames order: 0 1 2 3\r\n"
        "Frames order: 7 8\r\n";
    FasAnimMeta m;
    CHECK(PARSE(t, m) == FasAnimMetaOk);
    CHECK(m.frame_count == 2);
    CHECK(m.order[0] == 7);
    CHECK(m.order[1] == 8);
}

/* A long first list followed by a short one: appending would leave 64
 * entries in place and play the wrong frames. */
static void test_duplicate_frames_order_replaces_a_long_list(void) {
    char t[2048];
    int  n = snprintf(t, sizeof(t),
                      "Filetype: Flipper Animation\r\n"
                      "Width: 128\r\nHeight: 64\r\n"
                      "Passive frames: 200\r\nFrame rate: 5\r\n"
                      "Frames order:");
    for(int i = 0; i < 200; i++) {
        n += snprintf(t + n, sizeof(t) - (size_t)n, " %d", i % 16);
    }
    n += snprintf(t + n, sizeof(t) - (size_t)n, "\r\nFrames order: 3 4 5\r\n");

    FasAnimMeta m;
    CHECK(fas_anim_meta_parse(t, strlen(t), &m) == FasAnimMetaOk);
    CHECK(m.frame_count == 3);
    CHECK(m.order[0] == 3);
    CHECK(m.order[2] == 5);
    /* The first list's entries must not survive past the second's end. */
    CHECK(m.order[3] == 0);
    CHECK(m.order[FAS_ANIM_MAX_FRAMES - 1] == 0);
}

/* Minor 2: `long` is 64-bit on this host and 32-bit on the device, so a
 * value past INT32 wrapped on one and saturated on the other -- meaning
 * this suite was not a faithful oracle for exactly the inputs it exists
 * to pin down.  Clamping makes both agree.
 *
 * 4294967424 is 2^32 + 128: wrapping it through int yields 128, so the
 * host used to accept this as a valid full-screen animation. */
static void test_out_of_int_range_saturates(void) {
    FasAnimMeta m;

    const char* wide =
        "Filetype: Flipper Animation\r\n"
        "Width: 4294967424\r\nHeight: 64\r\n"
        "Passive frames: 1\r\nFrames order: 0\r\n";
    CHECK(PARSE(wide, m) == FasAnimMetaUnsupportedSize);

    /* 2^32 wraps to 0, which read as "no passive frames" on the host and
     * as INT32_MAX on the device. */
    const char* passive =
        "Filetype: Flipper Animation\r\n"
        "Width: 128\r\nHeight: 64\r\n"
        "Passive frames: 4294967296\r\nFrames order: 0 1\r\n";
    CHECK(PARSE(passive, m) == FasAnimMetaOk);
    CHECK(m.frame_count == 2);

    /* Saturating low must stay negative rather than wrapping positive. */
    const char* negative =
        "Filetype: Flipper Animation\r\n"
        "Width: -4294967168\r\nHeight: 64\r\n"
        "Passive frames: 1\r\nFrames order: 0\r\n";
    CHECK(PARSE(negative, m) == FasAnimMetaNoSize);
}

/* Minor 4: "Unsupported size" promises the animation is too big for the
 * screen.  A meta.txt that never says how big it is has not made that
 * claim, and the spec's table sends everything else unplayable to
 * "Cannot preview". */
static void test_absent_size_is_not_reported_as_oversize(void) {
    FasAnimMeta m;

    const char* none =
        "Filetype: Flipper Animation\r\n"
        "Passive frames: 1\r\nFrames order: 0\r\n";
    CHECK(PARSE(none, m) == FasAnimMetaNoSize);

    const char* no_height =
        "Filetype: Flipper Animation\r\n"
        "Width: 128\r\nPassive frames: 1\r\nFrames order: 0\r\n";
    CHECK(PARSE(no_height, m) == FasAnimMetaNoSize);

    const char* zero =
        "Filetype: Flipper Animation\r\n"
        "Width: 0\r\nHeight: 64\r\nPassive frames: 1\r\nFrames order: 0\r\n";
    CHECK(PARSE(zero, m) == FasAnimMetaNoSize);
}

/* Minor 3: only FAS_ANIM_META_MAX bytes of meta.txt are ever read, so a
 * longer file arrives with its last line cut at an arbitrary byte.  Cut
 * inside "Frames order" that silently shortened the animation; cut inside
 * "Filetype" it silently became unpreviewable.  The reader trims to the
 * last complete line so a half-line is never parsed as a whole one. */
static void test_whole_lines_drops_a_partial_tail(void) {
    const char* crlf = "Filetype: Flipper Animation\r\nFrames order: 0 1 2";
    CHECK(fas_anim_meta_whole_lines(crlf, strlen(crlf)) == 29);

    const char* complete = "Width: 128\r\nHeight: 64\r\n";
    CHECK(fas_anim_meta_whole_lines(complete, strlen(complete)) == 24);

    const char* lf = "Width: 128\nHeight: 6";
    CHECK(fas_anim_meta_whole_lines(lf, strlen(lf)) == 11);

    const char* bare_cr = "Width: 128\rHeight: 6";
    CHECK(fas_anim_meta_whole_lines(bare_cr, strlen(bare_cr)) == 11);

    /* No complete line at all: nothing is safe to parse. */
    const char* headless = "Filetype: Flipper Anim";
    CHECK(fas_anim_meta_whole_lines(headless, strlen(headless)) == 0);
    CHECK(fas_anim_meta_whole_lines("", 0) == 0);
}

/* Trimming must not change a file that already ends on a line boundary. */
static void test_whole_lines_is_identity_for_the_real_file(void) {
    size_t len = strlen(REAL_CRLF);
    CHECK(fas_anim_meta_whole_lines(REAL_CRLF, len) == len);
}

int main(void) {
    RUN(test_real_crlf_file);
    RUN(test_lf_parses_identically);
    RUN(test_no_trailing_newline);
    RUN(test_oversize_refused);
    RUN(test_frames_order_truncates);
    RUN(test_frame_rate_clamped);
    RUN(test_rejects_non_animation);
    RUN(test_rejects_unplayable);
    RUN(test_frame_count_is_the_lesser);
    RUN(test_active_frames_are_ignored);

    RUN(test_duplicate_frames_order_last_wins);
    RUN(test_duplicate_frames_order_replaces_a_long_list);
    RUN(test_out_of_int_range_saturates);
    RUN(test_absent_size_is_not_reported_as_oversize);
    RUN(test_whole_lines_drops_a_partial_tail);
    RUN(test_whole_lines_is_identity_for_the_real_file);

    RUN(test_bm_accepts_a_real_compressed_header);
    RUN(test_bm_rejects_overlong_declared_length);
    RUN(test_bm_rejects_declared_length_one_over);
    RUN(test_bm_tolerates_trailing_padding);
    RUN(test_bm_rejects_empty_payload);
    RUN(test_bm_rejects_truncated_header);
    RUN(test_bm_accepts_uncompressed_frame);
    RUN(test_bm_rejects_short_uncompressed_frame);
    RUN(test_bm_rejects_unknown_container);

    if(failures == 0) {
        printf("\nAll tests passed.\n");
        return 0;
    }
    printf("\n%d check(s) failed.\n", failures);
    return 1;
}
