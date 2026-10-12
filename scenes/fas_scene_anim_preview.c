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

/* Arms the next tick.  The timer is deliberately one-shot and re-armed only
 * once a tick has been handled, so at most one tick is ever in flight.
 *
 * A periodic timer would keep firing whether or not the previous frame had
 * been drawn.  Reading and decoding a frame off the SD card is not bounded
 * by the frame period, and the dispatcher's custom-event queue holds 16:
 * once it fills, view_dispatcher_send_custom_event() blocks forever, which
 * parks the timer-daemon thread.  If the app thread then handled Back, the
 * furi_timer_free() in on_exit would wait on that same daemon, and the only
 * thread that could drain the queue is the one doing the waiting.  Pacing
 * the timer off the handler removes the backlog, and the deadlock with it. */
static void fas_anim_preview_arm(FasApp* app) {
    int rate = fas_anim_player_frame_rate(app->anim_player);
    /* rate is guaranteed 1..FAS_ANIM_RATE_MAX by the parser, so this division
     * is safe and the period is between 33 and 1000 ms. */
    furi_timer_start(app->anim_timer, furi_ms_to_ticks(1000u / (uint32_t)rate));
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
            app->anim_timer =
                furi_timer_alloc(fas_anim_preview_timer_cb, FuriTimerTypeOnce, app);
            fas_anim_preview_arm(app);
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

            if(fas_anim_preview_show(app, app->anim_position)) {
                fas_anim_preview_arm(app);
            } else {
                /* A frame file vanished, was truncated, or failed to decode
                 * mid-playback.  Simply not re-arming stops playback -- with
                 * a one-shot timer there is nothing to cancel, so this path
                 * cannot touch a timer that was never allocated. */
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

    /* Clear the frame so the view falls back to drawing its message rather
     * than a stale image if it is ever shown again before a frame is set.
     * The model's 1 KB buffer is inline and lives for the life of the app;
     * this does not return any memory. */
    fas_anim_view_set_frame(app->anim_view, NULL, 0, 0);
}
