#include "flipper_animation_switcher.h"
#include "scenes/fas_scene.h"

#include <string.h>
#include <stdio.h>

/* ═══════════════════════════════════════════════════════════════════════
 * ViewDispatcher callbacks
 * ═══════════════════════════════════════════════════════════════════════ */

static bool fas_custom_event_cb(void* context, uint32_t event) {
    FasApp* app = context;
    return scene_manager_handle_custom_event(app->scene_manager, event);
}

static bool fas_navigation_event_cb(void* context) {
    FasApp* app = context;
    return scene_manager_handle_back_event(app->scene_manager);
}