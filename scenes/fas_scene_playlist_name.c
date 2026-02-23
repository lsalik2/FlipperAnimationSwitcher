#include "../flipper_animation_switcher.h"
#include "fas_scene.h"

static void fas_text_input_done_cb(void* context) {
    FasApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, FasEvtPlaylistNameDone);
}