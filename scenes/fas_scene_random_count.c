#include "../animation_switcher.h"
#include "fas_scene.h"

/* Count is stored in this scene's scene-state so it survives a back-out. */

static void fas_random_count_changed_cb(VariableItem* item) {
    FasApp* app = variable_item_get_context(item);
    uint8_t idx = variable_item_get_current_value_index(item);

    char label[12]; /* %d can emit 11 bytes; -Werror=format-truncation */
    snprintf(label, sizeof(label), "%d", idx + 1);
    variable_item_set_current_value_text(item, label);

    scene_manager_set_scene_state(
        app->scene_manager, FasSceneRandomCount, idx + 1);
}

static void fas_random_count_ok_cb(void* context, uint32_t index) {
    UNUSED(index);
    FasApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, FasEvtRandomCountDone);
}

/* Select exactly n of the visible animations, uniformly, in one pass and
 * with no temporary array (4 KB stack).  Each visible entry is taken with
 * probability needed/avail, which yields a uniform n-subset. */
static void fas_random_select(FasApp* app, int n) {
    int needed = n;
    int avail  = app->visible_count;

    for(int v = 0; v < app->visible_count; v++) {
        int  i    = app->visible_animations[v];
        bool take = false;
        if(avail > 0 && needed > 0) {
            take = (furi_hal_random_get() % (uint32_t)avail) < (uint32_t)needed;
        }
        app->animations[i].selected = take;
        if(take) needed--;
        avail--;
    }
}

void fas_scene_random_count_on_enter(void* context) {
    FasApp* app = context;

    int max = app->visible_count;
    if(max < 1) max = 1;

    uint32_t saved = scene_manager_get_scene_state(
        app->scene_manager, FasSceneRandomCount);
    int current = (saved >= 1 && (int)saved <= max) ? (int)saved : (max < 5 ? max : 5);

    variable_item_list_reset(app->var_list);

    VariableItem* item = variable_item_list_add(
        app->var_list, "Count", (uint8_t)max, fas_random_count_changed_cb, app);
    variable_item_set_current_value_index(item, (uint8_t)(current - 1));

    char label[12]; /* %d can emit 11 bytes; -Werror=format-truncation */
    snprintf(label, sizeof(label), "%d", current);
    variable_item_set_current_value_text(item, label);

    scene_manager_set_scene_state(
        app->scene_manager, FasSceneRandomCount, (uint32_t)current);

    variable_item_list_set_enter_callback(
        app->var_list, fas_random_count_ok_cb, app);

    view_dispatcher_switch_to_view(app->view_dispatcher, FasViewVarList);
}

bool fas_scene_random_count_on_event(void* context, SceneManagerEvent event) {
    FasApp* app      = context;
    bool    consumed = false;

    if(event.type == SceneManagerEventTypeCustom &&
       event.event == FasEvtRandomCountDone) {

        int n = (int)scene_manager_get_scene_state(
            app->scene_manager, FasSceneRandomCount);
        if(n < 1) n = 1;
        if(n > app->visible_count) n = app->visible_count;

        fas_random_select(app, n);

        /* Pop both this scene and Bulk Actions, back to the animation list.
         * returning_from_settings is still set from the Left press that
         * opened Bulk Actions, so the list preserves state on re-entry. */
        scene_manager_search_and_switch_to_previous_scene(
            app->scene_manager, FasSceneAnimList);
        consumed = true;
    }
    return consumed;
}

void fas_scene_random_count_on_exit(void* context) {
    FasApp* app = context;
    variable_item_list_reset(app->var_list);
}
