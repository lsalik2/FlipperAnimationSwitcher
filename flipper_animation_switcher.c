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

/* ═══════════════════════════════════════════════════════════════════════
 * App lifecycle
 * ═══════════════════════════════════════════════════════════════════════ */

static FasApp* fas_app_alloc(void) {
    FasApp* app = malloc(sizeof(FasApp));
    furi_assert(app);
    memset(app, 0, sizeof(FasApp));

    /* Open system records */
    app->gui     = furi_record_open(RECORD_GUI);
    app->storage = furi_record_open(RECORD_STORAGE);

    /* Scene manager */
    app->scene_manager = scene_manager_alloc(&fas_scene_handlers, app);

    /* View dispatcher */
    app->view_dispatcher = view_dispatcher_alloc();
    view_dispatcher_set_event_callback_context(app->view_dispatcher, app);
    view_dispatcher_set_custom_event_callback(app->view_dispatcher, fas_custom_event_cb);
    view_dispatcher_set_navigation_event_callback(app->view_dispatcher, fas_navigation_event_cb);

    /* ── Allocate views ────────────────────────────────────────────── */

    app->menu = menu_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, FasViewMenu, menu_get_view(app->menu));

    app->list_view = fas_list_view_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, FasViewList, fas_list_view_get_view(app->list_view));

    app->var_list = variable_item_list_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, FasViewVarList,
        variable_item_list_get_view(app->var_list));

    app->text_input = text_input_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, FasViewTextInput, text_input_get_view(app->text_input));

    app->widget = widget_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, FasViewWidget, widget_get_view(app->widget));

    app->dialog_ex = dialog_ex_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, FasViewDialogEx, dialog_ex_get_view(app->dialog_ex));

    /* Attach to GUI as fullscreen app */
    view_dispatcher_attach_to_gui(
        app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);

    return app;
}

static void fas_app_free(FasApp* app) {
    /* Remove views before freeing them */
    view_dispatcher_remove_view(app->view_dispatcher, FasViewMenu);
    menu_free(app->menu);

    view_dispatcher_remove_view(app->view_dispatcher, FasViewList);
    fas_list_view_free(app->list_view);

    view_dispatcher_remove_view(app->view_dispatcher, FasViewVarList);
    variable_item_list_free(app->var_list);

    view_dispatcher_remove_view(app->view_dispatcher, FasViewTextInput);
    text_input_free(app->text_input);

    view_dispatcher_remove_view(app->view_dispatcher, FasViewWidget);
    widget_free(app->widget);

    view_dispatcher_remove_view(app->view_dispatcher, FasViewDialogEx);
    dialog_ex_free(app->dialog_ex);

    view_dispatcher_free(app->view_dispatcher);
    scene_manager_free(app->scene_manager);

    furi_record_close(RECORD_STORAGE);
    furi_record_close(RECORD_GUI);

    free(app);
}

/* ═══════════════════════════════════════════════════════════════════════
 * Storage helpers
 * ═══════════════════════════════════════════════════════════════════════ */

void fas_ensure_playlists_dir(FasApp* app) {
    storage_simply_mkdir(app->storage, FAS_PLAYLISTS_PATH);
}

/**
 * Scan /ext/dolphin for subdirectories — each subdirectory is an animation.
 * Resets animation_count and populates the animations[] array with defaults.
 */
bool fas_load_animations(FasApp* app) {
    app->animation_count = 0;

    /* Verify SD card / dolphin folder is accessible */
    if(storage_common_stat(app->storage, FAS_DOLPHIN_PATH, NULL) != FSE_OK) {
        return false;
    }

    File* dir = storage_file_alloc(app->storage);
    if(!storage_dir_open(dir, FAS_DOLPHIN_PATH)) {
        storage_file_free(dir);
        return false;
    }

    FileInfo fi;
    char     name[FAS_ANIM_NAME_LEN];

    while(storage_dir_read(dir, &fi, name, sizeof(name)) &&
        app->animation_count < FAS_MAX_ANIMATIONS) {
        /* Only include subdirectories (not manifest.txt or other files) */
        if(fi.flags & FSF_DIRECTORY) {
            AnimEntry* e = &app->animations[app->animation_count];
            strncpy(e->name, name, FAS_ANIM_NAME_LEN - 1);
            e->name[FAS_ANIM_NAME_LEN - 1] = '\0';
            e->selected      = false;
            e->min_butthurt  = FAS_DEFAULT_MIN_BUTTHURT;
            e->max_butthurt  = FAS_DEFAULT_MAX_BUTTHURT;
            e->min_level     = FAS_DEFAULT_MIN_LEVEL;
            e->max_level     = FAS_DEFAULT_MAX_LEVEL;
            e->weight        = FAS_DEFAULT_WEIGHT;
            app->animation_count++;
        }
    }

    storage_dir_close(dir);
    storage_file_free(dir);
    return (app->animation_count > 0);
}