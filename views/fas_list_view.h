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

typedef void (*FasListCallback)(void* context, int index, FasListEvent event);

typedef struct FasListView FasListView;

/* Lifecycle */
FasListView* fas_list_view_alloc(void);
void         fas_list_view_free(FasListView* lv);
View*        fas_list_view_get_view(FasListView* lv);
