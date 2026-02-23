#include "../flipper_animation_switcher.h"
#include "fas_scene.h"

static void fas_text_input_done_cb(void* context) {
    FasApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, FasEvtPlaylistNameDone);
}

void fas_scene_playlist_name_on_enter(void* context) {
    FasApp* app = context;
    memset(app->text_input_buffer, 0, FAS_PLAYLIST_NAME_LEN);

    text_input_reset(app->text_input);
    text_input_set_header_text(app->text_input, "Playlist name:");
    text_input_set_result_callback(
        app->text_input,
        fas_text_input_done_cb,
        app,
        app->text_input_buffer,
        FAS_PLAYLIST_NAME_LEN,
        /*clear_default_text=*/false);

    view_dispatcher_switch_to_view(app->view_dispatcher, FasViewTextInput);
}