#pragma once

#include <gui/view.h>
#include <stdint.h>

typedef struct FasAnimView FasAnimView;

FasAnimView* fas_anim_view_alloc(void);
void         fas_anim_view_free(FasAnimView* av);
View*        fas_anim_view_get_view(FasAnimView* av);

/* Copies the frame into the view's own buffer.  Passing NULL (or a size the
 * buffer cannot hold) clears the frame, and the message is drawn instead. */
void fas_anim_view_set_frame(FasAnimView* av, const uint8_t* frame, int w, int h);

/* Drawn centered whenever no frame is set. */
void fas_anim_view_set_message(FasAnimView* av, const char* msg);
