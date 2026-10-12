#include "../animation_switcher.h"
#include "../anim_player.h"
#include "../views/fas_anim_view.h"
#include "fas_scene.h"

/* ── Timer callback (runs on the furi timer thread) ─────────────────────
 * It must not touch the view model or the player: every other piece of
 * state in this app is mutated on the GUI thread, and the view model is
 * not safe to write from here.  All it does is post an event. */
static void fas_anim_preview_timer_cb(void* context) {
    FasApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, FasEvtAnimPreviewTick);
}

/* Pushes playback position `pos` to the view.  Returns false when the frame
 * could not be read or decoded. */
static bool fas_anim_preview_show(FasApp* app, int pos) {
    const uint8_t* frame = fas_anim_player_frame(app->anim_player, pos);
    if(!frame) return false;

    fas_anim_view_set_frame(
        app->anim_view,
        frame,
        fas_anim_player_width(app->anim_player),
        fas_anim_player_height(app->anim_player));
    return true;
}

void fas_scene_anim_preview_on_enter(void* context) {
    FasApp* app = context;

    app->anim_position = 0;
    app->anim_timer    = NULL;
    app->anim_player   = fas_anim_player_alloc(app->storage);

    const char* name = app->animations[app->current_anim_index].name;

    switch(fas_anim_player_open(app->anim_player, name)) {
    case FasAnimErrNoMeta:
        fas_anim_view_set_message(app->anim_view, "No meta.txt");
        break;

    case FasAnimErrUnsupported:
        fas_anim_view_set_message(app->anim_view, "Unsupported size");
        break;

    case FasAnimErrCannotPreview:
        fas_anim_view_set_message(app->anim_view, "Cannot preview");
        break;

    case FasAnimOk:
        if(!fas_anim_preview_show(app, 0)) {
            fas_anim_view_set_message(app->anim_view, "Frame read error");
            break;
        }

        /* A single-frame animation needs no timer at all. */
        if(fas_anim_player_frame_count(app->anim_player) > 1) {
            int rate = fas_anim_player_frame_rate(app->anim_player);
            /* rate is guaranteed 1..FAS_ANIM_RATE_MAX by the parser, so this
             * division is safe and the period is between 33 and 1000 ms. */
            uint32_t period_ms = 1000u / (uint32_t)rate;

            app->anim_timer =
                furi_timer_alloc(fas_anim_preview_timer_cb, FuriTimerTypePeriodic, app);
            furi_timer_start(app->anim_timer, furi_ms_to_ticks(period_ms));
        }
        break;
    }

    view_dispatcher_switch_to_view(app->view_dispatcher, FasViewAnimPreview);
}

bool fas_scene_anim_preview_on_event(void* context, SceneManagerEvent event) {
    FasApp* app      = context;
    bool    consumed = false;

    if(event.type == SceneManagerEventTypeCustom &&
       event.event == FasEvtAnimPreviewTick) {
        int count = fas_anim_player_frame_count(app->anim_player);
        if(count > 0) {
            app->anim_position = (app->anim_position + 1) % count;

            if(!fas_anim_preview_show(app, app->anim_position)) {
                /* A frame file vanished or failed to decode mid-playback.
                 * Stop rather than flicker between good and bad frames. */
                furi_timer_stop(app->anim_timer);
                fas_anim_view_set_message(app->anim_view, "Frame read error");
            }
        }
        consumed = true;
    }

    return consumed;
}

void fas_scene_anim_preview_on_exit(void* context) {
    FasApp* app = context;

    /* Stop before freeing, and free the timer before the player: a tick that
     * is already queued is delivered to whichever scene is current by then,
     * and no other scene handles FasEvtAnimPreviewTick. */
    if(app->anim_timer) {
        furi_timer_stop(app->anim_timer);
        furi_timer_free(app->anim_timer);
        app->anim_timer = NULL;
    }

    if(app->anim_player) {
        fas_anim_player_free(app->anim_player);
        app->anim_player = NULL;
    }

    /* Release the view's 1 KB frame copy so it is not held while other
     * scenes run. */
    fas_anim_view_set_frame(app->anim_view, NULL, 0, 0);
}
