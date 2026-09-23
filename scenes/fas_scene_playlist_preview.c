#include "../animation_switcher.h"
#include "fas_scene.h"

/* ── Read the playlist .txt and build a display string ─────────────── */
/* Names are written from offset header_reserve so the stats header (computed
 * during the same scan) can be back-filled and the names shifted into their
 * final position with a single memmove.  tail_reserve keeps room for the
 * "...and N more" line so a name is never truncated mid-way. */
typedef struct {
    char* out;
    int   out_size;
    int   pos;
    int   total;
    int   rendered;
    int   bh_min, bh_max;
    int   lv_min, lv_max;
    bool  stats_seen;
    int   header_reserve;
    int   tail_reserve;
} PreviewCtx;

static void preview_entry_cb(const AnimEntry* e, void* ctx) {
    PreviewCtx* p = ctx;

    p->total++;

    if(!p->stats_seen) {
        p->bh_min = e->min_butthurt;
        p->bh_max = e->max_butthurt;
        p->lv_min = e->min_level;
        p->lv_max = e->max_level;
        p->stats_seen = true;
    } else {
        if(e->min_butthurt < p->bh_min) p->bh_min = e->min_butthurt;
        if(e->max_butthurt > p->bh_max) p->bh_max = e->max_butthurt;
        if(e->min_level    < p->lv_min) p->lv_min = e->min_level;
        if(e->max_level    > p->lv_max) p->lv_max = e->max_level;
    }

    if(p->header_reserve + p->pos < p->out_size - p->tail_reserve - 80) {
        int written = snprintf(
            p->out + p->header_reserve + p->pos,
            p->out_size - p->header_reserve - p->pos,
            "- %s\n", e->name);
        if(written > 0) {
            p->pos += written;
            p->rendered++;
        }
    }
}

static void build_preview_text(FasApp* app, char* out, int out_size) {
    char path[FAS_PATH_LEN];
    snprintf(path, sizeof(path), "%s/%s.txt",
             FAS_PLAYLISTS_PATH,
             app->playlists[app->current_playlist_index].name);

    PreviewCtx p = {
        .out            = out,
        .out_size       = out_size,
        .header_reserve = 64,
        .tail_reserve   = 32,
    };

    if(!fas_parse_playlist_file(app, path, preview_entry_cb, &p)) {
        snprintf(out, out_size, "Could not read\nplaylist file.");
        return;
    }

    if(p.total == 0) {
        snprintf(out, out_size, "(empty playlist)");
        return;
    }

    if(p.rendered < p.total) {
        int written = snprintf(out + p.header_reserve + p.pos,
                               out_size - p.header_reserve - p.pos,
                               "...and %d more\n", p.total - p.rendered);
        if(written > 0) p.pos += written;
    }

    /* Compose stats header, then shift the names into final position. */
    char header[64];
    int  header_len = snprintf(
        header, sizeof(header),
        "%d %s\nBH %d-%d  LV %d-%d\n\n",
        p.total, p.total == 1 ? "anim" : "anims",
        p.bh_min, p.bh_max, p.lv_min, p.lv_max);
    if(header_len < 0) header_len = 0;
    if(header_len >= (int)sizeof(header)) header_len = (int)sizeof(header) - 1;

    memmove(out + header_len, out + p.header_reserve, p.pos);
    memcpy(out, header, header_len);
    out[header_len + p.pos] = '\0';
}

/* ── Scene handlers ───────────────────────────────────────────────────── */
void fas_scene_playlist_preview_on_enter(void* context) {
    FasApp* app = context;
    widget_reset(app->widget);

    /* Title */
    char title[FAS_PLAYLIST_NAME_LEN + 16];
    snprintf(title, sizeof(title), "Playlist: %s",
             app->playlists[app->current_playlist_index].name);
    widget_add_string_element(
        app->widget, 64, 2, AlignCenter, AlignTop, FontPrimary, title);

    /* Animation list as scrollable text.  Sized to hold all 128 possible
     * animations (rough upper bound: 128 * ~70 chars per "- name\n"). */
    static char preview_buf[4096];
    preview_buf[0] = '\0';
    build_preview_text(app, preview_buf, sizeof(preview_buf));
    widget_add_text_scroll_element(app->widget, 0, 14, 128, 50, preview_buf);

    view_dispatcher_switch_to_view(app->view_dispatcher, FasViewWidget);
}

bool fas_scene_playlist_preview_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void fas_scene_playlist_preview_on_exit(void* context) {
    FasApp* app = context;
    widget_reset(app->widget);
}