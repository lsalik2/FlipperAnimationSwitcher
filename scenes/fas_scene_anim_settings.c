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