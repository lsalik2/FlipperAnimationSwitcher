#include "../animation_switcher.h"
#include "../views/fas_list_view.h"
#include "fas_scene.h"

/* ── List-view callback (runs on the input thread) ──────────────────────
 * The list view's "index" is a row in the filtered (visible) list.  We
 * translate it through app->visible_animations[] to find the underlying
 * animation.  Events that don't operate on a specific row (Right, Left
 * long) fire regardless of whether the index points at a real entry. */
static void fas_anim_list_cb(void* context, int index, FasListEvent event) {
    FasApp* app = context;

    switch(event) {
    case FasListEvtOkShort:
        if(index >= 0 && index < app->visible_count) {
            int actual = app->visible_animations[index];
            app->animations[actual].selected =
                fas_list_view_get_checked(app->list_view, index);
        }
        break;

    case FasListEvtOkLong:
        /* Open per-animation settings.  Set flag so on_enter preserves state. */
        if(index >= 0 && index < app->visible_count) {
            app->current_anim_index      = app->visible_animations[index];
            app->returning_from_settings = true;
            view_dispatcher_send_custom_event(
                app->view_dispatcher, FasEvtAnimListOpenSettings);
        }
        break;

    case FasListEvtRight:
        view_dispatcher_send_custom_event(app->view_dispatcher, FasEvtAnimListDone);
        break;

    case FasListEvtLeftLong:
        /* Open filter text input.  Set flag so on_enter preserves state. */
        app->returning_from_settings = true;
        view_dispatcher_send_custom_event(
            app->view_dispatcher, FasEvtAnimListOpenFilter);
        break;

    default:
        break;
    }
}

/* ── Scene handlers ───────────────────────────────────────────────────── */
void fas_scene_anim_list_on_enter(void* context) {
    FasApp* app = context;

    /* Fresh entry (first time or after completing/aborting a playlist) */
    if(!app->returning_from_settings) {
        fas_load_animations(app);
        app->current_anim_index = 0;
        app->filter[0]          = '\0';
    }
    app->returning_from_settings = false;

    fas_apply_anim_filter(app);

    fas_list_view_reset(app->list_view);
    fas_list_view_set_callback(app->list_view, fas_anim_list_cb, app);

    if(app->animation_count == 0) {
        fas_list_view_add_item(app->list_view, "No animations found", false, false);
    } else if(app->visible_count == 0) {
        /* Filter is set but matches nothing.  Long-press Left to clear it. */
        char msg[FAS_LIST_LABEL_LEN];
        snprintf(msg, sizeof(msg), "No match for \"%s\"", app->filter);
        fas_list_view_add_item(app->list_view, msg, false, false);
    } else {
        for(int i = 0; i < app->visible_count; i++) {
            int actual = app->visible_animations[i];
            fas_list_view_add_item(
                app->list_view,
                app->animations[actual].name,
                /*has_checkbox=*/true,
                app->animations[actual].selected);
        }
        /* Restore cursor: find the underlying animation's row in the
         * (possibly newly-filtered) visible list.  If it's been filtered
         * out, fall back to the top of the list. */
        int cursor_row = 0;
        for(int i = 0; i < app->visible_count; i++) {
            if(app->visible_animations[i] == app->current_anim_index) {
                cursor_row = i;
                break;
            }
        }
        fas_list_view_set_cursor(app->list_view, cursor_row);
    }

    view_dispatcher_switch_to_view(app->view_dispatcher, FasViewList);
}

bool fas_scene_anim_list_on_event(void* context, SceneManagerEvent event) {
    FasApp* app      = context;
    bool    consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        switch(event.event) {

        case FasEvtAnimListOpenSettings:
            scene_manager_next_scene(app->scene_manager, FasSceneAnimSettings);
            consumed = true;
            break;

        case FasEvtAnimListOpenFilter:
            scene_manager_next_scene(app->scene_manager, FasSceneAnimFilter);
            consumed = true;
            break;

        case FasEvtAnimListDone: {
            /* Only proceed if at least one animation is selected.  Counts
             * across the full list so hidden-but-checked entries still
             * count toward the playlist. */
            bool any = false;
            for(int i = 0; i < app->animation_count; i++) {
                if(app->animations[i].selected) { any = true; break; }
            }
            if(any) {
                scene_manager_next_scene(app->scene_manager, FasScenePlaylistName);
            }
            consumed = true;
            break;
        }

        default:
            break;
        }
    }
    return consumed;
}

void fas_scene_anim_list_on_exit(void* context) {
    FasApp* app = context;
    /* Sync checkbox state and cursor from the view back to app data.
     * Iteration is over visible rows; entries hidden by the filter keep
     * their existing selected state. */
    for(int i = 0; i < app->visible_count; i++) {
        int actual = app->visible_animations[i];
        app->animations[actual].selected =
            fas_list_view_get_checked(app->list_view, i);
    }
    int cursor = fas_list_view_get_cursor(app->list_view);
    if(cursor >= 0 && cursor < app->visible_count) {
        app->current_anim_index = app->visible_animations[cursor];
    }
}
