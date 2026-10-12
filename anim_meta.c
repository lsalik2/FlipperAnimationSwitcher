#include "anim_meta.h"

#include <stdlib.h>
#include <string.h>

/* Returns the length of the line at `s` excluding its terminator, and sets
 * *next to the first character of the following line.  CRLF counts as one
 * terminator, and a final line with no terminator is returned as-is. */
static size_t meta_line(const char* s, const char* end, const char** next) {
    const char* p = s;
    while(p < end && *p != '\n' && *p != '\r') p++;
    size_t len = (size_t)(p - s);

    if(p < end && *p == '\r') p++;
    if(p < end && *p == '\n') p++;
    *next = p;
    return len;
}

/* If `line` is exactly "<key>:" followed by optional blank space, returns a
 * pointer to the value and writes its length to *vlen.  Returns NULL when
 * the key does not match, so "Width" never matches "Width2". */
static const char* meta_value(const char* line, size_t len, const char* key, size_t* vlen) {
    size_t klen = strlen(key);
    if(len < klen + 1) return NULL;
    if(memcmp(line, key, klen) != 0) return NULL;
    if(line[klen] != ':') return NULL;

    size_t i = klen + 1;
    while(i < len && (line[i] == ' ' || line[i] == '\t')) i++;
    *vlen = len - i;
    return line + i;
}

/* strtol over a slice that is not NUL-terminated. */
static int meta_int(const char* v, size_t vlen, int fallback) {
    char buf[16];
    if(vlen == 0 || vlen >= sizeof(buf)) return fallback;
    memcpy(buf, v, vlen);
    buf[vlen] = '\0';

    char* endp = NULL;
    long  n    = strtol(buf, &endp, 10);
    if(endp == buf) return fallback;
    return (int)n;
}

FasAnimMetaResult fas_anim_meta_parse(const char* text, size_t len, FasAnimMeta* out) {
    memset(out, 0, sizeof(*out));
    out->frame_rate = FAS_ANIM_RATE_DEFAULT;

    bool filetype_ok = false;
    bool order_seen  = false;
    int  passive     = -1;
    int  order_count = 0;

    const char* p   = text;
    const char* end = text + len;

    while(p < end) {
        const char* line = p;
        size_t      llen = meta_line(p, end, &p);
        if(llen == 0) continue; /* blank separator lines are normal here */

        size_t      vlen = 0;
        const char* v    = NULL;

        if((v = meta_value(line, llen, "Filetype", &vlen)) != NULL) {
            /* Must be exact: "Flipper Animation Manifest" is a playlist. */
            const char* want = "Flipper Animation";
            filetype_ok = (vlen == strlen(want)) && (memcmp(v, want, vlen) == 0);

        } else if((v = meta_value(line, llen, "Width", &vlen)) != NULL) {
            out->width = meta_int(v, vlen, 0);

        } else if((v = meta_value(line, llen, "Height", &vlen)) != NULL) {
            out->height = meta_int(v, vlen, 0);

        } else if((v = meta_value(line, llen, "Passive frames", &vlen)) != NULL) {
            passive = meta_int(v, vlen, -1);

        } else if((v = meta_value(line, llen, "Frame rate", &vlen)) != NULL) {
            out->frame_rate = meta_int(v, vlen, FAS_ANIM_RATE_DEFAULT);

        } else if((v = meta_value(line, llen, "Frames order", &vlen)) != NULL) {
            order_seen = true;
            size_t i   = 0;
            while(i < vlen) {
                while(i < vlen && (v[i] == ' ' || v[i] == '\t')) i++;
                if(i >= vlen) break;

                size_t start = i;
                while(i < vlen && v[i] != ' ' && v[i] != '\t') i++;

                if(order_count < FAS_ANIM_MAX_FRAMES) {
                    out->order[order_count++] = meta_int(v + start, i - start, 0);
                } else {
                    out->order_truncated = true;
                }
            }
        }
    }

    if(!filetype_ok) return FasAnimMetaBadFiletype;

    /* Checked before anything else about frames: the render view's buffer is
     * sized for exactly FAS_ANIM_MAX_W x FAS_ANIM_MAX_H. */
    if(out->width <= 0 || out->height <= 0 || out->width > FAS_ANIM_MAX_W ||
       out->height > FAS_ANIM_MAX_H) {
        return FasAnimMetaUnsupportedSize;
    }

    if(passive <= 0) return FasAnimMetaNoPassiveFrames;
    if(!order_seen || order_count == 0) return FasAnimMetaNoFramesOrder;

    out->frame_count = (passive < order_count) ? passive : order_count;

    if(out->frame_rate < 1) out->frame_rate = FAS_ANIM_RATE_DEFAULT;
    if(out->frame_rate > FAS_ANIM_RATE_MAX) out->frame_rate = FAS_ANIM_RATE_MAX;

    return FasAnimMetaOk;
}
