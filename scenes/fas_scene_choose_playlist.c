#include "../flipper_animation_switcher.h"
#include "../views/fas_list_view.h"
#include "fas_scene.h"

static void fas_choose_playlist_cb(void* context, int index, FasListEvent event) {
    FasApp* app = context;
    app->current_playlist_index = index;

    switch(event) {
    case FasListEvtOkShort:
        view_dispatcher_send_custom_event(app->view_dispatcher, FasEvtChooseSelect);
        break;
    case FasListEvtOkLong:
        view_dispatcher_send_custom_event(app->view_dispatcher, FasEvtChoosePreview);
        break;
    default:
        break;
    }
}