#include "../animation_switcher.h"
#include "fas_scene.h"

/* Ranges mirror fas_scene_anim_settings.c so the per-animation editor and
 * the defaults editor always agree on what values are admissible. */
#define MIN_BUTTHURT_MIN 0
#define MIN_BUTTHURT_MAX 14
#define MAX_BUTTHURT_MIN 0
#define MAX_BUTTHURT_MAX 14
#define MIN_LEVEL_MIN    1
#define MIN_LEVEL_MAX    30
#define MAX_LEVEL_MIN    1
#define MAX_LEVEL_MAX    30
#define WEIGHT_MIN       1
#define WEIGHT_MAX       99

static void set_text(VariableItem* item, int val) {
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", val);
    variable_item_set_current_value_text(item, buf);
}

static void cb_def_min_butthurt(VariableItem* item) {
    FasApp* app = variable_item_get_context(item);
    int     val = (int)variable_item_get_current_value_index(item) + MIN_BUTTHURT_MIN;
    app->defaults.min_butthurt = val;
    set_text(item, val);
}

static void cb_def_max_butthurt(VariableItem* item) {
    FasApp* app = variable_item_get_context(item);
    int     val = (int)variable_item_get_current_value_index(item) + MAX_BUTTHURT_MIN;
    app->defaults.max_butthurt = val;
    set_text(item, val);
}

static void cb_def_min_level(VariableItem* item) {
    FasApp* app = variable_item_get_context(item);
    int     val = (int)variable_item_get_current_value_index(item) + MIN_LEVEL_MIN;
    app->defaults.min_level = val;
    set_text(item, val);
}

static void cb_def_max_level(VariableItem* item) {
    FasApp* app = variable_item_get_context(item);
    int     val = (int)variable_item_get_current_value_index(item) + MAX_LEVEL_MIN;
    app->defaults.max_level = val;
    set_text(item, val);
}

static void cb_def_weight(VariableItem* item) {
    FasApp* app = variable_item_get_context(item);
    int     val = (int)variable_item_get_current_value_index(item) + WEIGHT_MIN;
    app->defaults.weight = val;
    set_text(item, val);
}

void fas_scene_defaults_on_enter(void* context) {
    FasApp*           app  = context;
    FasDefaults*      d    = &app->defaults;
    VariableItemList* vl   = app->var_list;
    VariableItem*     item;

    variable_item_list_reset(vl);

    item = variable_item_list_add(
        vl, "Min Butthurt",
        MIN_BUTTHURT_MAX - MIN_BUTTHURT_MIN + 1,
        cb_def_min_butthurt, app);
    variable_item_set_current_value_index(item, d->min_butthurt - MIN_BUTTHURT_MIN);
    set_text(item, d->min_butthurt);

    item = variable_item_list_add(
        vl, "Max Butthurt",
        MAX_BUTTHURT_MAX - MAX_BUTTHURT_MIN + 1,
        cb_def_max_butthurt, app);
    variable_item_set_current_value_index(item, d->max_butthurt - MAX_BUTTHURT_MIN);
    set_text(item, d->max_butthurt);

    item = variable_item_list_add(
        vl, "Min Level",
        MIN_LEVEL_MAX - MIN_LEVEL_MIN + 1,
        cb_def_min_level, app);
    variable_item_set_current_value_index(item, d->min_level - MIN_LEVEL_MIN);
    set_text(item, d->min_level);

    item = variable_item_list_add(
        vl, "Max Level",
        MAX_LEVEL_MAX - MAX_LEVEL_MIN + 1,
        cb_def_max_level, app);
    variable_item_set_current_value_index(item, d->max_level - MAX_LEVEL_MIN);
    set_text(item, d->max_level);

    item = variable_item_list_add(
        vl, "Weight",
        WEIGHT_MAX - WEIGHT_MIN + 1,
        cb_def_weight, app);
    variable_item_set_current_value_index(item, d->weight - WEIGHT_MIN);
    set_text(item, d->weight);

    view_dispatcher_switch_to_view(app->view_dispatcher, FasViewVarList);
}

bool fas_scene_defaults_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void fas_scene_defaults_on_exit(void* context) {
    FasApp* app = context;
    /* Persist any changes the user made while in the scene. */
    fas_save_config(app);
    variable_item_list_reset(app->var_list);
}
