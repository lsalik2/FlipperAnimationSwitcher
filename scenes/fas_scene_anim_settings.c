#include "../flipper_animation_switcher.h"
#include "fas_scene.h"

/* ── Range definitions ───────────────────────────────────────────────── */
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

/* ── Helper: write the numeric label for an item ─────────────────────── */
static void set_text(VariableItem* item, int val) {
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", val);
    variable_item_set_current_value_text(item, buf);
}

/* ── Change callbacks ────────────────────────────────────────────────── */
static void cb_min_butthurt(VariableItem* item) {
    FasApp* app = variable_item_get_context(item);
    int     val = (int)variable_item_get_current_value_index(item) + MIN_BUTTHURT_MIN;
    app->animations[app->current_anim_index].min_butthurt = val;
    set_text(item, val);
}

static void cb_max_butthurt(VariableItem* item) {
    FasApp* app = variable_item_get_context(item);
    int     val = (int)variable_item_get_current_value_index(item) + MAX_BUTTHURT_MIN;
    app->animations[app->current_anim_index].max_butthurt = val;
    set_text(item, val);
}

static void cb_min_level(VariableItem* item) {
    FasApp* app = variable_item_get_context(item);
    int     val = (int)variable_item_get_current_value_index(item) + MIN_LEVEL_MIN;
    app->animations[app->current_anim_index].min_level = val;
    set_text(item, val);
}

static void cb_max_level(VariableItem* item) {
    FasApp* app = variable_item_get_context(item);
    int     val = (int)variable_item_get_current_value_index(item) + MAX_LEVEL_MIN;
    app->animations[app->current_anim_index].max_level = val;
    set_text(item, val);
}