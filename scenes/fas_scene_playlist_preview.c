#include "../flipper_animation_switcher.h"
#include "fas_scene.h"

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

    int  pos = 0;
    char line[128];
    int  lp  = 0;
    char c;

    while(storage_file_read(f, &c, 1) == 1 && pos < out_size - 40) {
        if(c == '\n' || c == '\r') {
            line[lp] = '\0';
            if(strncmp(line, "Name: ", 6) == 0) {
                int written = snprintf(out + pos, out_size - pos, "- %s\n", line + 6);
                if(written > 0) pos += written;
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
            snprintf(out + pos, out_size - pos, "- %s\n", line + 6);
        }
    }

    if(pos == 0) snprintf(out, out_size, "(empty playlist)");

    storage_file_close(f);
    storage_file_free(f);
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

    /* Animation list as scrollable text */
    static char preview_buf[512];
    preview_buf[0] = '\0';
    build_preview_text(app, preview_buf, sizeof(preview_buf));
    widget_add_text_scroll_element(app->widget, 0, 14, 128, 50, preview_buf);

    view_dispatcher_switch_to_view(app->view_dispatcher, FasViewWidget);
}