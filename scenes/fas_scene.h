#pragma once

#include <gui/scene_manager.h>

/* ── Scene ID enum ────────────────────────────────────────────────────── */
#define ADD_SCENE(prefix, name, id) FasScene##id,
typedef enum {
#include "fas_scene_config.h"
    FasSceneCount,
} FasScene;
#undef ADD_SCENE