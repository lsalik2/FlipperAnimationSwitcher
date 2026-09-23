#include "../animation_switcher.h"
#include "fas_scene.h"

typedef enum {
    FasBulkIdxAll = 0,
    FasBulkIdxNone,
    FasBulkIdxInvert,
} FasBulkIdx;

static void fas_bulk_actions_cb(void* context, uint32_t index) {
    FasApp* app = context;
    switch(index) {
    case FasBulkIdxAll:
        view_dispatcher_send_custom_event(app->view_dispatcher, FasEvtBulkSelectAll);
        break;
    case FasBulkIdxNone:
        view_dispatcher_send_custom_event(app->view_dispatcher, FasEvtBulkSelectNone);
        break;
    case FasBulkIdxInvert:
        view_dispatcher_send_custom_event(app->view_dispatcher, FasEvtBulkInvert);
        break;
    default:
        break;
    }
}

void fas_scene_bulk_actions_on_enter(void* context) {
    FasApp* app = context;

    /* Count against the visible set: bulk actions act on what you can see. */
    int selected = 0;
    for(int v = 0; v < app->visible_count; v++) {
        if(app->animations[app->visible_animations[v]].selected) selected++;
    }

    char header[48];
    snprintf(header, sizeof(header), "Bulk: %d / %d shown",
             selected, app->visible_count);

    submenu_reset(app->submenu);
    submenu_set_header(app->submenu, header);
    submenu_add_item(app->submenu, "Select All",  FasBulkIdxAll,    fas_bulk_actions_cb, app);
    submenu_add_item(app->submenu, "Select None", FasBulkIdxNone,   fas_bulk_actions_cb, app);
    submenu_add_item(app->submenu, "Invert",      FasBulkIdxInvert, fas_bulk_actions_cb, app);

    view_dispatcher_switch_to_view(app->view_dispatcher, FasViewSubmenu);
}

bool fas_scene_bulk_actions_on_event(void* context, SceneManagerEvent event) {
    FasApp* app      = context;
    bool    consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        switch(event.event) {
        case FasEvtBulkSelectAll:
            for(int v = 0; v < app->visible_count; v++) {
                app->animations[app->visible_animations[v]].selected = true;
            }
            scene_manager_previous_scene(app->scene_manager);
            consumed = true;
            break;

        case FasEvtBulkSelectNone:
            for(int v = 0; v < app->visible_count; v++) {
                app->animations[app->visible_animations[v]].selected = false;
            }
            scene_manager_previous_scene(app->scene_manager);
            consumed = true;
            break;

        case FasEvtBulkInvert:
            for(int v = 0; v < app->visible_count; v++) {
                int i = app->visible_animations[v];
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
    submenu_reset(app->submenu);
}
