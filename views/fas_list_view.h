#pragma once

#include <gui/view.h>
#include <stdbool.h>

/* Maximum items the list can hold */
#define FAS_LIST_MAX_ITEMS 128 // TODO increase this if possible
#define FAS_LIST_LABEL_LEN  64

/* Events fired via the user callback */
typedef enum {
    FasListEvtOkShort,  /* OK pressed briefly (toggles checkbox if present) */
    FasListEvtOkLong,   /* OK held                                          */
    FasListEvtRight,    /* Right arrow pressed                              */
} FasListEvent;