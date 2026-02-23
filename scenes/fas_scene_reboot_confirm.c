#include "../flipper_animation_switcher.h"
#include "fas_scene.h"

static void fas_reboot_confirm_cb(DialogExResult result, void* context) {
    FasApp* app = context;
    if(result == DialogExResultRight) {
        view_dispatcher_send_custom_event(app->view_dispatcher, FasEvtRebootYes);
    } else {
        view_dispatcher_send_custom_event(app->view_dispatcher, FasEvtRebootNo);
    }
}