#include "../flipper_animation_switcher.h"
#include "fas_scene.h"

typedef enum {
    FasMainIdxCreate = 0,
    FasMainIdxChoose,
    FasMainIdxDelete,
    FasMainIdxAbout,
} FasMainMenuIdx;


static void fas_main_menu_cb(void* context, uint32_t index) {
    FasApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, (uint32_t)index);
}

void fas_scene_main_menu_on_enter(void* context) {
    FasApp* app = context;
    menu_reset(app->menu);
    menu_add_item(app->menu, "Create Playlist", NULL, FasMainIdxCreate,  fas_main_menu_cb, app);
    menu_add_item(app->menu, "Choose Playlist", NULL, FasMainIdxChoose,  fas_main_menu_cb, app);
    menu_add_item(app->menu, "Delete Playlist", NULL, FasMainIdxDelete,  fas_main_menu_cb, app);
    menu_add_item(app->menu, "About / Help",    NULL, FasMainIdxAbout,   fas_main_menu_cb, app);
    view_dispatcher_switch_to_view(app->view_dispatcher, FasViewMenu);
}

bool fas_scene_main_menu_on_event(void* context, SceneManagerEvent event) {
    FasApp* app      = context;
    bool    consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        switch(event.event) {
        case FasMainIdxCreate:
            scene_manager_next_scene(app->scene_manager, FasSceneAnimList);
            consumed = true;
            break;
        case FasMainIdxChoose:
            scene_manager_next_scene(app->scene_manager, FasSceneChoosePlaylist);
            consumed = true;
            break;
        case FasMainIdxDelete:
            scene_manager_next_scene(app->scene_manager, FasSceneDeletePlaylist);
            consumed = true;
            break;
        case FasMainIdxAbout:
            scene_manager_next_scene(app->scene_manager, FasSceneAbout);
            consumed = true;
            break;
        default:
            break;
        }
    }
    return consumed;
}