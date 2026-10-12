#include "anim_player.h"

#include <lib/toolbox/compress.h>
#include <string.h>

#include "animation_switcher.h"

struct FasAnimPlayer {
    Storage*      storage;
    CompressIcon* decoder;

    /* The raw file buffer is a field, not a local, for two reasons: it keeps
     * a kilobyte off the 4 KB app stack, and compress_icon_decode returns a
     * pointer *into* this buffer for an uncompressed .bm, so it has to
     * outlive the decoded pointer the caller is handed. */
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
    p->decoder = compress_icon_alloc(FAS_ANIM_XBM_MAX);
    p->raw     = malloc(FAS_ANIM_FILE_MAX);
    furi_assert(p->raw);

    return p;
}

void fas_anim_player_free(FasAnimPlayer* p) {
    if(!p) return;
    compress_icon_free(p->decoder);
    free(p->raw);
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

    /* Reuse the frame buffer: nothing has been decoded yet, and this avoids
     * a second kilobyte of heap for the lifetime of the player. */
    size_t n = storage_file_read(f, p->raw, FAS_ANIM_META_MAX);
    storage_file_close(f);
    storage_file_free(f);

    if(n == 0) return FasAnimErrNoMeta;

    switch(fas_anim_meta_parse((const char*)p->raw, n, &p->meta)) {
    case FasAnimMetaOk:
        break;
    case FasAnimMetaUnsupportedSize:
        return FasAnimErrUnsupported;
    default:
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

    uint8_t* out = NULL;
    compress_icon_decode(p->decoder, p->raw, &out);
    return out;
}
