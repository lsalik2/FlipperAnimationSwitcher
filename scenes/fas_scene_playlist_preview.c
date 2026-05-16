#include "../animation_switcher.h"
#include "fas_scene.h"

#include <stdlib.h>

/* ── Read the playlist .txt and build a display string ─────────────────── */
static void build_preview_text(FasApp* app, char* out, int out_size) {
    char path[FAS_PATH_LEN];
    snprintf(path, sizeof(path), "%s/%s.txt",
             FAS_PLAYLISTS_PATH,
             app->playlists[app->current_playlist_index].name);

    File* f = storage_file_alloc(app->storage);
    if(!storage_file_open(f, path, FSAM_READ, FSOM_OPEN_EXISTING)) {
        storage_file_free(f);
        snprintf(out, out_size, "Could not read\nplaylist file.");
        return;
    }

    /* Reserve tail space for a "...and N more" line so we never truncate
     * mid-name. Each name takes up to ~70 bytes when rendered.
     *
     * Names are written from offset `header_reserve` so the stats header
     * (computed during the same scan) can be back-filled and the names
     * shifted into their final position with a single memmove. */
    const int tail_reserve   = 32;
    const int header_reserve = 64;
    int  pos        = 0;
    int  total      = 0;
    int  rendered   = 0;
    int  bh_min = 0, bh_max = 0;
    int  lv_min = 0, lv_max = 0;
    bool stats_seen = false;
    char line[128];
    int  lp  = 0;
    char c;

    while(storage_file_read(f, &c, 1) == 1) {
        if(c == '\n' || c == '\r') {
            line[lp] = '\0';
            if(strncmp(line, "Name: ", 6) == 0) {
                total++;
                if(header_reserve + pos < out_size - tail_reserve - 80) {
                    int written = snprintf(
                        out + header_reserve + pos,
                        out_size - header_reserve - pos,
                        "- %s\n", line + 6);
                    if(written > 0) {
                        pos += written;
                        rendered++;
                    }
                }
            } else if(strncmp(line, "Min butthurt: ", 14) == 0) {
                int v = atoi(line + 14);
                if(!stats_seen || v < bh_min) bh_min = v;
            } else if(strncmp(line, "Max butthurt: ", 14) == 0) {
                int v = atoi(line + 14);
                if(!stats_seen || v > bh_max) bh_max = v;
            } else if(strncmp(line, "Min level: ", 11) == 0) {
                int v = atoi(line + 11);
                if(!stats_seen || v < lv_min) lv_min = v;
            } else if(strncmp(line, "Max level: ", 11) == 0) {
                int v = atoi(line + 11);
                if(!stats_seen || v > lv_max) lv_max = v;
                stats_seen = true;
            }
            lp = 0;
        } else if(lp < (int)sizeof(line) - 1) {
            line[lp++] = c;
        }
    }
    /* Flush last line (no trailing newline) */
    if(lp > 0) {
        line[lp] = '\0';
        if(strncmp(line, "Name: ", 6) == 0) {
            total++;
            if(header_reserve + pos < out_size - tail_reserve - 80) {
                int written = snprintf(
                    out + header_reserve + pos,
                    out_size - header_reserve - pos,
                    "- %s\n", line + 6);
                if(written > 0) {
                    pos += written;
                    rendered++;
                }
            }
        }
    }

    storage_file_close(f);
    storage_file_free(f);

    if(total == 0) {
        snprintf(out, out_size, "(empty playlist)");
        return;
    }

    if(rendered < total) {
        int written = snprintf(out + header_reserve + pos,
                               out_size - header_reserve - pos,
                               "...and %d more\n", total - rendered);
        if(written > 0) pos += written;
    }

    /* Compose stats header, then shift the names into final position. */
    char header[64];
    int  header_len = snprintf(
        header, sizeof(header),
        "%d %s\nBH %d-%d  LV %d-%d\n\n",
        total, total == 1 ? "anim" : "anims",
        bh_min, bh_max, lv_min, lv_max);
    if(header_len < 0) header_len = 0;
    if(header_len >= (int)sizeof(header)) header_len = (int)sizeof(header) - 1;

    memmove(out + header_len, out + header_reserve, pos);
    memcpy(out, header, header_len);
    out[header_len + pos] = '\0';
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