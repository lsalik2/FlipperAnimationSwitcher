#include "fas_anim_view.h"

#include <furi.h>
#include <gui/canvas.h>
#include <gui/elements.h>
#include <string.h>

#include "../anim_meta.h"

#define FAS_ANIM_MSG_LEN 32

typedef struct {
    uint8_t frame[FAS_ANIM_XBM_MAX];
    int     w;
    int     h;
    bool    has_frame;
    char    msg[FAS_ANIM_MSG_LEN];
} FasAnimViewModel;

struct FasAnimView {
    View* view;
};

static void fas_anim_draw(Canvas* canvas, void* model_ptr) {
    FasAnimViewModel* m = model_ptr;

    canvas_clear(canvas);
    canvas_set_color(canvas, ColorBlack);

    if(m->has_frame) {
        /* Animations are normally exactly screen-sized; center a smaller one. */
        int x = (FAS_ANIM_MAX_W - m->w) / 2;
        int y = (FAS_ANIM_MAX_H - m->h) / 2;
        canvas_draw_xbm(canvas, x, y, (size_t)m->w, (size_t)m->h, m->frame);
    } else {
        canvas_set_font(canvas, FontSecondary);
        elements_multiline_text_aligned(
            canvas, FAS_ANIM_MAX_W / 2, FAS_ANIM_MAX_H / 2,
            AlignCenter, AlignCenter, m->msg);
    }
}

FasAnimView* fas_anim_view_alloc(void) {
    FasAnimView* av = malloc(sizeof(FasAnimView));
    furi_assert(av);

    av->view = view_alloc();
    view_allocate_model(av->view, ViewModelTypeLocking, sizeof(FasAnimViewModel));
    view_set_draw_callback(av->view, fas_anim_draw);
    view_set_context(av->view, av);
    /* No input callback on purpose: Back falls through to the scene manager. */

    with_view_model(
        av->view, FasAnimViewModel * m, { memset(m, 0, sizeof(FasAnimViewModel)); }, false);

    return av;
}

void fas_anim_view_free(FasAnimView* av) {
    view_free(av->view);
    free(av);
}

View* fas_anim_view_get_view(FasAnimView* av) {
    return av->view;
}

void fas_anim_view_set_frame(FasAnimView* av, const uint8_t* frame, int w, int h) {
    with_view_model(
        av->view,
        FasAnimViewModel * m,
        {
            if(frame == NULL || w <= 0 || h <= 0 || w > FAS_ANIM_MAX_W ||
               h > FAS_ANIM_MAX_H) {
                m->has_frame = false;
            } else {
                /* The bytes are copied, not referenced.  The player reuses one
                 * decode buffer for every frame, and a redraw is not
                 * synchronous with this call: borrowing the pointer would let
                 * a late redraw paint the *next* frame's bytes, tearing the
                 * image and coupling this view to the player's internals. */
                size_t bytes = (size_t)((w + 7) / 8) * (size_t)h;
                if(bytes > FAS_ANIM_XBM_MAX) bytes = FAS_ANIM_XBM_MAX;

                memcpy(m->frame, frame, bytes);
                m->w         = w;
                m->h         = h;
                m->has_frame = true;
            }
        },
        true);
}

void fas_anim_view_set_message(FasAnimView* av, const char* msg) {
    with_view_model(
        av->view,
        FasAnimViewModel * m,
        {
            strncpy(m->msg, msg, FAS_ANIM_MSG_LEN - 1);
            m->msg[FAS_ANIM_MSG_LEN - 1] = '\0';
            m->has_frame                 = false;
        },
        true);
}
