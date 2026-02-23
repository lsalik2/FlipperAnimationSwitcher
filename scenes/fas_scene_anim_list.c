#include "../flipper_animation_switcher.h"
#include "../views/fas_list_view.h"
#include "fas_scene.h"

/* ── List-view callback (runs on the input thread) ────────────────────── */
static void fas_anim_list_cb(void* context, int index, FasListEvent event) {
    FasApp* app = context;

    switch(event) {
    case FasListEvtOkShort:
        /* Checkbox was toggled inside the view; sync the value back to app */
        app->animations[index].selected = fas_list_view_get_checked(app->list_view, index);
        break;

    case FasListEvtOkLong:
        /* Open per-animation settings.  Set flag so on_enter preserves state. */
        app->current_anim_index         = index;
        app->returning_from_settings    = true;
        view_dispatcher_send_custom_event(
            app->view_dispatcher, FasEvtAnimListOpenSettings);
        break;

    case FasListEvtRight:
        view_dispatcher_send_custom_event(app->view_dispatcher, FasEvtAnimListDone);
        break;

    default:
        break;
    }
}