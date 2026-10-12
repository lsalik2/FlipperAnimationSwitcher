#pragma once

#include <furi.h>
#include <storage/storage.h>

#include "anim_meta.h"

typedef struct FasAnimPlayer FasAnimPlayer;

/* Distinguished so the preview scene can show the right message.  The spec's
 * error table needs three different strings, which a bool cannot carry. */
typedef enum {
    FasAnimOk = 0,
    FasAnimErrNoMeta,        /* meta.txt missing, empty or unreadable     */
    FasAnimErrCannotPreview, /* wrong filetype, no passive frames/order   */
    FasAnimErrUnsupported,   /* declared size exceeds the Flipper screen  */
} FasAnimOpenResult;

FasAnimPlayer* fas_anim_player_alloc(Storage* storage);
void           fas_anim_player_free(FasAnimPlayer* p);

/* Reads and parses /ext/dolphin/<anim_name>/meta.txt. */
FasAnimOpenResult fas_anim_player_open(FasAnimPlayer* p, const char* anim_name);

int fas_anim_player_frame_count(const FasAnimPlayer* p); /* passive only */
int fas_anim_player_frame_rate(const FasAnimPlayer* p);  /* 1..30 fps    */
int fas_anim_player_width(const FasAnimPlayer* p);
int fas_anim_player_height(const FasAnimPlayer* p);

/* Decoded XBM bytes for playback position i (0 .. frame_count-1), or NULL on
 * a read or decode failure.
 *
 * The returned pointer is valid only until the next call to this function or
 * to fas_anim_player_free().  Copy from it immediately; never retain it. */
const uint8_t* fas_anim_player_frame(FasAnimPlayer* p, int i);
