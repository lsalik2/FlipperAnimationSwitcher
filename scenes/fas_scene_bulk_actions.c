#include "../animation_switcher.h"
#include "fas_scene.h"

static void fas_bulk_actions_cb(DialogExResult result, void* context) {
    FasApp* app = context;
    switch(result) {
    case DialogExResultLeft:
        view_dispatcher_send_custom_event(app->view_dispatcher, FasEvtBulkSelectAll);
        break;
    case DialogExResultCenter:
        view_dispatcher_send_custom_event(app->view_dispatcher, FasEvtBulkInvert);
        break;
    case DialogExResultRight:
        view_dispatcher_send_custom_event(app->view_dispatcher, FasEvtBulkSelectNone);
        break;
    default:
        break;
    }
}

void fas_scene_bulk_actions_on_enter(void* context) {
    FasApp* app = context;

    int selected = 0;
    for(int i = 0; i < app->animation_count; i++) {
        if(app->animations[i].selected) selected++;
    }

    char body[32];
    snprintf(body, sizeof(body), "%d / %d selected", selected, app->animation_count);

    dialog_ex_reset(app->dialog_ex);
    dialog_ex_set_header(
        app->dialog_ex, "Bulk Actions", 64, 10, AlignCenter, AlignCenter);
    dialog_ex_set_text(
        app->dialog_ex, body, 64, 32, AlignCenter, AlignCenter);
    dialog_ex_set_left_button_text(app->dialog_ex,   "All");
    dialog_ex_set_center_button_text(app->dialog_ex, "Invert");
    dialog_ex_set_right_button_text(app->dialog_ex,  "None");
    dialog_ex_set_context(app->dialog_ex, app);
    dialog_ex_set_result_callback(app->dialog_ex, fas_bulk_actions_cb);

    view_dispatcher_switch_to_view(app->view_dispatcher, FasViewDialogEx);
}

bool fas_scene_bulk_actions_on_event(void* context, SceneManagerEvent event) {
    FasApp* app      = context;
    bool    consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        switch(event.event) {
        case FasEvtBulkSelectAll:
            for(int i = 0; i < app->animation_count; i++) {
                app->animations[i].selected = true;
            }
            scene_manager_previous_scene(app->scene_manager);
            consumed = true;
            break;

        case FasEvtBulkSelectNone:
            for(int i = 0; i < app->animation_count; i++) {
                app->animations[i].selected = false;
            }
            scene_manager_previous_scene(app->scene_manager);
            consumed = true;
            break;

        case FasEvtBulkInvert:
            for(int i = 0; i < app->animation_count; i++) {
                app->animations[i].selected = !app->animations[i].selected;
            }
            scene_manager_previous_scene(app->scene_manager);
            consumed = true;
            break;

        default:
            break;
        }
    }
    return consumed;
}

void fas_scene_bulk_actions_on_exit(void* context) {
    FasApp* app = context;
    dialog_ex_reset(app->dialog_ex);
}
