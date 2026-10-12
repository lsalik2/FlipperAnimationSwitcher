#include "anim_player.h"

#include <lib/toolbox/compress.h>
#include <string.h>

#include "animation_switcher.h"

struct FasAnimPlayer {
    Storage*  storage;

    /* compress_decode() rather than compress_icon_decode(): the icon helper
     * does not return a status, it furi_check()s the decode and takes the
     * whole device down with a crash screen.  Its buffer is only
     * decode_buf_size + 4, so any frame whose payload expands past that --
     * which a single flipped bit in a heatshrink stream can cause, and SD
     * cards do get corrupted -- would reboot the Flipper rather than report
     * a bad frame.  compress_decode() returns false instead, and reports how
     * many bytes it actually produced. */
    Compress* decoder;
    uint8_t*  xbm; /* decode output, FAS_ANIM_XBM_MAX bytes */

    /* The raw file buffer is a field, not a local, for two reasons: it keeps
     * a kilobyte off the 4 KB app stack, and an uncompressed .bm is handed
     * back as a pointer *into* this buffer, so it has to outlive the frame
     * pointer the caller is given. */
    uint8_t* raw;

    char        name[FAS_ANIM_NAME_LEN];
    FasAnimMeta meta;
    bool        opened;
};

FasAnimPlayer* fas_anim_player_alloc(Storage* storage) {
    FasAnimPlayer* p = malloc(sizeof(FasAnimPlayer));
    furi_assert(p);
    memset(p, 0, sizeof(FasAnimPlayer));

    p->storage = storage;
    p->decoder = compress_alloc(CompressTypeHeatshrink, &compress_config_heatshrink_default);
    p->raw     = malloc(FAS_ANIM_FILE_MAX);
    p->xbm     = malloc(FAS_ANIM_XBM_MAX);
    furi_assert(p->raw);
    furi_assert(p->xbm);

    return p;
}

void fas_anim_player_free(FasAnimPlayer* p) {
    if(!p) return;
    compress_free(p->decoder);
    free(p->raw);
    free(p->xbm);
    free(p);
}

FasAnimOpenResult fas_anim_player_open(FasAnimPlayer* p, const char* anim_name) {
    p->opened = false;

    char path[FAS_PATH_LEN];
    snprintf(path, sizeof(path), "%s/%.64s/meta.txt", FAS_DOLPHIN_PATH, anim_name);

    File* f = storage_file_alloc(p->storage);
    if(!storage_file_open(f, path, FSAM_READ, FSOM_OPEN_EXISTING)) {
        storage_file_free(f);
        return FasAnimErrNoMeta;
    }

    uint64_t fsize = storage_file_size(f);

    /* Reuse the frame buffer: nothing has been decoded yet, and this avoids
     * a second kilobyte of heap for the lifetime of the player. */
    size_t n = storage_file_read(f, p->raw, FAS_ANIM_META_MAX);
    storage_file_close(f);
    storage_file_free(f);

    if(n == 0) return FasAnimErrNoMeta;

    /* Only the first FAS_ANIM_META_MAX bytes are read, so a longer meta.txt
     * arrives with its last line cut at an arbitrary byte.  Cut inside a
     * "Frames order" list that would have previewed a silently shortened
     * animation; cut inside any other key it would have reported a value
     * that is not the one in the file.  Drop the partial line instead. */
    if(fsize > FAS_ANIM_META_MAX) {
        n = fas_anim_meta_whole_lines((const char*)p->raw, n);
        if(n == 0) return FasAnimErrCannotPreview;
    }

    switch(fas_anim_meta_parse((const char*)p->raw, n, &p->meta)) {
    case FasAnimMetaOk:
        break;
    case FasAnimMetaUnsupportedSize:
        return FasAnimErrUnsupported;
    default:
        /* BadFiletype, NoPassiveFrames, NoFramesOrder and NoSize.  A
         * meta.txt with no size declared deliberately lands here rather
         * than on "Unsupported size", which would tell the user their
         * animation is too big for the screen when it never said how big
         * it is. */
        return FasAnimErrCannotPreview;
    }

    strncpy(p->name, anim_name, sizeof(p->name) - 1);
    p->name[sizeof(p->name) - 1] = '\0';
    p->opened                    = true;
    return FasAnimOk;
}

int fas_anim_player_frame_count(const FasAnimPlayer* p) {
    return p->opened ? p->meta.frame_count : 0;
}

int fas_anim_player_frame_rate(const FasAnimPlayer* p) {
    return p->opened ? p->meta.frame_rate : FAS_ANIM_RATE_DEFAULT;
}

int fas_anim_player_width(const FasAnimPlayer* p) {
    return p->opened ? p->meta.width : 0;
}

int fas_anim_player_height(const FasAnimPlayer* p) {
    return p->opened ? p->meta.height : 0;
}

const uint8_t* fas_anim_player_frame(FasAnimPlayer* p, int i) {
    if(!p->opened || i < 0 || i >= p->meta.frame_count) return NULL;

    char path[FAS_PATH_LEN];
    snprintf(path, sizeof(path), "%s/%.64s/frame_%d.bm",
             FAS_DOLPHIN_PATH, p->name, p->meta.order[i]);

    File* f = storage_file_alloc(p->storage);
    if(!storage_file_open(f, path, FSAM_READ, FSOM_OPEN_EXISTING)) {
        storage_file_free(f);
        return NULL;
    }

    uint64_t size = storage_file_size(f);

    /* Refuse anything that will not fit rather than truncating it: a short
     * read would be handed to the decoder as a valid frame. */
    if(size < 2 || size > FAS_ANIM_FILE_MAX) {
        storage_file_close(f);
        storage_file_free(f);
        return NULL;
    }

    size_t got = storage_file_read(f, p->raw, (size_t)size);
    storage_file_close(f);
    storage_file_free(f);

    if(got != (size_t)size) return NULL;

    /* The decoder takes its input length from the header inside the data and
     * ignores the size it is passed, which is safe for the compiled-in icons
     * this code path was written for and not for a file off the SD card.
     * Validate the container against the bytes actually read first. */
    size_t xbm_bytes =
        (size_t)((p->meta.width + 7) / 8) * (size_t)p->meta.height;
    FasAnimBmKind kind = fas_anim_bm_validate(p->raw, (size_t)size, xbm_bytes);

    if(kind == FasAnimBmRaw) {
        /* Hand back the XBM bytes directly instead of routing them through
         * the decoder, which is what it would do for this container anyway.
         *
         * This is what keeps the two readings of the file from ever
         * disagreeing: the decoder takes no input size, so the only thing it
         * is ever given here is a container whose declared length has already
         * been bounded against the bytes on disk.  Were it handed an
         * uncompressed frame and to dispatch differently than this function
         * does, it would read a length out of bytes 2 and 3 that nothing has
         * checked. */
        return p->raw + 1;
    }

    if(kind != FasAnimBmCompressed) return NULL;

    /* data_in_size is passed honestly, but note the decoder takes the input
     * length from the container header and ignores it -- which is why
     * fas_anim_bm_validate() above is still what bounds the read. */
    size_t decoded = 0;
    if(!compress_decode(
           p->decoder, p->raw, (size_t)size, p->xbm, FAS_ANIM_XBM_MAX, &decoded)) {
        return NULL;
    }

    /* A stream that ends early decodes "successfully" into a partial buffer.
     * Without this the view would copy a full frame's worth and paint the
     * tail of whatever the previous frame left behind. */
    if(decoded != xbm_bytes) return NULL;

    return p->xbm;
}
