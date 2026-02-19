#include "fas_scene.h"

const SceneManagerHandlers fas_scene_handlers = {
    .on_enter_handlers = fas_on_enter_handlers,
    .on_event_handlers = fas_on_event_handlers,
    .on_exit_handlers  = fas_on_exit_handlers,
    .scene_num         = FasSceneCount,
};