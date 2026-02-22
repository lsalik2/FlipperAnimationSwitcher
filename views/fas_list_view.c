#include "fas_list_view.h"

#include <furi.h>
#include <gui/canvas.h>
#include <gui/elements.h>
#include <input/input.h>
#include <string.h>
#include <stdio.h>

/* ── Layout constants ─────────────────────────────────────────────────── */
#define VISIBLE_ROWS   4
#define ROW_HEIGHT    14
#define LIST_Y_START   2
#define CHECKBOX_W    10
#define CHECKBOX_H    10

/* ── Model (stored inside the View) ──────────────────────────────────── */
typedef struct {
    char labels[FAS_LIST_MAX_ITEMS][FAS_LIST_LABEL_LEN];
    bool has_checkbox[FAS_LIST_MAX_ITEMS];
    bool checked[FAS_LIST_MAX_ITEMS];
    int  count;
    int  cursor;
    int  scroll;  /* index of the topmost visible item */

    FasListCallback callback;
    void*           callback_ctx;
} FasListViewModel;

struct FasListView {
    View* view;
};

/* ── Draw ─────────────────────────────────────────────────────────────── */
static void fas_list_draw(Canvas* canvas, void* model_ptr) {
    FasListViewModel* m = model_ptr;
    canvas_clear(canvas);
    canvas_set_font(canvas, FontSecondary);

    for(int row = 0; row < VISIBLE_ROWS; row++) {
        int idx = m->scroll + row;
        if(idx >= m->count) break;

        int y_top  = LIST_Y_START + row * ROW_HEIGHT;
        int y_text = y_top + ROW_HEIGHT - 3; /* baseline for FontSecondary */
        bool sel   = (idx == m->cursor);

        if(sel) {
            canvas_set_color(canvas, ColorBlack);
            canvas_draw_box(canvas, 0, y_top, 128, ROW_HEIGHT);
            canvas_set_color(canvas, ColorWhite);
        } else {
            canvas_set_color(canvas, ColorBlack);
        }

        int x = 2;
        if(m->has_checkbox[idx]) {
            /* Draw checkbox border */
            int cb_y = y_top + (ROW_HEIGHT - CHECKBOX_H) / 2;
            if(sel) {
                canvas_set_color(canvas, ColorWhite);
            }
            canvas_draw_rframe(canvas, x, cb_y, CHECKBOX_W, CHECKBOX_H, 1);
            if(m->checked[idx]) {
                /* Draw a small X inside */
                canvas_draw_line(canvas, x + 2, cb_y + 2, x + CHECKBOX_W - 3, cb_y + CHECKBOX_H - 3);
                canvas_draw_line(canvas, x + CHECKBOX_W - 3, cb_y + 2, x + 2, cb_y + CHECKBOX_H - 3);
            }
            if(sel) canvas_set_color(canvas, ColorWhite);
            x += CHECKBOX_W + 3;
        }

        /* Truncate label to fit available width */
        int max_chars = m->has_checkbox[idx] ? 17 : 20;
        char display[FAS_LIST_LABEL_LEN];
        snprintf(display, sizeof(display), "%.*s", max_chars, m->labels[idx]);
        canvas_draw_str(canvas, x, y_text, display);

        if(sel) canvas_set_color(canvas, ColorBlack);
    }

    /* Scroll indicators */
    if(m->scroll > 0) {
        canvas_draw_str(canvas, 120, 10, "^");
    }
    if(m->scroll + VISIBLE_ROWS < m->count) {
        canvas_draw_str(canvas, 120, 60, "v");
    }
}

/* ── Input ────────────────────────────────────────────────────────────── */
static bool fas_list_input(InputEvent* event, void* context) {
    FasListView* lv = context;
    bool consumed   = false;