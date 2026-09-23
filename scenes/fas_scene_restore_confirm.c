#include "../animation_switcher.h"
#include "fas_scene.h"

static void fas_restore_confirm_cb(DialogExResult result, void* context) {
    FasApp* app = context;
    /* Right button = Restore (only present when a backup exists).
     * Left / Center = back out. */
    if(result == DialogExResultRight) {
        view_dispatcher_send_custom_event(app->view_dispatcher, FasEvtRestoreYes);
    } else {
        view_dispatcher_send_custom_event(app->view_dispatcher, FasEvtRestoreNo);
    }
}

void fas_scene_restore_confirm_on_enter(void* context) {
    FasApp* app = context;

    dialog_ex_reset(app->dialog_ex);
    dialog_ex_set_context(app->dialog_ex, app);
    dialog_ex_set_result_callback(app->dialog_ex, fas_restore_confirm_cb);

    if(fas_manifest_backup_exists(app)) {
        dialog_ex_set_header(
            app->dialog_ex, "Restore Backup?", 64, 8, AlignCenter, AlignCenter);
        dialog_ex_set_text(
            app->dialog_ex,
            "This will overwrite\nthe current manifest.",
            64, 32, AlignCenter, AlignCenter);
        dialog_ex_set_left_button_text(app->dialog_ex,  "Cancel");
        dialog_ex_set_right_button_text(app->dialog_ex, "Restore");
    } else {
        dialog_ex_set_header(
            app->dialog_ex, "No Backup", 64, 8, AlignCenter, AlignCenter);
        dialog_ex_set_text(
            app->dialog_ex,
            "No previous manifest\nto restore from.",
            64, 32, AlignCenter, AlignCenter);
        dialog_ex_set_center_button_text(app->dialog_ex, "OK");
    }

    view_dispatcher_switch_to_view(app->view_dispatcher, FasViewDialogEx);
}

bool fas_scene_restore_confirm_on_event(void* context, SceneManagerEvent event) {
    FasApp* app      = context;
    bool    consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        switch(event.event) {

        case FasEvtRestoreYes:
            if(fas_restore_manifest(app)) {
                scene_manager_set_scene_state(
                    app->scene_manager, FasSceneRebootConfirm, FasRebootSourceRestore);
                scene_manager_next_scene(app->scene_manager, FasSceneRebootConfirm);
            } else {
                scene_manager_search_and_switch_to_previous_scene(
                    app->scene_manager, FasSceneMainMenu);
            }
            consumed = true;
            break;

        case FasEvtRestoreNo:
            scene_manager_search_and_switch_to_previous_scene(
                app->scene_manager, FasSceneMainMenu);
            consumed = true;
            break;

        default:
            break;
        }
    }
    return consumed;
}

void fas_scene_restore_confirm_on_exit(void* context) {
    FasApp* app = context;
    dialog_ex_reset(app->dialog_ex);
}
