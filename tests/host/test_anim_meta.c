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
    CHECK(m.order_truncated == false);
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

    const char* zero =
        "Filetype: Flipper Animation\r\n"
        "Width: 0\r\n"
        "Height: 64\r\n"
        "Passive frames: 1\r\n"
        "Frames order: 0\r\n";
    CHECK(PARSE(zero, m) == FasAnimMetaUnsupportedSize);
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
    CHECK(m.order_truncated == true);
    CHECK(m.frame_count == FAS_ANIM_MAX_FRAMES);
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

    if(failures == 0) {
        printf("\nAll tests passed.\n");
        return 0;
    }
    printf("\n%d check(s) failed.\n", failures);
    return 1;
}
