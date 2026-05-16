#include "../animation_switcher.h"
#include "fas_scene.h"

static void fas_anim_filter_done_cb(void* context) {
    FasApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, FasEvtAnimFilterDone);
}

void fas_scene_anim_filter_on_enter(void* context) {
    FasApp* app = context;

    text_input_reset(app->text_input);
    text_input_set_header_text(app->text_input, "Filter (empty = all):");
    text_input_set_result_callback(
        app->text_input,
        fas_anim_filter_done_cb,
        app,
        app->filter,
        FAS_ANIM_NAME_LEN,
        /*clear_default_text=*/false);

    view_dispatcher_switch_to_view(app->view_dispatcher, FasViewTextInput);
}

bool fas_scene_anim_filter_on_event(void* context, SceneManagerEvent event) {
    FasApp* app      = context;
    bool    consumed = false;

    if(event.type == SceneManagerEventTypeCustom &&
       event.event == FasEvtAnimFilterDone) {
        /* The TextInput writes directly into app->filter; the anim_list
         * scene re-applies the filter on re-entry. */
        scene_manager_previous_scene(app->scene_manager);
        consumed = true;
    }
    return consumed;
}

void fas_scene_anim_filter_on_exit(void* context) {
    UNUSED(context);
}
