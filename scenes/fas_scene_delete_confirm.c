#include "../flipper_animation_switcher.h"
#include "fas_scene.h"

static void fas_delete_confirm_cb(DialogExResult result, void* context) {
    FasApp* app = context;
    if(result == DialogExResultRight) {
        view_dispatcher_send_custom_event(app->view_dispatcher, FasEvtDeleteYes);
    } else {
        view_dispatcher_send_custom_event(app->view_dispatcher, FasEvtDeleteNo);
    }
}