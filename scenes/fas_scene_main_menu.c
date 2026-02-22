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